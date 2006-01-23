/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <socket.h>
#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif

#include <ctype.h>

#include "md5.h"

#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmNetEndpointRoster.h"
#include "BmNetJobModel.h"
#include "BmPrefs.h"

/********************************************************************************\
	BmStatusFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmStatusFilter::BmStatusFilter( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize, nTagImmediatePassOn)
	,	mHaveStatus( false)
	,	mUpdate( false)
	,	mInfoMsg( NULL)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmStatusFilter::Reset( BmMemIBuf* input)
{
	inherited::Reset( input);
	mHaveStatus = false;
	mStatusText.Truncate( 0);
	mBottomStatusText.Truncate( 0);
	mInfoMsg = NULL;
}



/********************************************************************************\
	BmNetJobModel
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME Name()

const char* const BmNetJobModel::MSG_MODEL = 		"bm:model";
const char* const BmNetJobModel::MSG_DELTA = 		"bm:delta";
const char* const BmNetJobModel::MSG_TRAILING = 	"bm:trailing";
const char* const BmNetJobModel::MSG_LEADING = 		"bm:leading";
const char* const BmNetJobModel::MSG_ENCRYPTED = 	"bm:encrypted";
const char* const BmNetJobModel::MSG_FAILED = 		"bm:failed";

const char* const BmNetJobModel::IMSG_NEED_DATA = 	"needData";

/*------------------------------------------------------------------------------*\
	BmNetJobModel()
		-	constructor
\*------------------------------------------------------------------------------*/
BmNetJobModel::BmNetJobModel( const BmString& name, uint32 logType, 
										BmStatusFilter* statusFilter)
	:	inherited( name)
	,	mConnection( NULL)
	,	mConnected( false)
	,	mStatusFilter( statusFilter)
	,	mLogType( logType)
{
	mReader = new BmNetIBuf( this);
	mWriter = new BmNetOBuf( this);
}

