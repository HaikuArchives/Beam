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

#include <assert.h>

#include <algorithm>

#include "BmMemIO.h"

/********************************************************************************\
	BmMemFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMemFilter()
		-	constructor
\*------------------------------------------------------------------------------*/
BmMemFilter::BmMemFilter( BmMemIBuf* input, uint32 blockSize, bool immediatePassOn)
	:	mInput( input)
	,	mBuf( new char [blockSize])
	,	mCurrPos( 0)
	,	mCurrSize( 0)
	,	mBlockSize( blockSize)
	,	mSrcCount( 0)
	,	mDestCount( 0)
	,	mIsFinalized( false)
	,	mHadError( false)
	,	mEndReached( false)
	,	mImmediatePassOnMode( immediatePassOn)
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
	Reset()
		-	
\*------------------------------------------------------------------------------*/
void BmMemFilter::Reset( BmMemIBuf* input) {
	mCurrPos = mCurrSize = mSrcCount = mDestCount = 0;
	mIsFinalized = false;
	mHadError = false;
	mEndReached = false;
	if (input)
		mInput = input;
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMemFilter::Read( char* data, uint32 reqLen) {
	const uint32 minSize=10;
							// Minimum size of destination-buffer that we need.
							// The minSize must be larger than the biggest destination-
							// size of any single source-byte. At the moment, the
							// biggest possible destination-size of a single source-byte
							// is produced by the UTF8-encoder, which may blow up a single
							// character to 5 bytes (or less, of course).
	uint32 readLen = 0;
	uint32 srcLen;
	uint32 destLen;
	bool tooSmall = false;
	assert( mInput);
	while( !mHadError && !mEndReached && readLen < reqLen-minSize) {
		if (mCurrPos==mCurrSize || tooSmall) {
			// block is empty or too small, we need to fetch more data:
			if (mInput->IsAtEnd()) {
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
			mCurrSize += mInput->Read( mBuf+srcLen, mBlockSize-srcLen);
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
			if (mImmediatePassOnMode && readLen)
				break;
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
	assert( mInput);
	return mHadError || mEndReached
			 || (mCurrPos==mCurrSize && mInput->IsAtEnd() && mIsFinalized);
}



/********************************************************************************\
	BmStringIBuf
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmStringIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::BmStringIBuf( const char* str, int32 len)
	:	mIndex( 0)
{
	AddBuffer( str, len);
}

/*------------------------------------------------------------------------------*\
	BmStringIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::BmStringIBuf( const BmString& str)
	:	mIndex( 0)
{
	AddBuffer( str);
}

/*------------------------------------------------------------------------------*\
	AddBuffer( str, len)
		-	
\*------------------------------------------------------------------------------*/
void BmStringIBuf::AddBuffer( const char* str, int32 len) {
	mBufInfo.push_back( BufInfo( str, len<0 ? strlen( str) : len));
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::Read( char* data, uint32 reqLen) {
	uint32 readLen = 0;
	while( readLen < reqLen && !IsAtEnd()) {
		uint32 size = min( reqLen-readLen, mBufInfo[mIndex].size-mBufInfo[mIndex].currPos);
		memcpy( data+readLen, mBufInfo[mIndex].buf+mBufInfo[mIndex].currPos, size);
		readLen += size;
		mBufInfo[mIndex].currPos += size;
		if (mBufInfo[mIndex].currPos == mBufInfo[mIndex].size)
			mIndex++;
	}
	return readLen;
}

/*------------------------------------------------------------------------------*\
	IsAtEnd()
		-	
\*------------------------------------------------------------------------------*/
bool BmStringIBuf::IsAtEnd() {
	while (mIndex < mBufInfo.size()) {
		if (mBufInfo[mIndex].currPos < mBufInfo[mIndex].size)
			return false;
		mIndex++;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	EndsWithNewline()
		-	
\*------------------------------------------------------------------------------*/
bool BmStringIBuf::EndsWithNewline() {
	uint32 lst = mBufInfo.size()-1;
	return lst && mBufInfo[lst].size 
			 && mBufInfo[lst].buf[mBufInfo[lst].size-1] == '\n';
}

/*------------------------------------------------------------------------------*\
	Size()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::Size() const {
	uint32 sz = 0;
	for( uint32 i=0; i<mBufInfo.size(); ++i)
		sz += mBufInfo[i].size;
	return sz;
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
uint32 BmStringOBuf::Write( BmMemIBuf* input, uint32 blockSize) {
	uint32 writeLen=0;
	uint32 len;
	while( input && !input->IsAtEnd()) {
		if (!GrowBufferToFit( blockSize))
			break;
		len = input->Read( mBuf+mCurrPos, blockSize);
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

