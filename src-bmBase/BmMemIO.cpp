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
	,	mBuf( new char [blockSize])
	,	mCurrPos( 0)
	,	mCurrSize( 0)
	,	mBlockSize( blockSize)
	,	mSrcCount( 0)
	,	mDestCount( 0)
	,	mIsFinalized( false)
	,	mHadError( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmMemFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMemFilter::~BmMemFilter() {
	delete [] mBuf;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMemFilter::Read( char* data, uint32 reqLen) {
	const uint32 minSize=5;
	uint32 readLen = 0;
	uint32 srcLen;
	uint32 destLen;
	bool tooSmall = false;
	while( !mHadError && readLen < reqLen-minSize) {
		if (mCurrPos==mCurrSize || tooSmall) {
			// block is empty, we need to fetch more data:
			if (mInput.IsAtEnd()) {
				// there is no more input data in our input-MemIO, 
				// this means that we have reached the end.
				// We just have to finalize the filter-output:
				destLen = reqLen-readLen;
				Finalize( data+readLen, destLen);
				readLen += destLen;
					break;
			}
			// we "move-up" the remaining part of the buffer...
			srcLen = mCurrSize-mCurrPos;
			memmove( mBuf, mBuf+mCurrPos, srcLen);
			mCurrPos = 0;
			mCurrSize = srcLen;
			tooSmall = false;
			// ...and (re-)fill the buffer from our input-stream:
			mCurrSize += mInput.Read( mBuf+srcLen, mBlockSize-srcLen);
		}
		srcLen = max( (uint32)0, mCurrSize - mCurrPos);
		if (srcLen) {
			// actually filter one buffer-block:
			destLen = reqLen-readLen;
			Filter( mBuf+mCurrPos, srcLen, data+readLen, destLen);
			if (!srcLen)
				tooSmall = true;
			mCurrPos += srcLen;
			mSrcCount += srcLen;
			readLen += destLen;
		}
	}
	mDestCount += readLen;
	return readLen;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMemFilter::IsAtEnd() {
	return mHadError || (mCurrPos==mCurrSize && mInput.IsAtEnd() && mIsFinalized);
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
	~BmStringOBuf()
		-	destructor
\*------------------------------------------------------------------------------*/
BmStringOBuf::~BmStringOBuf() {
	if (mBuf)
		mStr.UnlockBuffer( mCurrPos);
}

/*------------------------------------------------------------------------------*\
	GrowBufferToFit( len)
		-	makes sure that the buffer is big enough to write the given number
			of bytes.
\*------------------------------------------------------------------------------*/
bool BmStringOBuf::GrowBufferToFit( uint32 len) {
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
	return true;
}

/*------------------------------------------------------------------------------*\
	Write( data, len)
		-	adds given data to end of string
\*------------------------------------------------------------------------------*/
uint32 BmStringOBuf::Write( const char* data, uint32 len) {
	if (!len || !GrowBufferToFit( len))
		return 0;
	memcpy( mBuf+mCurrPos, data, len);
	mCurrPos += len;
	return len;
}

/*------------------------------------------------------------------------------*\
	Write( input)
		-	adds all data from given BmMemIBuf input to end of string
\*------------------------------------------------------------------------------*/
uint32 BmStringOBuf::Write( BmMemIBuf& input, uint32 blockSize) {
	uint32 writeLen=0;
	uint32 len;
	while( !input.IsAtEnd()) {
		if (!GrowBufferToFit( blockSize))
			break;
		len = input.Read( mBuf+mCurrPos, blockSize);
		writeLen += len;
		mCurrPos += len;
	}
	return writeLen;
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

