/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmNetJobModel_h
#define _BmNetJobModel_h

#include "BmDaemon.h"

#include "BmDataModel.h"
#include "BmMemIO.h"
#include "BmNetEndpoint.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	class BmStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmStatusFilter : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmStatusFilter( BmMemIBuf* input, uint32 blockSize=65536);

	// native methods:
	virtual bool CheckForPositiveAnswer() = 0;
	virtual void Stop()						{ mHadError = true; }

	// overrides of BmMemFilter-base:
	void Reset( BmMemIBuf* input=NULL);

	// getters:
	inline const BmString& StatusText() const
													{ return mStatusText; }
	inline const BmString& BottomStatusText() const
													{ return mBottomStatusText; }

	// setters:
	inline void DoUpdate( bool b)			{ mUpdate = b; }
	void SetInfoMsg(BMessage* infoMsg)	{ mInfoMsg = infoMsg; }

protected:
	BmString mStatusText;
		// status server sent before any data
	BmString mBottomStatusText;
		// status server sent after any data
	bool mHaveStatus;
	bool mUpdate;
	BMessage* mInfoMsg;
};

class BmSocket;
/*------------------------------------------------------------------------------*\
	class BmNetJobModel
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmNetJobModel : public BmJobModel {
	typedef BmJobModel inherited;

	friend class BmNetIBuf;
	friend class BmNetOBuf;

public:
	BmNetJobModel( const BmString& name, uint32 logType, 
						BmStatusFilter* statusFilter);
	~BmNetJobModel();

	// native methods:
	virtual void UpdateProgress( uint32 numBytes) = 0;

	// overrides of BmJobModel base:
	bool ShouldContinue();

	// getters:
	inline BmNetEndpoint* Connection()	{ return mConnection; }
	inline bool Connected() const			{ return mConnected; }
	inline uint32 LogType() const			{ return mLogType; }
	inline const BmString& AnswerText() const
													{ return mAnswerText; }
	inline const BmString& StatusText() const
													{ return mStatusFilter->StatusText(); }
	inline const BmString& BottomStatusText() const
													{ return mStatusFilter
																	->BottomStatusText(); }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;
	static const char* const MSG_ENCRYPTED;
	static const char* const MSG_FAILED;

	// message components used for info-msgs (communication between
	// protocol implementation and protocol-specific status filter):
	static const char* const IMSG_NEED_DATA;

protected:
	// native methods:
	virtual bool Connect( const BNetAddress* addr);
	virtual void Disconnect();
	virtual bool CheckForPositiveAnswer( uint32 expectedSize=4096, 
													 bool dotstuffDecoding=false,
													 bool update=false,
													 BMessage* infoMsg=NULL);
	virtual void GetAnswer( uint32 expectedSize=4096, 
									bool dotstuffDecoding=false,
									bool update=false,
									BMessage* infoMsg=NULL);
	virtual void SendCommand( const BmString& cmd, 
									  const BmString& secret=BM_DEFAULT_STRING,
									  bool dotstuffEncoding=false,
									  bool update=false);
	virtual void SendCommandBuf( BmStringIBuf& cmd, 
										  const BmString& secret=BM_DEFAULT_STRING, 
										  bool dotstuffEncoding=false,
										  bool update=false);

	virtual void ExtractBase64(const BmString& text, BmString& base64) = 0;

	void AuthDigestMD5( const BmString& username, const BmString& password,
							  const BmString& serviceUri);
	void AuthCramMD5( const BmString& username, const BmString& password);

	BmNetEndpoint* mConnection;
	bool mConnected;
	BmStatusFilter* mStatusFilter;
							// filter that splits server-reply into data and status 
							// part (the status part is kept inside the filter object)
	BMessage mInfoMsg;
							// message with protocol-specific info for status filter
	BmString mAnswerText;
							// data part of server-reply
	BmString mErrorString;
							// error-text of last failed command (Beam-generated, 
							// not from server)
	uint32 mLogType;
							// log-type can be BM_LogRecv or BM_LogSmtp
	BmNetIBuf* mReader;
							// input stream
	BmNetOBuf* mWriter;
							// output stream

	// Hide copy-constructor and assignment:
	BmNetJobModel( const BmNetJobModel&);
#ifndef __POWERPC__
	BmNetJobModel operator=( const BmNetJobModel&);
#endif
};


/*------------------------------------------------------------------------------*\
	class BmTrafficLogger
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmTrafficLogger : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmTrafficLogger( BmMemIBuf* input, BmNetJobModel* job, int32 logLimit,
						  const char* prefix, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	int32 mLogLimit;
	int32 mLoggedLength;
	BmNetJobModel* mJob;
	BmString mPrefix;
};


/*------------------------------------------------------------------------------*\
	class BmDotstuffDecoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmDotstuffDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmDotstuffDecoder( BmMemIBuf* input, BmNetJobModel* job,
							 uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	bool mAtStartOfLine;
	bool mHaveDotAtStartOfLine;
	BmNetJobModel* mJob;
};


/*------------------------------------------------------------------------------*\
	class BmDotstuffEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmDotstuffEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmDotstuffEncoder( BmMemIBuf* input, BmNetJobModel* job,
							 uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	bool mAtStartOfLine;
	BmNetJobModel* mJob;
};


/*------------------------------------------------------------------------------*\
	class BmNetIBuf
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmNetIBuf : public BmMemIBuf {
	typedef BmMemIBuf inherited;

public:
	BmNetIBuf( BmNetJobModel* job);
	
	// overrides of BmMemIBuf base:
	uint32 Read( char* data, uint32 reqLen);
	bool IsAtEnd();

	// getters:
	BmNetEndpoint* Connection()			{ return mJob 
																? mJob->Connection() 
																: NULL; }

protected:
	BmNetJobModel* mJob;
	
};


/*------------------------------------------------------------------------------*\
	class BmNetOBuf
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmNetOBuf {

public:
	BmNetOBuf( BmNetJobModel* job);

	// overrides/overloads of BmMemOBuf base:
	uint32 Write( const char* data, uint32 len);
	uint32 Write( BmMemIBuf* input, uint32 blockSize);

	// getters:
	BmNetEndpoint* Connection()			{ return mJob 
																? mJob->Connection() 
																: NULL; }

	// setters:
	inline void DoUpdate( bool b)			{ mUpdate = b; }

protected:
	BmNetJobModel* mJob;
	bool mUpdate;

};
							 

#endif
