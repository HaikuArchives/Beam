/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <assert.h>
#include <cstring>
#include <stdlib.h>
#include <new>

#include "BmMemIO.h"

/********************************************************************************\
	BmMemFilter
\********************************************************************************/

const uint32 BmMemFilter::nBlockSize = 32768;
const char* BmMemFilter::nTagImmediatePassOn = "<ImmPassOn>";

/*------------------------------------------------------------------------------*\
	BmMemFilter()
		-	constructor
\*------------------------------------------------------------------------------*/
BmMemFilter::BmMemFilter( BmMemIBuf* input, uint32 blockSize, 
								  const BmString& tags)
	:	mInput( input)
	,	mBuf( new char [blockSize])
	,	mCurrPos( 0)
	,	mCurrSize( 0)
	,	mBlockSize( blockSize)
	,	mSrcCount( 0)
	,	mDestCount( 0)
	,	mTags( tags)
	,	mIsFinalized( false)
	,	mHadError( false)
	,	mEndReached( false)
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
	mStatusText.Truncate(0);
	if (input)
		mInput = input;
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMemFilter::Read( char* data, uint32 reqLen) {
	uint32 readLen = 0;
	uint32 srcLen;
	uint32 destLen;
	bool tooSmall = false;
	assert( mInput);
	while( !mHadError && !mEndReached && readLen < reqLen) {
		if (mCurrPos==mCurrSize || tooSmall) {
			// block is empty or too small, we need to fetch more data:
			if (!tooSmall && mInput->IsAtEnd()) {
				// There is no more input data in our input-MemIO, 
				// Having reached the end of our input,
				// we just have to finalize the filter-output:
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
			// ...and (re-)fill the buffer from our input-stream:
			mCurrSize += mInput->Read( mBuf+srcLen, mBlockSize-srcLen);
		}
		srcLen = max_c( (uint32)0, (uint32)(mCurrSize - mCurrPos));
		if (srcLen) {
			// actually filter one buffer-block:
			destLen = reqLen-readLen;
			Filter( mBuf+mCurrPos, srcLen, data+readLen, destLen);
			mCurrPos += srcLen;
			mSrcCount += srcLen;
			readLen += destLen;
			if (!srcLen) {
				if (tooSmall)
					// destination buffer is too small to continue, we break:
					break;
				tooSmall = true;
			} else
				tooSmall = false;
			if (IsTagSet(nTagImmediatePassOn) && readLen)
				// in immediate-pass-on mode, we pass-on data as soon as we got 
				// some:
				break;
		} else
			tooSmall = false;
	}
	mDestCount += readLen;
	return readLen;
}

/*------------------------------------------------------------------------------*\
	AddStatusText()
		-	
\*------------------------------------------------------------------------------*/
void BmMemFilter::AddStatusText( const BmString& text)
{
	if (mStatusText.Length())
		mStatusText << "\n";
	mStatusText << text;
}

/*------------------------------------------------------------------------------*\
	IsTagSet()
		-	
\*------------------------------------------------------------------------------*/
bool BmMemFilter::IsTagSet( const char* tag) {
	return mTags.FindFirst(tag) >= B_OK;
}

/*------------------------------------------------------------------------------*\
	SetTag()
		-	
\*------------------------------------------------------------------------------*/
bool BmMemFilter::SetTag( const char* tag, bool newVal) {
	if ((mTags.FindFirst(tag) >= B_OK) == newVal)
		// no change
		return false;
	if (newVal)
		mTags << tag;
	else
		mTags.RemoveFirst(tag);
	return true;
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMemFilter::Stop() {
	mEndReached = true;
	if (mInput)
		mInput->Stop();
}



/********************************************************************************\
	BmStringIBuf
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmStringIBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::BmStringIBuf()
	:	mIndex( 0)
{
}

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
	~BmStringIBuf()
		-	destructor
\*------------------------------------------------------------------------------*/
BmStringIBuf::~BmStringIBuf() {
	for( int i=0; i<mBufInfo.CountItems(); ++i)
		delete static_cast< BufInfo*>( mBufInfo.ItemAt(i));
}

/*------------------------------------------------------------------------------*\
	AddBuffer( str, len)
		-	
\*------------------------------------------------------------------------------*/
void BmStringIBuf::AddBuffer( const char* str, int32 len) {
	mBufInfo.AddItem( new BufInfo( str, len<0 ? strlen( str) : len));
}

/*------------------------------------------------------------------------------*\
	FirstBuf()
		-	
\*------------------------------------------------------------------------------*/
const char* BmStringIBuf::FirstBuf() const {
	BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(0));
	return bufInfo ? bufInfo->buf : 0;
}

/*------------------------------------------------------------------------------*\
	FirstSize()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::FirstSize() const {
	BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(0));
	return bufInfo ? bufInfo->size : 0;
}

/*------------------------------------------------------------------------------*\
	Read()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::Read( char* data, uint32 reqLen) {
	uint32 readLen = 0;
	while( readLen < reqLen && !IsAtEnd()) {
		BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(mIndex));
		if (!bufInfo)
			break;
		uint32 size = min_c( reqLen-readLen, bufInfo->size - bufInfo->currPos);
		memcpy( data+readLen, bufInfo->buf + bufInfo->currPos, size);
		readLen += size;
		bufInfo->currPos += size;
		if (bufInfo->currPos == bufInfo->size)
			mIndex++;
	}
	return readLen;
}

/*------------------------------------------------------------------------------*\
	IsAtEnd()
		-	
\*------------------------------------------------------------------------------*/
bool BmStringIBuf::IsAtEnd() {
	uint32 count = mBufInfo.CountItems();
	while (mIndex < count) {
		BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(mIndex));
		if (!bufInfo)
			break;
		if (bufInfo->currPos < bufInfo->size)
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
	uint32 lst = mBufInfo.CountItems()-1;
	if (lst) {
		BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(lst));
		if (bufInfo)
			return bufInfo->buf[bufInfo->size-1] == '\n';
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	Size()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmStringIBuf::Size() const {
	uint32 sz = 0;
	uint32 count = mBufInfo.CountItems();
	for( uint32 i=0; i<count; ++i) {
		BufInfo* bufInfo = static_cast< BufInfo*>( mBufInfo.ItemAt(i));
		if (!bufInfo)
			break;
		sz += bufInfo->size;
	}
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
	,	mGrowFactor( max_c((float)1.0, growFactor))
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
	Reset()
		-	reset to empty state
\*------------------------------------------------------------------------------*/
void BmStringOBuf::Reset() {
	mCurrPos = 0;
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
			mBufLen = (uint32)max_c( mGrowFactor*mBufLen, mGrowFactor*(mCurrPos+len));
		} else
			mBufLen = (uint32)max_c( mBufLen, mCurrPos+len);
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



/********************************************************************************\
	BmMemBufConsumer
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMemBufConsumer()
		-	constructor
\*------------------------------------------------------------------------------*/
BmMemBufConsumer::BmMemBufConsumer( uint32 bufSize)
	:	mBuf( new char [bufSize])
	,	mBufSize( bufSize)
{
}

/*------------------------------------------------------------------------------*\
	~BmMemBufConsumer()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMemBufConsumer::~BmMemBufConsumer() {
	delete [] mBuf;
}

/*------------------------------------------------------------------------------*\
	Consume( input)
		-	reads adds all data from given BmMemIBuf and (potentially) passes
			it through the given functor
\*------------------------------------------------------------------------------*/
void BmMemBufConsumer::Consume( BmMemIBuf* input, Functor* functor) {
	uint32 len;
	while( input && !input->IsAtEnd()) {
		len = input->Read( mBuf, mBufSize);
		if (len>0 && functor)
			if ((*functor)( mBuf, len) != B_OK)
				break;
	}
}




/********************************************************************************\
	BmRingBuf
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmRingBuf()
		-	constructor
\*------------------------------------------------------------------------------*/
BmRingBuf::BmRingBuf( uint32 startLen, float growFactor)
	:	mBufLen( startLen)
	,	mBuf( NULL)
	,	mGrowFactor( max_c((float)2.0, growFactor))
	,	mCurrFront( 0)
	,	mCurrTail( 0)
{
	mBuf = (char*)malloc( mBufLen);
}

/*------------------------------------------------------------------------------*\
	~BmRingBuf()
		-	destructor
\*------------------------------------------------------------------------------*/
BmRingBuf::~BmRingBuf() {
	if (mBuf)
		free( mBuf);
}

/*------------------------------------------------------------------------------*\
	Reset()
		-	reset to empty state
\*------------------------------------------------------------------------------*/
void BmRingBuf::Reset() {
	mCurrFront = 0;
	mCurrTail = 0;
}

/*------------------------------------------------------------------------------*\
	Length()
		-	returns number of bytes in buffer
\*------------------------------------------------------------------------------*/
int32 BmRingBuf::Length() const {
	if (mCurrFront <= mCurrTail)
		return mCurrTail - mCurrFront;
	else
		return mBufLen + mCurrTail - mCurrFront;
}

/*------------------------------------------------------------------------------*\
	Put()
		-	adds given data to end buffer
\*------------------------------------------------------------------------------*/
void BmRingBuf::Put( const char* data, uint32 len) {
	if (!mBuf || mBufLen <= Length()+len) {
		// need more space:
		int32 oldLen = mBufLen;
		mBufLen = (uint32)max_c( mGrowFactor*mBufLen, mGrowFactor*(Length()+len));
		mBuf = (char*)realloc( mBuf, mBufLen);
		if (mCurrTail < mCurrFront && mCurrTail) {
			// re-join wrapped part of buffer with its front:
			memcpy( mBuf+oldLen, mBuf, mCurrTail);
			mCurrTail += oldLen;
		}
		if (!mBuf)
			throw bad_alloc();
	}
	for( uint32 i=0; i<len; ++i) {
		if (mCurrTail == mBufLen)
			mCurrTail = 0;						// wrap
		mBuf[mCurrTail++] = *data++;
	}
}

/*------------------------------------------------------------------------------*\
	operator BmString()
		-	return complete buffer as string (and removes it)
\*------------------------------------------------------------------------------*/
BmRingBuf::operator BmString() {
	BmString str;
	if (mCurrFront == mCurrTail)			// buffer is empty
		return str;
	if (mCurrFront == mBufLen)
		mCurrFront = 0;						// wrap

	if (mCurrFront <= mCurrTail)
		str.SetTo(mBuf + mCurrFront, mCurrTail - mCurrFront);
	else {
		str.SetTo(mBuf + mCurrFront, mBufLen - mCurrFront);
		str.Append(mBuf, mCurrTail);
	}
	Reset();
	return str;
}

/*------------------------------------------------------------------------------*\
	Get()
		-	fetches data from front of buffer (and removes it)
\*------------------------------------------------------------------------------*/
char BmRingBuf::Get() {
	if (mCurrFront == mCurrTail)			// buffer is empty
		return '\0';
	if (mCurrFront == mBufLen)
		mCurrFront = 0;						// wrap
	return mBuf[mCurrFront++];
}

/*------------------------------------------------------------------------------*\
	PeekFront()
		-	return data from front of buffer (but does not remove it)
\*------------------------------------------------------------------------------*/
char BmRingBuf::PeekFront() const {
	if (mCurrFront == mCurrTail)			// buffer is empty
		return '\0';
	if (mCurrFront == mBufLen)
		return mBuf[0];
	else
		return mBuf[mCurrFront];
}

/*------------------------------------------------------------------------------*\
	PeekTail()
		-	returns the last character in buffer (but does not remove it)
\*------------------------------------------------------------------------------*/
char BmRingBuf::PeekTail() const {
	if (mCurrFront == mCurrTail)
		return '\0';
	if (mCurrTail == 0)
		return mBuf[mBufLen-1];
	else
		return mBuf[mCurrTail-1];
}

/*------------------------------------------------------------------------------*\
	<< operators:
\*------------------------------------------------------------------------------*/
BmRingBuf&
BmRingBuf::operator<<(const char c)
{
	Put( &c, 1);
	return *this;	
}

BmRingBuf&
BmRingBuf::operator<<(const char *str)
{
	if (str)
		Put( str, strlen(str));
	return *this;	
}


BmRingBuf&
BmRingBuf::operator<<(const BmString &string)
{
	Put( string.String(), string.Length());
	return *this;
}
