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

#include <list>

#include "BmString.h"

/*------------------------------------------------------------------------------*\
	class BmMemIBuf
		-	
\*------------------------------------------------------------------------------*/
class BmMemIBuf {
public:
	virtual ~BmMemIBuf()						{}
	virtual uint32 Read( char* data, uint32 reqLen) = 0;
	virtual BmString Peek( uint32 len, int32 offsetToCurr) = 0;
	virtual bool IsAtEnd() = 0;
};

/*------------------------------------------------------------------------------*\
	class BmMemOBuf
		-	
\*------------------------------------------------------------------------------*/
class BmMemOBuf {
public:
	virtual ~BmMemOBuf()						{}
	virtual uint32 Write( const char* data, uint32 dataLen) = 0;
};

/*------------------------------------------------------------------------------*\
	class BmMemFilter
		-	
\*------------------------------------------------------------------------------*/
class BmMemFilter : public virtual BmMemIBuf {
	typedef BmMemIBuf inherited;

	static const uint32 nMaxBufferFixupSize = 256;

public:
	BmMemFilter( BmMemIBuf& input, uint32 blockSize=65536);
	~BmMemFilter();

	// overrides of BmMemIBuf:
	uint32 Read( char* data, uint32 reqLen);
	bool IsAtEnd();

protected:
	struct BmMemBlock {
		inline BmMemBlock( uint32 sz, uint32 safety)
			:	data( new char [sz])
			,	currPos( 0)
			,	size( sz-safety)				{}
		inline ~BmMemBlock()					{ delete [] data; }
		char* data;
		uint32 currPos;
		uint32 size;
	};
	typedef list< BmMemBlock*> BmMemBlockList;

	// native methods:
	virtual uint32 DetermineBufferFixup( BmMemIBuf& input)	{ return 0; }
	virtual void DoFilter( const char* srcBuf, uint32& srcLen, 
								  char* destBuf, uint32& destLen) = 0;
	
	BmMemIBuf& mInput;
	BmMemBlockList mBlockList;
	uint32 mBlockSize;
};

/*------------------------------------------------------------------------------*\
	class BmStringIBuf
		-	
\*------------------------------------------------------------------------------*/
class BmStringIBuf : public virtual BmMemIBuf {
	typedef BmMemIBuf inherited;

public:
	BmStringIBuf( const char* str, int32 len=-1);
	BmStringIBuf( const BmString& str);

	// overrides of BmMemIBuf base:
	uint32 Read( char* data, uint32 reqLen);
	BmString Peek( uint32 len, int32 offsetToCurr);
	inline bool IsAtEnd()					{ return mCurrPos == mSize; }

private:
	const char* mBuf;
	uint32 mCurrPos;
	uint32 mSize;

	// Hide copy-constructor and assignment:
	BmStringIBuf( const BmStringIBuf&);
	BmStringIBuf operator=( const BmStringIBuf&);
};

/*------------------------------------------------------------------------------*\
	class BmStringOBuf
		-	
\*------------------------------------------------------------------------------*/
class BmStringOBuf : public virtual BmMemOBuf {
	typedef BmMemOBuf inherited;

public:
	BmStringOBuf( uint32 startLen, float growFactor=1.2);
	uint32 Write( const char* data, uint32 len);
	uint32 Write( const BmString& data)	{ return Write( data.String(), data.Length()); }
	BmString& TheString();
	inline bool HasData() const 			{ return mBuf!=NULL; }
	inline uint32 CurrPos() const 		{ return mCurrPos; }
	inline char ByteAt( uint32 pos) const { return (!mBuf||pos<0||pos>=mCurrPos) ? 0 : mBuf[pos]; }

	BmStringOBuf 		&operator<<(const char *);
	BmStringOBuf 		&operator<<(const BmString &);

private:
	uint32 mBufLen;
	float mGrowFactor;
	char* mBuf;
	uint32 mCurrPos;
	BmString mStr;

	// Hide copy-constructor and assignment:
	BmStringOBuf( const BmStringOBuf&);
	BmStringOBuf operator=( const BmStringOBuf&);
};

#endif