/*------------------------------------------------------------------------------*\
	~BmNetJobModel()
		-	destructor
\*------------------------------------------------------------------------------*/
BmNetJobModel::~BmNetJobModel() 
{
	Disconnect();
	delete mWriter;
	delete mReader;
	delete mStatusFilter;
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetJobModel::Connect( const BNetAddress* addr)
{
	Disconnect();
	mConnection = TheNetEndpointRoster->CreateEndpoint();
	mErrorString.Truncate( 0);
	if (mConnection->InitCheck() != B_OK) {
		mErrorString = "unable to create BNetEndpoint";
		return false;
	}
	status_t err;
	if ((err=mConnection->Connect( *addr)) != B_OK) {
		mErrorString = strerror(err);
		return false;
	}
	mConnected = true;
	return true;
}

/*------------------------------------------------------------------------------*\
	Disconnect()
		-	
\*------------------------------------------------------------------------------*/
void BmNetJobModel::Disconnect()
{
	if (mConnection) {
		if (mConnected)
			mConnection->Close();
		delete mConnection;
		mConnection = NULL;
	}
	mConnected = false;
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the netjob should continue to run
		-	if the job has been stopped, we pass this info on to our
			status-filter so that this will really stop the network connection
\*------------------------------------------------------------------------------*/
bool BmNetJobModel::ShouldContinue()
{
	bool shouldCont = inherited::ShouldContinue();
	if (!shouldCont)
		mStatusFilter->Stop();
	return shouldCont;
}

/*------------------------------------------------------------------------------*\
	CheckForPositiveAnswer()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetJobModel::CheckForPositiveAnswer( uint32 expectedSize, 
														  bool dotstuffDecoding,
														  bool update,
														  BMessage* infoMsg)
{
	BM_ASSERT( mStatusFilter);
	GetAnswer( expectedSize, dotstuffDecoding, update, infoMsg);
	return mStatusFilter->CheckForPositiveAnswer() && ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	GetAnswer()
		-	
\*------------------------------------------------------------------------------*/
void BmNetJobModel::GetAnswer( uint32 expectedSize, bool dotstuffDecoding,
										 bool update, BMessage* infoMsg) 
{
	uint32 logLevel = ThePrefs->GetNumericLogLevelFor(mLogType);
	int32 logLimits[4] = { 0, 256, 2048, 32768 };
	BmTrafficLogger logger(mReader, this, logLimits[logLevel], "<--\n");

	mStatusFilter->Reset( &logger);
	mStatusFilter->DoUpdate( update);
	if (!infoMsg) {
		mInfoMsg.MakeEmpty();
		infoMsg = &mInfoMsg;
	}
	mStatusFilter->SetInfoMsg(infoMsg);

	uint32 blockSize = ThePrefs->GetInt( "NetReceiveBufferSize", 10*1500);
	BmStringOBuf answerBuf( max( expectedSize+128, blockSize), 2.0);

	if (dotstuffDecoding) {
		bool dummy;
		if (infoMsg->FindBool(IMSG_NEED_DATA, &dummy) != B_OK) {
			// dotstuff-decoding means that there needs to be data (or else
			// there would be nothing we could decode...):
			infoMsg->AddBool(IMSG_NEED_DATA, true);
		}
		BmDotstuffDecoder decoder( mStatusFilter, this, blockSize);
		answerBuf.Write( &decoder, blockSize);
	} else
		answerBuf.Write( mStatusFilter, blockSize);
	mAnswerText.Adopt( answerBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	sends the specified command to the server.
\*------------------------------------------------------------------------------*/
void BmNetJobModel::SendCommand( const BmString& cmd, const BmString& secret,
											bool dotstuffEncoding, bool update) 
{
	BmStringIBuf cmdBuf( cmd);
	SendCommandBuf( cmdBuf, secret, dotstuffEncoding, update);
}

/*------------------------------------------------------------------------------*\
	_SendCommand( cmd)
		-	sends the specified command to the server.
\*------------------------------------------------------------------------------*/
void BmNetJobModel::SendCommandBuf( BmStringIBuf& cmd, 
												const BmString& secret,
												bool dotstuffEncoding, bool update)
{
	BmString logStr( "-->\n");
	logStr.Append( cmd.FirstBuf(), min( (uint32)16384, cmd.FirstSize()));
	if (secret.Length()) {
		logStr << " secret_data_omitted_here";
							// we do not want to log any passwords...
		cmd.AddBuffer( secret);
	}
	BM_LOG( mLogType, logStr);
	if (!cmd.EndsWithNewline())
		cmd.AddBuffer( "\r\n", 2);
	uint32 blockSize = ThePrefs->GetInt( "NetSendBufferSize", 10*1500);
	mWriter->DoUpdate( update);
	uint32 writtenLen;
	if (dotstuffEncoding) {
		BmDotstuffEncoder encoder( &cmd, this, blockSize);
		writtenLen = mWriter->Write( &encoder, blockSize);
	} else
		writtenLen = mWriter->Write( &cmd, blockSize);
	if (ShouldContinue() && writtenLen < cmd.Size()) {
		BmString s = BmString( "Wrote only ") << writtenLen 
							<< " bytes when at least " << cmd.Size() 
							<< " should have been written";
		throw BM_network_error( s);
	}
}

/*------------------------------------------------------------------------------*\
	AuthDigestMD5()
		-	
\*------------------------------------------------------------------------------*/
void BmNetJobModel::AuthDigestMD5( const BmString& username,
											  const BmString& password,
											  const BmString& serviceUri)
{ 
	// splits a challenge-string into the given map (see RFC-2831):
	struct MapSplitter {
		bool operator() (BmString str, map<BmString,BmString>& m)
		{
			m.clear();
			const char* key;
			const char* val;
			char* s = (char*)str.String();
			while(*s != 0) {
				while(isspace(*s))
					s++;
				key = s;
				while(isalpha(*s))
					s++;
				if (*s != '=')
					return false;
				*s++ = '\0';
				while(isspace(*s))
					s++;
				if (*s=='"') {
					val = ++s;
					while(*s!='"') {
						if (*s == 0)
							return false;
						s++;
					}			
					*s++ = '\0';
				} else {
					val = s;
					while(*s!=0 && *s!=',' && !isspace(*s))
						s++;
					if (*s != 0)
						*s++ = '\0';
				}
				m[key] = val;
				while(isspace(*s))
					s++;
				if (*s != ',')
					return false;
				s++;
			}
			return true;
		}
	};
	// DIGEST-MD5-method: receive digest-challenge, then send
	// digest-response (that includes several hashings, one of which
	// contains the password):
	//
	// N.B.: we use the local implementations of sending and receiving
	//       methods as using the protocol-specific version would interfere
	//			with the way this authentication mechanism works.
	if (BmNetJobModel::CheckForPositiveAnswer()) {
		BmString base64, rawChallenge;
		ExtractBase64(StatusText(), base64);
		Decode( "base64", base64, rawChallenge);
		map<BmString,BmString> challengeMap;
		BM_LOG2( mLogType, BmString("challenge:\n") << rawChallenge);
		MapSplitter mapSplitter;
		mapSplitter(rawChallenge, challengeMap);
		BmString rawResponse = BmString("username=") << '"' << username << '"';
		rawResponse << ",realm=" << '"' << challengeMap["realm"] << '"';
		rawResponse << ",nonce=" << '"' << challengeMap["nonce"] << '"';
		char temp[33];
		srand(system_time());
		for( int i=0; i<32; ++i)
			temp[i] = 1+(rand()%254);
		temp[32] = '\0';
		BmString rawCnonce(temp);
		BmString cnonce;
		Encode("base64", rawCnonce, cnonce);
		rawResponse	<< ",cnonce=" << '"' << cnonce << '"';
		rawResponse	<< ",nc=00000001";
		rawResponse	<< ",qop=auth";
		rawResponse	<< ",digest-uri=" << '"' << serviceUri << '"';
		char sum0[17], hd1[33], hd2[33], hd3[33];
		BmString a1,a2,kd;
		BmString t1 = username + ":" + challengeMap["realm"] + ":" + password;
		MD5Sum((unsigned char*)t1.String(), sum0);
		a1 << sum0 << ":" << challengeMap["nonce"] << ":" << cnonce;
		MD5Digest((unsigned char*)a1.String(), hd1);
		a2 << "AUTHENTICATE:" << serviceUri;
		MD5Digest((unsigned char*)a2.String(), hd2);
		kd << hd1 << ':' << challengeMap["nonce"] << ":" << "00000001"
			<< ':' << cnonce << ':' << "auth" << ':' << hd2;
		MD5Digest((unsigned char*)kd.String(), hd3);
		rawResponse	<< ",response=" << hd3;
//		rawResponse	<< ",charset=utf-8";
		rawResponse	<< ",maxbuf=65535";
		BM_LOG2( mLogType, BmString("response:\n") << rawResponse);
		BmString tags = BmBase64Encoder::nTagOnSingleLine;
		Encode( "base64", rawResponse, base64, tags);
		BmNetJobModel::SendCommand( base64);
		if (BmNetJobModel::CheckForPositiveAnswer()) {
			// now compute expected answer from server and compare it with
			// real server response:
			BmString a2s, serverAuthResp, clientAuthResp;
			ExtractBase64(StatusText(), base64);
			Decode( "base64", base64, serverAuthResp);
			a2s << ":" << serviceUri;
			MD5Digest((unsigned char*)a2s.String(), hd2);
			kd = "";
			kd << hd1 << ':' << challengeMap["nonce"] << ":" << "00000001"
				<< ':' << cnonce << ':' << "auth" << ':' << hd2;
			MD5Digest((unsigned char*)kd.String(), hd3);
			clientAuthResp << "rspauth=" << hd3;
			BM_LOG2( mLogType, BmString("expected <") << clientAuthResp.String()
										<< ">\ngot <" << serverAuthResp.String() << ">");
			if (serverAuthResp != clientAuthResp)
				throw BM_network_error(
					"ATTENTION: Auth-Response from Server is incorrect!\n"
					"This server probably isn't the right one, we better stop."
				);
			BmNetJobModel::SendCommand( "");
		}
	}
}

/*------------------------------------------------------------------------------*\
	AuthCramMD5()
		-	
\*------------------------------------------------------------------------------*/
void BmNetJobModel::AuthCramMD5( const BmString& username, 
											const BmString& pwd)
{ 
	// CRAM-MD5-method: receive timestamp, then send
	// md5-hash of this timestamp (with password used as key):
	//
	// N.B.: we use the local implementations of sending and receiving
	//       methods as using the protocol-specific version would interfere
	//			with the way this authentication mechanism works.
	if (BmNetJobModel::CheckForPositiveAnswer()) {
		BmString base64, timestamp;
		ExtractBase64(StatusText(), base64);
		Decode( "base64", base64, timestamp);
		BM_LOG2( mLogType, BmString("timestamp:\n") << timestamp);
		char digest[33];
		MD5_HMAC( (unsigned char*)timestamp.String(), timestamp.Length(),
					 (unsigned char*)pwd.String(), pwd.Length(), 
					 (unsigned char*)digest);
		BmString raw = username + " " + digest;
		BM_LOG2( mLogType, BmString("response:\n") << raw);
		Encode( "base64", raw, base64);
		BmNetJobModel::SendCommand( base64);
	}
}



/********************************************************************************\
	BmTrafficLogger
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME mJob->Name()

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmTrafficLogger::BmTrafficLogger( BmMemIBuf* input, BmNetJobModel* job,
											 int32 logLimit, const char* prefix, 
											 uint32 blockSize)
	:	inherited( input, blockSize, nTagImmediatePassOn)
	,	mLogLimit( logLimit)
	,	mLoggedLength( 0)
	,	mJob( job)
	,	mPrefix( prefix)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmTrafficLogger::Filter( const char* srcBuf, uint32& srcLen, 
										char* destBuf, uint32& destLen)
{
	BM_LOG3( mJob->LogType(), 
				BmString("starting to log traffic of ") << srcLen << " bytes");

	if (mLogLimit < 0 || mLoggedLength < mLogLimit) {
		uint32 logLeft = mLogLimit < 0 ? srcLen : mLogLimit - mLoggedLength;
		uint32 logLen = min(srcLen, logLeft);
		BmString logStr = mPrefix;
		logStr.Append(srcBuf, logLen);
		mLoggedLength += logLen;
		BM_LOG( mJob->LogType(), logStr);
	}

	// pass on the data:
	uint32 size = min( destLen, srcLen);
	memcpy( destBuf, srcBuf, size);
	srcLen = destLen = size;
	BM_LOG3( mJob->LogType(), "traffic logger: done");
}



/********************************************************************************\
	BmDotstuffDecoder
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME mJob->Name()

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmDotstuffDecoder::BmDotstuffDecoder( BmMemIBuf* input, BmNetJobModel* job,
												  uint32 blockSize)
	:	inherited( input, blockSize, nTagImmediatePassOn)
	,	mAtStartOfLine( true)
	,	mHaveDotAtStartOfLine( false)
	,	mJob( job)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmDotstuffDecoder::Filter( const char* srcBuf, uint32& srcLen, 
											char* destBuf, uint32& destLen)
{
	BM_LOG3( mJob->LogType(), 
				BmString("starting to decode dot-stuffing of ") 
					<< srcLen << " bytes");
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	char c;
	for( ; src<srcEnd && dest<destEnd && !mEndReached; ++src) {
		if (mHaveDotAtStartOfLine) {
			if (*src != '.') {
				mEndReached = true;
				continue;
			}
			mHaveDotAtStartOfLine = false;
		}
		if ((c = *src)!='.' || !mAtStartOfLine)
			*dest++ = c;
		else
			mHaveDotAtStartOfLine = true;
		mAtStartOfLine = (c=='\n');
	}

	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( mJob->LogType(), "dotstuff-decode: done");
}



/********************************************************************************\
	BmDotstuffEncoder
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME mJob->Name()

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmDotstuffEncoder::BmDotstuffEncoder( BmMemIBuf* input, BmNetJobModel* job,
												  uint32 blockSize)
	:	inherited( input, blockSize, nTagImmediatePassOn)
	,	mAtStartOfLine( true)
	,	mJob( job)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmDotstuffEncoder::Filter( const char* srcBuf, uint32& srcLen, 
										  char* destBuf, uint32& destLen)
{
	BM_LOG3( mJob->LogType(), 
				BmString("starting to dot-stuff a string of ") 
					<< srcLen << " bytes");

	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	char c;
	for( ; src<srcEnd && dest<destEnd; ++src) {
		if ((c = *src)=='.' && mAtStartOfLine) {
			if (dest>destEnd-2)
				break;
			*dest++ = c;
		}
		*dest++ = c;
		mAtStartOfLine = (c=='\n');
	}

	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( mJob->LogType(), "dotstuff-encode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmDotstuffEncoder::Finalize( char* destBuf, uint32& destLen) 
{
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (dest <= destEnd-3) {
		// output a dot on an empty line:
		*dest++ = '.';
		*dest++ = '\r';
		*dest++ = '\n';
		mIsFinalized = true;
	}
	destLen = dest-destBuf;
}



/********************************************************************************\
	BmNetIBuf
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME mJob->Name()

/*------------------------------------------------------------------------------*\
	NetIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmNetIBuf::BmNetIBuf( BmNetJobModel* job)
	:	mJob( job)
{
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmNetIBuf::Read( char* dest, uint32 destLen)
{
	int32 feedbackTimeout = ThePrefs->GetInt("FeedbackTimeout", 200)*1000;
	int32 timeout = ThePrefs->GetInt("ReceiveTimeout")*1000*1000;
	int32 timeWaiting = 0;
	int32 numBytes = 0;
	Connection()->SetTimeout( feedbackTimeout);
	while( mJob->ShouldContinue() && !numBytes) {
		BM_LOG3( mJob->LogType(), 
					BmString("Trying to receive up to ") << destLen << " bytes...");
		numBytes = Connection()->Receive( dest, destLen);
		BM_LOG3( mJob->LogType(), 
					BmString("...received ") << numBytes << " bytes");
		if (numBytes <= 0) {
			timeWaiting += feedbackTimeout;
	 		if (timeWaiting >= timeout)
	 			throw BM_network_error( "no answer from server (timeout)");
			if (numBytes < 0)
				throw BM_network_error( Connection()->ErrorStr());
		}
	}
	return numBytes;
}

/*------------------------------------------------------------------------------*\
	IsAtEnd()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetIBuf::IsAtEnd() 
{
	return false;
}



/********************************************************************************\
	BmNetOBuf
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME mJob->Name()

/*------------------------------------------------------------------------------*\
	NetOBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmNetOBuf::BmNetOBuf( BmNetJobModel* job)
	:	mJob( job)
	,	mUpdate( false)
{
}

/*------------------------------------------------------------------------------*\
	Write( data, len)
		-	sends given data to the server
\*------------------------------------------------------------------------------*/
uint32 BmNetOBuf::Write( const char* data, uint32 len) 
{
	uint32 sentSize=0;
	for( uint32 offs=0; mJob->ShouldContinue() && offs<len; ) {
		int32 sz = len-offs;
		BM_LOG3( mJob->LogType(), 
					BmString("Trying to send ") << sz << " bytes...");
		int32 sent = Connection()->Send( data+offs, sz);
		BM_LOG3( mJob->LogType(), 
					BmString("...sent ") << sent << " bytes");
		if (sent < 0)
			throw BM_network_error( Connection()->ErrorStr());
		else {
			offs += sent;
			sentSize += (uint32)sent;
			if (mUpdate)
				mJob->UpdateProgress( sent);
			if (sent != sz)
				throw BM_network_error( BmString("error during send, sent only ") 
													<< sent << " bytes instead of " << sz);
		}
	}
	return sentSize;
}

/*------------------------------------------------------------------------------*\
	Write( input)
		-	adds all data from given BmMemIBuf input to end of string
\*------------------------------------------------------------------------------*/
uint32 BmNetOBuf::Write( BmMemIBuf* input, uint32 blockSize) 
{
	char* buf = new char [blockSize];
	uint32 writeLen=0;
	try {
		uint32 len;
		while( mJob->ShouldContinue() && !input->IsAtEnd()) {
			len = input->Read( buf, blockSize);
			writeLen += Write( buf, len);
		}
		delete [] buf;
	} catch (BM_error& err) {
		delete [] buf;
		throw err;
	}
	return writeLen;
}
