/*
	BmMemIO.h
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


#ifndef _BmMemIO_h
#define _BmMemIO_h

#include <List.h>

#include "BmBase.h"
#include "BmString.h"

/*------------------------------------------------------------------------------*\
	class BmMemIBuf
		-	an interface representing a memory input buffer, i.e. a stream that 
			can be read from.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmMemIBuf {
public:
	virtual ~BmMemIBuf()						{}
	virtual uint32 Read( char* data, uint32 reqLen) = 0;
	virtual bool IsAtEnd() = 0;
};

/*------------------------------------------------------------------------------*\
	class BmMemFilter
		-	an interface representing a memory filter, i.e. a class that reads
			from a memory stream, filters the data and then allows other classes
			to read the filtered data.
			BmMemFilters and BmMemIBufs can be connected to form filter chains.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmMemFilter : public BmMemIBuf {
	typedef BmMemIBuf inherited;
	
public:
	BmMemFilter( BmMemIBuf* input, uint32 blockSize=65536, 
					 const BmString& tags=BM_DEFAULT_STRING);
	~BmMemFilter();

	// native methods:
	virtual void Reset( BmMemIBuf* input=NULL);

	// overrides of BmMemIBuf:
	uint32 Read( char* data, uint32 reqLen);
	bool IsAtEnd();

	// getters
	bool HadError() const					{ return mHadError; }
	uint32 SrcCount() const					{ return mSrcCount; }
	uint32 DestCount() const				{ return mDestCount; }

	static IMPEXPBMBASE const uint32 nBlockSize;
	static IMPEXPBMBASE const char* nTagImmediatePassOn;
							// indicates that instead of trying to fill its buffer, the
							// filter will process every block of data it gets and 
							// immediately pass it on to its output buffer. 
							// Network-related filters usually behave this way so that 
							// they process the data while more bytes are travelling 
							// through the wire.

protected:
	// native methods:
	virtual void Filter( const char* srcBuf, uint32& srcLen, 
								char* destBuf, uint32& destLen) = 0;
	virtual void Finalize( char* , uint32& destLen) 
													{ destLen=0; mIsFinalized = true; }
	//
	bool SetTag( const char* tag, bool newVal);
	bool IsTagSet( const char* tag);
	
	BmMemIBuf* mInput;
	char* mBuf;
	uint32 mCurrPos;
	uint32 mCurrSize;
	uint32 mBlockSize;
	uint32 mSrcCount;
	uint32 mDestCount;
	BmString mTags;
							// tags influencing the behaviour of the filter
							// (subclasses may add new tags for themselves)
	bool mIsFinalized;
							// indicates that Finalize() has completed
	bool mHadError;
							// indicates that an error has occurred, no more
							// processing will be done by this filter
	bool mEndReached;
							// the filter has reached a byte-combination that indicates
							// the end of data (e.g. "\r\n.\r\n" in dotstuffed encoding
							// the remaining data will be ignored
};

/*------------------------------------------------------------------------------*\
	class BmStringIBuf
		-	an implementation of BmMemIBuf which allows the use of one or more
			strings (char* or BmString) as a single input-stream.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmStringIBuf : public BmMemIBuf {
	typedef BmMemIBuf inherited;

public:
	BmStringIBuf();
	BmStringIBuf( const char* str, int32 len=-1);
	BmStringIBuf( const BmString& str);
	~BmStringIBuf();

	// native methods:
	void AddBuffer( const char* str, int32 len=-1);
	inline void AddBuffer( const BmString& str)
													{ AddBuffer( str.String(), 
																	 str.Length()); }

	// overrides of BmMemIBuf base:
	uint32 Read( char* data, uint32 reqLen);
	bool IsAtEnd();
	bool EndsWithNewline();

	// getters:
	uint32 Size() const;
	const char* FirstBuf() const;
	uint32 FirstSize() const;

private:
	struct BufInfo {
		const char* buf;
		uint32 currPos;
		uint32 size;
		BufInfo()
			: buf( 0), currPos( 0), size( 0)		{}
		BufInfo( const char* b, uint32 s)
			: buf( b), currPos( 0), size( s)		{}
	};
	BList mBufInfo;
	uint32 mIndex;

	// Hide copy-constructor and assignment:
	BmStringIBuf( const BmStringIBuf&);
	BmStringIBuf operator=( const BmStringIBuf&);
};

/*------------------------------------------------------------------------------*\
	class BmStringOBuf
		-	a class which represents a dynamic string-buffer, i.e. the buffer
			grows (in a more or less efficient manner) as data is written to it.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmStringOBuf {

public:
	BmStringOBuf( uint32 startLen, float growFactor=1.5);
	~BmStringOBuf();

	// native methods:
	BmString& TheString();
	inline const char* Buffer() const	{ return mBuf; }
	inline bool HasData() const			{ return mBuf!=NULL; }
	inline char ByteAt( uint32 pos) const
													{ return (!mBuf||pos<0||pos>=mCurrPos) 
																	? 0 
																	: mBuf[pos];
													}
	void Reset();

	uint32 Write( const char* data, uint32 len);
	uint32 Write( BmMemIBuf* input, uint32 blockSize=BmMemFilter::nBlockSize);
	uint32 Write( const BmString& data)	{ return Write( data.String(), 
																		 data.Length()); }
	
	// getters:
	inline uint32 CurrPos() const			{ return mCurrPos; }

	BmStringOBuf 		&operator<<(const char *);
	BmStringOBuf 		&operator<<(const BmString &);

private:
	bool GrowBufferToFit( uint32 len);

	uint32 mBufLen;
	float mGrowFactor;
	char* mBuf;
	uint32 mCurrPos;
	BmString mStr;

	// Hide copy-constructor and assignment:
	BmStringOBuf( const BmStringOBuf&);
	BmStringOBuf operator=( const BmStringOBuf&);
};

/*------------------------------------------------------------------------------*\
	class BmMemBufConsumer
		-	a class that "consumes" a memory-stream, i.e. it empties the stream,
			potentially feeding the data through a consuming functor.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmMemBufConsumer {
	
public:
	BmMemBufConsumer( uint32 bufSize);
	~BmMemBufConsumer();

	// Functor which is called for each buffer that is to be consumed
	struct Functor {
		virtual ~Functor() 					{}
		virtual status_t operator() (char* buf, uint32 bufLen) = 0;
	};

	void Consume( BmMemIBuf* input, Functor* functor=NULL);
	
private:
	char* mBuf;
	uint32 mBufSize;

	// Hide copy-constructor and assignment:
	BmMemBufConsumer( const BmMemBufConsumer&);
	BmMemBufConsumer operator=( const BmMemBufConsumer&);
};

/*------------------------------------------------------------------------------*\
	class BmRingBuf
		-	a simple ring buffer
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmRingBuf {
	friend class MemIoTest;

public:
	BmRingBuf( uint32 startLen, float growFactor=1.5);
	~BmRingBuf();

	// native methods:
	void Put( const char* data, uint32 len);
	char Get();
	char PeekFront() const;
	char PeekTail() const;
	int32 Length() const;
	void Reset();
	
	BmRingBuf 		&operator<<(const char);
	BmRingBuf 		&operator<<(const char *);
	BmRingBuf 		&operator<<(const BmString &);

private:

	uint32 mBufLen;
	char* mBuf;
	float mGrowFactor;
	uint32 mCurrFront;
	uint32 mCurrTail;

	// Hide copy-constructor and assignment:
	BmRingBuf( const BmRingBuf&);
	BmRingBuf operator=( const BmRingBuf&);
};

#endif
