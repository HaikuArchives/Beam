/*
	BmNetJobModel.cpp

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

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
	:	inherited( input, blockSize, true)
	,	mUpdate( false)
	,	mHaveStatus( false)
	,	mNeedData( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmStatusFilter::Reset( BmMemIBuf* input) {
	inherited::Reset( input);
	mHaveStatus = false;
	mNeedData = false;
	mStatusText.Truncate( 0);
}



/********************************************************************************\
	BmNetJobModel
\********************************************************************************/

#undef BM_LOGNAME
#define BM_LOGNAME Name()

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
	,	mPwdAcquisitorFunc( NULL)
	,	mLogType( logType)
{
	mReader = new BmNetIBuf( this);
	mWriter = new BmNetOBuf( this);
}

/*------------------------------------------------------------------------------*\
	~BmNetJobModel()
		-	destructor
\*------------------------------------------------------------------------------*/
BmNetJobModel::~BmNetJobModel() { 
	Disconnect();
	delete mWriter;
	delete mReader;
	delete mStatusFilter;
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetJobModel::Connect( const BNetAddress& addr) { 
	Disconnect();
	mConnection = new BNetEndpoint;
	mErrorString.Truncate( 0);
	if (mConnection->InitCheck() != B_OK) {
		mErrorString = "unable to create BNetEndpoint";
		return false;
	}
	status_t err;
	if ((err=mConnection->Connect( addr)) != B_OK) {
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
void BmNetJobModel::Disconnect() {
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
bool BmNetJobModel::ShouldContinue() {
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
														  bool update) {
	assert( mStatusFilter);
	GetAnswer( expectedSize, dotstuffDecoding, update);
	return mStatusFilter->CheckForPositiveAnswer() && ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	GetAnswer()
		-	
\*------------------------------------------------------------------------------*/
void BmNetJobModel::GetAnswer( uint32 expectedSize, bool dotstuffDecoding,
										 bool update) {
	uint32 blockSize = ThePrefs->GetInt( "NetReceiveBufferSize", 10*1500);
	BmStringOBuf answerBuf( max( expectedSize+128, blockSize), 2.0);
	mStatusFilter->Reset( mReader);
	mStatusFilter->DoUpdate( update);
	mStatusFilter->NeedData( dotstuffDecoding);
	if (dotstuffDecoding) {
		BmDotstuffDecoder decoder( mStatusFilter, this, blockSize);
		answerBuf.Write( &decoder, blockSize);
	} else
		answerBuf.Write( mStatusFilter, blockSize);
	mAnswerText.Adopt( answerBuf.TheString());
#ifdef BM_LOGGING
	BmString logStr( "<--\n");
	logStr.Append( StatusText() + "\n");
	logStr.Append( mAnswerText.String(), min( (int32)1024, mAnswerText.Length()));
	BM_LOG( mLogType, logStr);
#endif
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	sends the specified command to the server.
\*------------------------------------------------------------------------------*/
void BmNetJobModel::SendCommand( const BmString& cmd, const BmString& secret,
											bool dotstuffEncoding, bool update) {
	BmStringIBuf cmdBuf( cmd);
	SendCommand( cmdBuf, secret, dotstuffEncoding, update);
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	sends the specified command to the server.
\*------------------------------------------------------------------------------*/
void BmNetJobModel::SendCommand( BmStringIBuf& cmd, const BmString& secret,
											bool dotstuffEncoding, bool update) {
#ifdef BM_LOGGING
	BmString logStr( "-->\n");
	logStr.Append( cmd.FirstBuf(), min( (uint32)1024, cmd.FirstSize()));
	if (secret.Length()) {
		logStr << " secret_data_omitted_here";
							// we do not want to log any passwords...
		cmd.AddBuffer( secret);
	}
	BM_LOG( mLogType, logStr);
#else
	if (secret.Length())
		cmd.AddBuffer( secret);
#endif
	if (!cmd.EndsWithNewline())
		cmd.AddBuffer( "\r\n", 2);
	uint32 blockSize = ThePrefs->GetInt( "NetReceiveBufferSize", 10*1500);
	mWriter->DoUpdate( update);
	uint32 writtenLen;
	if (dotstuffEncoding) {
		BmDotstuffEncoder encoder( &cmd, this, blockSize);
		writtenLen = mWriter->Write( &encoder, blockSize);
	} else
		writtenLen = mWriter->Write( &cmd, blockSize);
	if (writtenLen < cmd.Size()) {
		BmString s = BmString( "Wrote only ") << writtenLen 
							<< " bytes when at least " << cmd.Size() 
							<< " should have been written";
		throw BM_network_error( s);
	}
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
	:	inherited( input, blockSize, true)
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
											char* destBuf, uint32& destLen) {
	BM_LOG3( mJob->LogType(), BmString("starting to decode dot-stuffing of ") << srcLen << " bytes");
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
	:	inherited( input, blockSize, true)
	,	mAtStartOfLine( true)
	,	mJob( job)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmDotstuffEncoder::Filter( const char* srcBuf, uint32& srcLen, 
										  char* destBuf, uint32& destLen) {
	BM_LOG3( mJob->LogType(), BmString("starting to dot-stuff a string of ") << srcLen << " bytes");

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
void BmDotstuffEncoder::Finalize( char* destBuf, uint32& destLen) {
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
uint32 BmNetIBuf::Read( char* dest, uint32 destLen) {
	int32 feedbackTimeout = ThePrefs->GetInt("FeedbackTimeout", 200)*1000;
	int32 timeout = ThePrefs->GetInt("ReceiveTimeout")*1000*1000;
	int32 timeWaiting = 0;
	int32 numBytes = 0;
	Connection()->SetTimeout( feedbackTimeout);
	while( mJob->ShouldContinue() && !numBytes) {
		BM_LOG3( mJob->LogType(), BmString("Trying to receive up to ") << destLen << " bytes...");
		numBytes = Connection()->Receive( dest, destLen);
		BM_LOG3( mJob->LogType(), BmString("...received ") << numBytes << " bytes");
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
bool BmNetIBuf::IsAtEnd() {
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
uint32 BmNetOBuf::Write( const char* data, uint32 len) {
	uint32 sentSize;
	uint32 blockSize = ThePrefs->GetInt("NetSendBufferSize", 10*1500);
	for( uint32 offs=0; offs<len; ) {
		int32 sz = MIN( len-offs, blockSize);
		BM_LOG3( mJob->LogType(), BmString("Trying to send ") << sz << " bytes...");
		int32 sent = Connection()->Send( data+offs, sz);
		BM_LOG3( mJob->LogType(), BmString("...sent ") << sent << " bytes");
		if (sent < 0)
			throw BM_network_error( Connection()->ErrorStr());
		else {
			offs += sent;
			sentSize += (uint32)sent;
			if (mUpdate)
				mJob->UpdateProgress( sent);
			if (sent != sz)
				throw BM_network_error( BmString("error during send, sent only ") << sent << " bytes instead of " << sz);
		}
	}
	return sentSize;
}

/*------------------------------------------------------------------------------*\
	Write( input)
		-	adds all data from given BmMemIBuf input to end of string
\*------------------------------------------------------------------------------*/
uint32 BmNetOBuf::Write( BmMemIBuf* input, uint32 blockSize) {
	char* buf = new char [blockSize];
	uint32 writeLen=0;
	try {
		uint32 len;
		while( !input->IsAtEnd()) {
			len = input->Read( buf, blockSize);
			writeLen += Write( buf, len);
		}
		delete [] buf;
	} catch (exception& err) {
		delete [] buf;
		throw err;
	}
	return writeLen;
}