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
	
	bool NeedsEncoding( const BmString& charsetString);
	bool IsCompatibleWithText( const BmString& s);
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableDecoder
		-	
\*------------------------------------------------------------------------------*/
class BmQuotedPrintableDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

	BmQuotedPrintableDecoder( BmMemIBuf& input, bool isEncodedWord=false, 
									  uint32 blockSize=65536);

protected:
	// overrides of BmMailFilter base:
	uint32 DetermineBufferFixup( BmMemIBuf& input);
	void DoFilter( const char* srcBuf, uint32& srcLen, 
						char* destBuf, uint32& destLen);

	bool mIsEncodedWord;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class BmQuotedPrintableEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

	BmQuotedPrintableEncoder( BmMemIBuf& input, bool isEncodedWord=false, 
									  uint32 blockSize=65536);

protected:
	// overrides of BmMailFilter base:
	void DoFilter( const char* srcBuf, uint32& srcLen, 
						char* destBuf, uint32& destLen);

	bool mIsEncodedWord;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Decoder
		-	
\*------------------------------------------------------------------------------*/
class BmBase64Decoder : public BmMemFilter {
	typedef BmMemFilter inherited;

	BmBase64Decoder( BmMemIBuf& input, uint32 blockSize=65536)
		:	inherited( input, blockSize)
		,	mIndex( 0)							{}

protected:
	// overrides of BmMailFilter base:
	uint32 DetermineBufferFixup( BmMemIBuf& input);
	void DoFilter( const char* srcBuf, uint32& srcLen, 
						char* destBuf, uint32& destLen);

	uint32 mIndex;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Encoder
		-	
\*------------------------------------------------------------------------------*/
class BmBase64Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

	BmBase64Encoder( BmMemIBuf& input, uint32 blockSize=65536)
		:	inherited( input, blockSize)
		,	mCurrLineLen( 0)					{}

protected:
	// overrides of BmMailFilter base:
	void DoFilter( const char* srcBuf, uint32& srcLen, 
						char* destBuf, uint32& destLen);
	
	int mCurrLineLen;
};

#endif

