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

/*------------------------------------------------------------------------------*\
	BmEncoding 
\*------------------------------------------------------------------------------*/
namespace BmEncoding {
	
	typedef map< BmString, bool> BmCharsetMap;
	extern BmCharsetMap TheCharsetMap;

	extern BmString DefaultCharset;
	
	void InitCharsetMap();

	const char* ConvertFromBeosToLibiconv( uint32 encoding);

	void ConvertToUTF8( const BmString& srcCharset, const BmString& src, BmString& dest);
	void ConvertFromUTF8( const BmString& destCharset, const BmString& src, BmString& dest);

	void Encode( BmString encodingStyle, const BmString& src, BmString& dest, 
					 bool isEncodedWord=false);
	void Decode( BmString encodingStyle, const BmString& src, BmString& dest, 
					 bool isEncodedWord=false);

	BmString ConvertHeaderPartToUTF8( const BmString& headerPart, const BmString& defaultCharset);
	BmString ConvertUTF8ToHeaderPart( const BmString& utf8text, const BmString& charset,
												 bool useQuotedPrintableIfNeeded,
												 bool fold=false, int32 fieldLen=0);
	
	bool NeedsEncoding( const BmString& utf8String);
	bool IsCompatibleWithText( const BmString& s);

	typedef auto_ptr<BmMemFilter> BmMemFilterRef;
	BmMemFilterRef FindDecoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 bool isEncodedWord=false, 
											 uint32 blockSize=BmMemFilter::nBlockSize);
	BmMemFilterRef FindEncoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 bool isEncodedWord=false, 
											 uint32 blockSize=BmMemFilter::nBlockSize);
};


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

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	const BmString& mDestCharset;
	iconv_t mIconvDescr;
	bool mTransliterate;
	bool mDiscard;
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

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	const BmString& mSrcCharset;
	iconv_t mIconvDescr;
	bool mTransliterate;
	bool mDiscard;
	bool mHadToDiscardChars;
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

	bool mIsEncodedWord;
	bool mLastWasLinebreak;
	int mSpacesThatMayNeedRemoval;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmQuotedPrintableEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQuotedPrintableEncoder( BmMemIBuf* input, bool isEncodedWord=false, 
									  uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	bool mIsEncodedWord;
	int mCurrLineLen;
	int mSpacesThatMayNeedEncoding;
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

	bool mLastWasCR;
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

