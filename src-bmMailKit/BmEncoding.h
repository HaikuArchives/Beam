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

#include "BmString.h"
#include "BmMemIO.h"

/*------------------------------------------------------------------------------*\
	BmEncoding 
\*------------------------------------------------------------------------------*/
namespace BmEncoding {
	
	struct BmEncodingPair {
		const char* charset;
		const uint32 encoding;
	};
	extern BmEncodingPair BM_Encodings[];

	const uint32 BM_UTF8_CONVERSION = 0xFF;
	const uint32 BM_UNKNOWN_ENCODING = 0xFFFF;

	uint32 CharsetToEncoding( const BmString& charset);
	BmString EncodingToCharset( const uint32 encoding);

	void ConvertToUTF8( uint32 srcEncoding, const BmString& src, BmString& dest);
	void ConvertFromUTF8( uint32 destEncoding, const BmString& src, BmString& dest);

	void Encode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord=false);
	void Decode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord=false);

	BmString ConvertHeaderPartToUTF8( const BmString& headerPart, uint32 defaultEncoding);
	BmString ConvertUTF8ToHeaderPart( const BmString& utf8text, uint32 encoding,
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
	BmUtf8Decoder( BmMemIBuf* input, uint32 destEncoding, 
						uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	uint32 mDestEncoding;
};

/*------------------------------------------------------------------------------*\
	class BmUtf8Encoder
		-	
\*------------------------------------------------------------------------------*/
class BmUtf8Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmUtf8Encoder( BmMemIBuf* input, uint32 srcEncoding, 
						uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	uint32 mSrcEncoding;
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

