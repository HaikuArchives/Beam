/*
	BmEncoding.h
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

#ifndef _BmEncoding_h
#define _BmEncoding_h

#include <memory>
#include <map>

#include <iconv.h>

#include "BmString.h"
#include "BmMemIO.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmEncoding 
\*------------------------------------------------------------------------------*/
namespace BmEncoding {
	
	typedef map< BmString, bool> BmCharsetMap;
	extern BmCharsetMap TheCharsetMap;

	extern BmString DefaultCharset;
	
	void InitCharsetMap();

	const char* ConvertFromBeosToLibiconv( uint32 encoding);

	void ConvertToUTF8( const BmString& srcCharset, const BmString& src, 
							  BmString& dest);
	void ConvertFromUTF8( const BmString& destCharset, const BmString& src, 
								 BmString& dest);

	void Encode( BmString encodingStyle, const BmString& src, BmString& dest);
	void Decode( BmString encodingStyle, const BmString& src, BmString& dest);

	BmString ConvertHeaderPartToUTF8( const BmString& headerPart, 
												 const BmString& defaultCharset);
	BmString ConvertUTF8ToHeaderPart( const BmString& utf8text, 
												 const BmString& charset,
												 bool useQuotedPrintableIfNeeded, 
												 int32 fieldLen);
	
	bool NeedsEncoding( const BmString& utf8String);
	bool IsCompatibleWithText( const BmString& s);

	typedef auto_ptr<BmMemFilter> BmMemFilterRef;
	BmMemFilterRef FindDecoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 bool isEncodedWord=false, 
											 uint32 blockSize=BmMemFilter::nBlockSize);
	BmMemFilterRef FindEncoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 uint32 blockSize=BmMemFilter::nBlockSize);

}


/*------------------------------------------------------------------------------*\
	class BmUtf8Decoder
		-	
\*------------------------------------------------------------------------------*/
class BmUtf8Decoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmUtf8Decoder( BmMemIBuf* input, const BmString& destCharset, 
						uint32 blockSize=nBlockSize);
	~BmUtf8Decoder();
	
	// native methods:
	void InitConverter();
	void SetTransliterate( bool transliterate);
	void SetDiscard( bool discard);

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	BmString mDestCharset;
	iconv_t mIconvDescr;
	bool mTransliterate;
	bool mDiscard;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;
};

/*------------------------------------------------------------------------------*\
	class BmUtf8Encoder
		-	
\*------------------------------------------------------------------------------*/
class BmUtf8Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmUtf8Encoder( BmMemIBuf* input, const BmString& srcCharset, 
						uint32 blockSize=nBlockSize);
	~BmUtf8Encoder();

	// native methods:
	void InitConverter();
	void SetTransliterate( bool transliterate);
	void SetDiscard( bool discard);

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	BmString mSrcCharset;
	iconv_t mIconvDescr;
	bool mTransliterate;
	bool mDiscard;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableDecoder
		-	
\*------------------------------------------------------------------------------*/
class BmQuotedPrintableDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQuotedPrintableDecoder( BmMemIBuf* input, bool isEncodedWord=false, 
									  uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	bool mIsEncodedWord;
	int mSpacesThatMayNeedRemoval;
	bool mSoftbreakPending;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmQuotedPrintableEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQuotedPrintableEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	void Queue( const char* chars, uint32 len);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	BmRingBuf mQueuedChars;
	int mSpacesThatMayNeedEncoding;
	int mLastAddedLen;
	int mCurrAddedLen;
	bool mNeedFlush;
	int mKeepLen;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmQpEncodedWordEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQpEncodedWordEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize, 
									int startAtOffset=0,
									const BmString& charset=BM_DEFAULT_STRING);
	virtual ~BmQpEncodedWordEncoder();

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

protected:
	// native methods:
	void InitConverter();
	void EncodeConversionBuf();
	void Queue( const char* chars, uint32 len);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	BmString mEncodedWord;
	BmRingBuf mQueuedChars;
	int mStartAtOffset;
	int mLastAddedLen;
	int mCurrAddedLen;
	bool mNeedFlush;
	int mKeepLen;
	int mCharacterLen;
	//
	BmString mDestCharset;
	iconv_t mIconvDescr;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;

	BmString mConversionBuf;
};

/*------------------------------------------------------------------------------*\
	class BmFoldedLineEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmFoldedLineEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmFoldedLineEncoder( BmMemIBuf* input, int lineLen,
							   uint32 blockSize=nBlockSize, int startAtOffset=0);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	int mStartAtOffset;
	int mMaxLineLen;
	int mLastSpacePos;
	BmRingBuf mQueuedChars;
	bool mNeedFlush;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Decoder
		-	
\*------------------------------------------------------------------------------*/
class BmBase64Decoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBase64Decoder( BmMemIBuf* input, uint32 blockSize=nBlockSize)
		:	inherited( input, blockSize)
		,	mConcat( 0)
		,	mIndex( 0)							{}

	static const int32 nBase64Alphabet[256];

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	uint32 mConcat;
	uint32 mIndex;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Encoder
		-	
\*------------------------------------------------------------------------------*/
class BmBase64Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBase64Encoder( BmMemIBuf* input, uint32 blockSize=nBlockSize)
		:	inherited( input, blockSize)
		,	mConcat( 0)
		,	mIndex( 0)
		,	mCurrLineLen( 0)					{}

	static const char nBase64Alphabet[64];

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	
	uint32 mConcat;
	uint32 mIndex;
	int mCurrLineLen;
};

/*------------------------------------------------------------------------------*\
	class BmLinebreakDecoder
		-	
\*------------------------------------------------------------------------------*/
class BmLinebreakDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmLinebreakDecoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

};

/*------------------------------------------------------------------------------*\
	class BmLinebreakEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmLinebreakEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmLinebreakEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
};

/*------------------------------------------------------------------------------*\
	class BmBinaryDecoder
		-	
\*------------------------------------------------------------------------------*/
class BmBinaryDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBinaryDecoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

};

/*------------------------------------------------------------------------------*\
	class BmBinaryEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmBinaryEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBinaryEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
};

#endif

