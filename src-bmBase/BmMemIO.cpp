/*
	BmMemIO.cpp

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

#include "BmMemIO.h"

/********************************************************************************\
	BmMemFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMemFilter()
		-	constructor
\*------------------------------------------------------------------------------*/
BmMemFilter::BmMemFilter( BmMemIBuf& input, uint32 blockSize)
	:	mInput( input)
	,	mBlockSize( blockSize)
{
}

/*------------------------------------------------------------------------------*\
	~BmMemFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMemFilter::~BmMemFilter() {
	BmMemBlockList::iterator iter;
	for( iter = mBlockList.begin(); iter != mBlockList.end(); ++iter)
		delete *iter;
	mBlockList.clear();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMemFilter::Read( char* data, uint32 reqLen) {
	uint32 readLen = 0;
	BmMemBlock* block = NULL;
	while( readLen<reqLen) {
		if (mBlockList.empty()) {
			if (mInput.IsAtEnd())
				return readLen;
			block = new BmMemBlock( mBlockSize, nMaxBufferFixupSize);
			uint32 readSize = mInput.Read( block->data, block->size);
			if (readSize < block->size)
				block->size	= readSize;
			uint32 fixupSize = DetermineBufferFixup( mInput);
			assert( fixupSize<=nMaxBufferFixupSize);
			mInput.Read( block->data+block->size, fixupSize);
			block->size += fixupSize;
			mBlockList.push_back( block);
		} else
			block = mBlockList.front();
		uint32 srcLen = max( (uint32)0, block->size - block->currPos);
		uint32 destLen = reqLen-readLen;
		DoFilter( block->data+block->currPos, srcLen, data+readLen, destLen);
		block->currPos += srcLen;
		if (block->currPos == block->size) {
			mBlockList.pop_front();
			delete block;
		}
		readLen += destLen;
	}
	return readLen;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMemFilter::IsAtEnd() {
	return mBlockList.empty() && mInput.IsAtEnd();
}



/********************************************************************************\
	BmStringIBuf
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmStringIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::BmStringIBuf( const char* str, int32 len)
	:	mBuf( str)
	,	mCurrPos( 0)
	,	mSize( len==-1 ? strlen( str) : len)
{
}

/*------------------------------------------------------------------------------*\
	BmStringIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::BmStringIBuf( const BmString& str)
	:	mBuf( str.String())
	,	mCurrPos( 0)
	,	mSize( str.Length())
{
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::Read( char* data, uint32 reqLen) {
	uint32 size = min( reqLen, mSize-mCurrPos);
	memcpy( data, mBuf+mCurrPos, size);
	mCurrPos += size;
	return size;
}

/*------------------------------------------------------------------------------*\
	Peek()
		-	
\*------------------------------------------------------------------------------*/
BmString BmStringIBuf::Peek( uint32 len, int32 offsetToCurr) {
	return BmString( mBuf+max((uint32)0,mCurrPos+offsetToCurr), len);
}



/********************************************************************************\
	BmStringOBuf
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmStringOBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringOBuf::BmStringOBuf( uint32 startLen, float growFactor)
	:	mBufLen( startLen)
	,	mGrowFactor( max((float)1.0, growFactor))
	,	mBuf( NULL)
	,	mCurrPos( 0)
{
}

/*------------------------------------------------------------------------------*\
	Write( data, len)
		-	adds given data to end of string
\*------------------------------------------------------------------------------*/
uint32 BmStringOBuf::Write( const char* data, uint32 len) {
	if (!len)
		return true;
	if (!mBuf || mCurrPos+len > mBufLen) {
		if (mBuf) {
			mStr.UnlockBuffer( mBufLen);
			mBufLen = (uint32)max( mGrowFactor*mBufLen, mGrowFactor*(mCurrPos+len));
		} else
			mBufLen = (uint32)max( mBufLen, mCurrPos+len);
		mBuf = mStr.LockBuffer( mBufLen);
		if (!mBuf)
			return false;
	}
	memcpy( mBuf+mCurrPos, data, len);
	mCurrPos += len;
	return true;
}

/*------------------------------------------------------------------------------*\
	TheString()
		-	returns the string
\*------------------------------------------------------------------------------*/
BmString& BmStringOBuf::TheString() {
	if (mBuf) {
		mBuf[mCurrPos] = '\0';
		mStr.UnlockBuffer( mCurrPos);
		mBuf = NULL;
	}
	return mStr;
}

/*------------------------------------------------------------------------------*\
	<< operators:
\*------------------------------------------------------------------------------*/
BmStringOBuf&
BmStringOBuf::operator<<(const char *str)
{
	if (str)
		Write( str, strlen(str));
	return *this;	
}


BmStringOBuf&
BmStringOBuf::operator<<(const BmString &string)
{
	Write( string.String(), string.Length());
	return *this;
}

