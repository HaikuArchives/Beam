/*
	BmNetIO.h
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


#ifndef _BmNetJobModel_h
#define _BmNetJobModel_h

#include "BmDataModel.h"
#include "BmMemIO.h"
#include "BmUtil.h"

class BNetAddress;
class BNetEndpoint;
/*------------------------------------------------------------------------------*\
	class BmStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class BmStatusFilter : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmStatusFilter( BmMemIBuf* input, uint32 blockSize=65536);

	// native methods:
	virtual bool CheckForPositiveAnswer() = 0;
	virtual void Stop()						{ mHadError = true; }

	// overrides of BmMemFilter-base:
	void Reset( BmMemIBuf* input);

	// getters:
	inline const BmString& StatusText() const
													{ return mStatusText; }

	// setters:
	inline void DoUpdate( bool b)			{ mUpdate = b; }
	inline void NeedData( bool b)			{ mNeedData = b; }

protected:
	BmString mStatusText;
	bool mUpdate;
	bool mHaveStatus;
	bool mNeedData;

};

/*------------------------------------------------------------------------------*\
	class BmNetJobModel
		-	
\*------------------------------------------------------------------------------*/
class BmNetJobModel : public BmJobModel {
	typedef BmJobModel inherited;

	friend class BmNetIBuf;
	friend class BmNetOBuf;

public:
	BmNetJobModel( const BmString& name, uint32 logType, 
						BmStatusFilter* statusFilter);
	~BmNetJobModel();

	// native methods:
	virtual bool Connect( const BNetAddress* addr);
	virtual void Disconnect();
	virtual bool CheckForPositiveAnswer( uint32 expectedSize=4096, 
													 bool dotstuffDecoding=false,
													 bool update=false);
	virtual void GetAnswer( uint32 expectedSize=4096, 
									bool dotstuffDecoding=false,
									bool update=false);
	virtual void SendCommand( const BmString& cmd, 
									  const BmString& secret=BM_DEFAULT_STRING,
									  bool dotstuffEncoding=false,
									  bool update=false);
	virtual void SendCommand( BmStringIBuf& cmd, 
									  const BmString& secret=BM_DEFAULT_STRING, 
									  bool dotstuffEncoding=false,
									  bool update=false);

	virtual void UpdateProgress( uint32 numBytes) = 0;

	// overrides of BmJobModel base:
	bool ShouldContinue();

	// getters:
	inline BNetEndpoint* Connection()	{ return mConnection; }
	inline bool Connected() const			{ return mConnected; }
	inline uint32 LogType() const			{ return mLogType; }
	inline const BmString& AnswerText() const
													{ return mAnswerText; }
	inline const BmString& StatusText() const
													{ return mStatusFilter->StatusText(); }

	// setters:

protected:
	BNetEndpoint* mConnection;
	bool mConnected;
	BmStatusFilter* mStatusFilter;
							// filter that splits server-reply into data and status 
							// part (the status part is kept inside the filter object)
	BmString mAnswerText;
							// data part of server-reply
	BmString mErrorString;
							// error-text of last failed command (Beam-generated, 
							// not from server)
	uint32 mLogType;
							// log-type can be BmLogPop or BmLogSmtp
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
	class BmDotstuffDecoder
		-	
\*------------------------------------------------------------------------------*/
class BmDotstuffDecoder : public BmMemFilter {
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
class BmDotstuffEncoder : public BmMemFilter {
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
class BmNetIBuf : public BmMemIBuf {
	typedef BmMemIBuf inherited;

public:
	BmNetIBuf( BmNetJobModel* job);
	
	// overrides of BmMemIBuf base:
	uint32 Read( char* data, uint32 reqLen);
	bool IsAtEnd();

	// getters:
	BNetEndpoint* Connection()				{ return mJob 
																	? mJob->Connection() 
																	: NULL; }

protected:
	BmNetJobModel* mJob;
	
};


/*------------------------------------------------------------------------------*\
	class BmNetOBuf
		-	
\*------------------------------------------------------------------------------*/
class BmNetOBuf : public BmMemOBuf {
	typedef BmMemOBuf inherited;

public:
	BmNetOBuf( BmNetJobModel* job);

	// overrides/overloads of BmMemOBuf base:
	uint32 Write( const char* data, uint32 len);
	uint32 Write( BmMemIBuf* input, uint32 blockSize);

	// getters:
	BNetEndpoint* Connection()				{ return mJob 
																	? mJob->Connection() 
																	: NULL; }

	// setters:
	inline void DoUpdate( bool b)			{ mUpdate = b; }

protected:
	BmNetJobModel* mJob;
	bool mUpdate;

};
							 

#endif
