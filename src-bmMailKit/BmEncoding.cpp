/*
	BmEncoding.cpp
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


#include <cstring>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmUtil.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

#define BM_MAX_LINE_LEN 76
							// as stated in [K. Johnson, p. 149f]

#define HEXDIGIT2CHAR(d) (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0)

#define CHAR2HIGHNIBBLE(c) (((c) > 0x9F ? 'A'-10 : '0')+((c)>>4))
#define CHAR2LOWNIBBLE(c)  ((((c)&0x0F) > 9 ? 'A'-10 : '0')+((c)&0x0F))


BmCharsetMap BmEncoding::TheCharsetMap;

BmString BmEncoding::DefaultCharset = "ISO-8859-15";

static iconv_t ICONV_ERR = (iconv_t)0xFFFFFFFF;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static int HandleOneCharset( unsigned int namescount, const char * const * names,
                             void* data) {
	static const char* rxs[] = {
		"^ISO-",
		"^WINDOWS-",
		"^CP",
		"^KOI8-",
		"^EUC-",
		"^MACROMAN$",
		"^SHIFT-JIS$",
		"^UTF-",
		"^UCS-",
		NULL
	};
	Regexx rx;
	int index=0;
	for( int r=0; rxs[r]!=NULL; ++r) {
		rx.expr( rxs[r]);
		for( uint32 i=0; i<namescount; ++i) {
			rx.str( names[i]);
			if (rx.exec() > 0) {
				index = i;
				goto out;
			}
		}
	}
out:
	for( uint32 i=0; i<namescount; ++i)
		TheCharsetMap[names[i]] = false;
	TheCharsetMap[names[index]] = true;
	return 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::InitCharsetMap() {
	iconvlist( &HandleOneCharset, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const char* BmEncoding::ConvertFromBeosToLibiconv( uint32 encoding) {
	static const char* map[] = {
		"ISO-8859-1",
		"ISO-8859-2",
		"ISO-8859-3",
		"ISO-8859-4",
		"ISO-8859-5",
		"ISO-8859-6",
		"ISO-8859-7",
		"ISO-8859-8",
		"ISO-8859-9",
		"ISO-8859-10",
		"MACROMAN",
		"SHIFT-JIS",
		"EUC-JP",
		"ISO-2022-JP-2",
		"WINDOWS-1252",
		"UCS-4",
		"KOI8-R",
		"WINDOWS-1251",
		"CP866",
		"CP850",
		"EUC-KR",
		"ISO-8859-13",
		"ISO-8859-14",
		"ISO-8859-15",
		"UTF-8"
	};
	if (encoding >= 0 && encoding <= 24)
		return map[encoding];
	else
		return "UTF-8";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertToUTF8( const BmString& srcCharset, const BmString& src,
										  BmString& dest) {
	if (srcCharset == "UTF-8") {
		// source already is UTF8...
		dest = src;
		return;
	}
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmUtf8Encoder encoder( &srcBuf, srcCharset, blockSize);
	destBuf.Write( &encoder, blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertFromUTF8( const BmString& destCharset, const BmString& src, 
											 BmString& dest) {
	if (destCharset == "UTF-8") {
		// destination shall be UTF8, too...
		dest = src;
		return;
	}
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmUtf8Decoder encoder( &srcBuf, destCharset, blockSize);
	destBuf.Write( &encoder, blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Encode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord) {
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmMemFilterRef encoder = FindEncoderFor( &srcBuf, encodingStyle, isEncodedWord, blockSize);
	destBuf.Write( encoder.get(), blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Decode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord) {
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmMemFilterRef decoder = FindDecoderFor( &srcBuf, encodingStyle, isEncodedWord, blockSize);
	destBuf.Write( decoder.get(), blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmEncoding::ConvertHeaderPartToUTF8( const BmString& headerPart, 
															 const BmString& defaultCharset) {
	int32 nm;
	Regexx rx;
	rx.expr( "=\\?(.+?)\\?(.)\\?(.+?)\\?=\\s*");
	rx.str( headerPart);
	const uint32 blockSize = max( (int32)128, headerPart.Length());
	BmStringOBuf utf8( blockSize, 2.0);
	
	if ((nm = rx.exec( Regexx::global))!=0) {
		int32 len=headerPart.Length();
		int32 curr=0;
		vector<RegexxMatch>::const_iterator i;
		for( i = rx.match.begin(); i != rx.match.end(); ++i) {
			if (curr < i->start()) {
				// copy the characters between start/curr match and first match:
				utf8.Write( headerPart.String()+curr, i->start()-curr);
			}
			// convert the match (an encoded word) into UTF8:
			const BmString srcCharset( i->atom[0]);
			const BmString srcQuotingStyle( i->atom[1]);
			BmStringIBuf text( headerPart.String()+i->atom[2].start(), i->atom[2].Length());
			BmMemFilterRef decoder = FindDecoderFor( &text, srcQuotingStyle, true, blockSize);
			BmUtf8Encoder textConverter( decoder.get(), srcCharset, blockSize);
			utf8.Write( &textConverter, blockSize);
			curr = i->start()+i->Length();
		}
		if (curr<len) {
			utf8.Write( headerPart.String()+curr, len-curr);
		}
	} else {
		BmStringIBuf text( headerPart);
		BmUtf8Encoder textConverter( &text, defaultCharset);
		utf8.Write( &textConverter);
	}
	return utf8.TheString();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmEncoding::ConvertUTF8ToHeaderPart( const BmString& utf8Text, 
															 const BmString& charset,
															 bool useQuotedPrintableIfNeeded,
															 bool fold, int32 fieldLen) {
	bool needsQuotedPrintable = false;
	BmString transferEncoding = "7bit";
	if (useQuotedPrintableIfNeeded)
		needsQuotedPrintable = NeedsEncoding( utf8Text);
	if (ThePrefs->GetBool( "Allow8BitMimeInHeader", false) && needsQuotedPrintable) {
		transferEncoding = "8bit";
		needsQuotedPrintable = false;
	}
	BmString charsetString;
	ConvertFromUTF8( charset, utf8Text, charsetString);
	BmString encodedString;
	if (needsQuotedPrintable) {
		// encoded-words (quoted-printable) are neccessary, since headerfield contains
		// non-ASCII chars:
		Encode( "Q", charsetString, encodedString, true);
		int32 maxChars = BM_MAX_LINE_LEN		// 76 chars maximum
							- (fieldLen + 2)		// space used by "fieldname: "
							- charset.Length()	// length of charset in encoded-word
							- 7;						// =?...?q?...?=
		if (fold && encodedString.Length() > maxChars) {
			// fold header-line, since it is too long:
			BmString foldedString;
			while( encodedString.Length() > maxChars) {
				BmString tmp;
				int32 foldPos = maxChars;
				int32 lastEqualSignPos = encodedString.FindLast( '=', foldPos);
				if (lastEqualSignPos != B_ERROR && foldPos - lastEqualSignPos < 3) {
					// we avoid folding within an encoded char (=XX) by moving the
					// folding point just before the equal-sign:
					foldPos = lastEqualSignPos;
				}
				encodedString.MoveInto( tmp, 0, foldPos);
				foldedString << "=?" << charset << "?q?" << tmp << "?=\r\n ";
				// now compute maxChars for lines without the leading fieldname:
				maxChars = BM_MAX_LINE_LEN			// 76 chars maximum
								- 1						// a single space
								- charset.Length()	// length of charset in encoded-word
								- 7;						// =?...?q?...?=
			}
			foldedString << "=?" << charset << "?q?" << encodedString << "?=";
			return foldedString;		
		} else
			return BmString("=?") + charset + "?q?" + encodedString + "?=";
	} else {
		// simpler case, no encoded-words neccessary:
		Encode( transferEncoding, charsetString, encodedString, true);
		int32 maxChars = BM_MAX_LINE_LEN		// 76 chars maximum
							- (fieldLen + 2);		// space used by "fieldname: "
		bool isFirstLine = true;
		if (fold && encodedString.Length() > maxChars) {
			// fold header-line, since it is too long:
			BmString foldedString;
			while( encodedString.Length() > maxChars) {
				int32 foldPos = encodedString.FindLast( ' ', maxChars-1);
				if (foldPos == 0)
					foldPos = 1;						// skip over space at beginning of line
				if (foldPos == B_ERROR)
					foldPos = isFirstLine ? 0 : maxChars;
				BmString tmp;
				encodedString.MoveInto( tmp, 0, foldPos);
				foldedString << tmp << "\r\n ";
				if (isFirstLine) {
					// now compute maxChars for lines without the leading fieldname:
					maxChars = BM_MAX_LINE_LEN			// 76 chars maximum
									- 1;						// a single space
					isFirstLine = false;
				}
			}
			foldedString << encodedString;
			return foldedString;		
		} else
			return encodedString;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmEncoding::NeedsEncoding( const BmString& utf8String) {
	// check if string needs quoted-printable/base64 encoding
	// (which it does if it contains non-ASCII chars):
	for( const char* p = utf8String.String(); *p; ++p) {
		if (*p<32 && *p!='\r' && *p!='\n' && *p!='\t')
			// N.B.: This is a signed char, so c<32 means [0-31] and [128-255]
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmEncoding::IsCompatibleWithText( const BmString& s) {
	// check if given string contains any characters that suggest the data
	// is in fact binary:
	for( int32 i=0; i<s.Length(); ++i) {
		// currently, we believe that only ascii-0 is not compatible
		// with attachments of type text:
		if (!s[i])
			return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMemFilterRef BmEncoding::FindDecoderFor( BmMemIBuf* input, 
														 const BmString& encodingStyle, 
														 bool isEncodedWord, uint32 blockSize) {
	BmMemFilter* filter = NULL;
	if (encodingStyle.ICompare("q")==0 || encodingStyle.ICompare("quoted-printable")==0)
		filter = new BmQuotedPrintableDecoder( input, isEncodedWord, blockSize);
	else if (encodingStyle.ICompare("b")==0 || encodingStyle.ICompare("base64")==0)
		filter = new BmBase64Decoder( input, blockSize);
	else if (encodingStyle.ICompare("7bit")==0 || encodingStyle.ICompare("8bit")==0)
		filter = new BmLinebreakDecoder( input, blockSize);
	else if (encodingStyle.ICompare("binary")==0)
		filter = new BmBinaryDecoder( input, blockSize);
	else {
		BM_SHOWERR( BmString("FindDecoderFor(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nAssuming 7bit.");
		filter = new BmLinebreakDecoder( input, blockSize);
	}
	return BmMemFilterRef( filter);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMemFilterRef BmEncoding::FindEncoderFor( BmMemIBuf* input, 
														 const BmString& encodingStyle, 
														 bool isEncodedWord, uint32 blockSize) {
	BmMemFilter* filter = NULL;
	if (encodingStyle.ICompare("q")==0 || encodingStyle.ICompare("quoted-printable")==0)
		filter = new BmQuotedPrintableEncoder( input, isEncodedWord, blockSize);
	else if (encodingStyle.ICompare("b")==0 || encodingStyle.ICompare("base64")==0)
		filter = new BmBase64Encoder( input, blockSize);
	else if (encodingStyle.ICompare("7bit")==0 || encodingStyle.ICompare("8bit")==0)
		filter = new BmLinebreakEncoder( input, blockSize);
	else if (encodingStyle.ICompare("binary")==0)
		filter = new BmBinaryEncoder( input, blockSize);
	else {
		BM_SHOWERR( BmString("FindEncoderFor(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nAssuming 7bit.");
		filter = new BmLinebreakEncoder( input, blockSize);
	}
	return BmMemFilterRef( filter);
}




/********************************************************************************\
	BmUtf8Decoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmUtf8Decoder::BmUtf8Decoder( BmMemIBuf* input, const BmString& destCharset, 
										uint32 blockSize)
	:	inherited( input, blockSize)
	,	mDestCharset( destCharset)
	,	mIconvDescr( ICONV_ERR)
	,	mTransliterate( false)
	,	mDiscard( false)
{
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmUtf8Decoder::~BmUtf8Decoder()
{
	if (mIconvDescr != ICONV_ERR) {
		iconv_close( mIconvDescr);
		mIconvDescr = ICONV_ERR;
	}
}

/*------------------------------------------------------------------------------*\
	Reset()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::Reset( BmMemIBuf* input) {
	inherited::Reset( input);
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	InitConverter()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::InitConverter() {
	if (mIconvDescr != ICONV_ERR) {
		iconv_close( mIconvDescr);
		mIconvDescr = ICONV_ERR;
	}
	BmString flag;
	if (mTransliterate)
		flag = "//TRANSLIT";
	else if (mDiscard)
		flag = "//IGNORE";
	BmString toSet = mDestCharset	+ flag;
	if ((mIconvDescr = iconv_open( toSet.String(), "UTF-8")) == ICONV_ERR) {
		BM_LOGERR( BmString("libiconv: unable to convert from UTF-8 to ") << toSet);
		mHadError = true;
		return;
	}
}

/*------------------------------------------------------------------------------*\
	SetTransliterate()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::SetTransliterate( bool transliterate) {
	if (mIconvDescr != ICONV_ERR && mTransliterate == transliterate)
		return;
	mTransliterate = transliterate;
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	SetDiscard()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::SetDiscard( bool discard) {
	if (mIconvDescr != ICONV_ERR && mDiscard == discard)
		return;
	mDiscard = discard;
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::Filter( const char* srcBuf, uint32& srcLen, 
									 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to decode utf8 of ") << srcLen << " bytes");

	if (mDestCharset == "UTF-8") {
		// destination shall be UTF8, too...
		uint32 size = min( srcLen, destLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
		return;
	}

	const char* inBuf = srcBuf;
	size_t inBytesLeft = srcLen;
	char* outBuf = destBuf;
	size_t outBytesLeft = destLen;
	size_t irrevCount = iconv( mIconvDescr, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	srcLen -= inBytesLeft;
	destLen -= outBytesLeft;
	if (irrevCount == (size_t)-1) {
		if (errno == E2BIG)
			BM_LOG3( BM_LogMailParse, "Result in utf8-decode: too big, need to continue");
		else if (errno == EINVAL)
			BM_LOG3( BM_LogMailParse, "Result in utf8-decode: stopped on multibyte char, need to continue");
		else if (errno == EILSEQ) {
			BM_LOG( BM_LogMailParse, "Result in utf8-decode: invalid multibyte char found");
			mHadError = true;
		} else {
			BM_LOGERR( BmString("Unknown error-result in utf8-decode: ") << errno);
			mHadError = true;
		}
	}

	BM_LOG3( BM_LogMailParse, "utf8-decode: done");
}



/********************************************************************************\
	BmUtf8Encoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmUtf8Encoder::BmUtf8Encoder( BmMemIBuf* input, const BmString& srcCharset, 
										uint32 blockSize)
	:	inherited( input, blockSize)
	,	mSrcCharset( srcCharset)
	,	mIconvDescr( ICONV_ERR)
	,	mTransliterate( false)
	,	mDiscard( false)
	,	mHadToDiscardChars( false)
{
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmUtf8Encoder::~BmUtf8Encoder()
{
	if (mIconvDescr != ICONV_ERR) {
		iconv_close( mIconvDescr);
		mIconvDescr = ICONV_ERR;
	}
}

/*------------------------------------------------------------------------------*\
	Reset()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::Reset( BmMemIBuf* input) {
	inherited::Reset( input);
	InitConverter();
	mHadToDiscardChars = false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::InitConverter() { 
	if (mIconvDescr != ICONV_ERR) {
		iconv_close( mIconvDescr);
		mIconvDescr = ICONV_ERR;
	}
	BmString flag;
	if (mTransliterate)
		flag = "//TRANSLIT";
	else if (mDiscard)
		flag = "//IGNORE";
	BmString toSet = BmString("UTF-8") + flag;
	if ((mIconvDescr = iconv_open( toSet.String(), mSrcCharset.String())) == ICONV_ERR) {
		BM_LOGERR( BmString("libiconv: unable to convert from ") << mSrcCharset << " to " << toSet);
		mHadError = true;
		return;
	}
}

/*------------------------------------------------------------------------------*\
	SetTransliterate()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::SetTransliterate( bool transliterate) {
	if (mIconvDescr != ICONV_ERR && mTransliterate == transliterate)
		return;
	mTransliterate = transliterate;
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	SetDiscard()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::SetDiscard( bool discard) {
	if (mIconvDescr != ICONV_ERR && mDiscard == discard)
		return;
	mDiscard = discard;
	InitConverter();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::Filter( const char* srcBuf, uint32& srcLen, 
									 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to encode utf8 of ") << srcLen << " bytes");

	if (mSrcCharset == "UTF-8") {
		// source already is UTF8...
		uint32 size = min( srcLen, destLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
		return;
	}

	const char* inBuf = srcBuf;
	size_t inBytesLeft = srcLen;
	char* outBuf = destBuf;
	size_t outBytesLeft = destLen;
	size_t irrevCount = iconv( mIconvDescr, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	srcLen -= inBytesLeft;
	destLen -= outBytesLeft;
	if (irrevCount == (size_t)-1) {
		if (errno == E2BIG)
			BM_LOG3( BM_LogMailParse, "Result in utf8-encode: too big, need to continue");
		else if (errno == EINVAL)
			BM_LOG3( BM_LogMailParse, "Result in utf8-encode: stopped on multibyte char, need to continue");
		else if (errno == EILSEQ) {
			BM_LOG2( BM_LogMailParse, "Result in utf8-encode: invalid multibyte char found");
			mHadToDiscardChars = true;
			int on = 1;
			iconvctl( mIconvDescr, ICONV_SET_DISCARD_ILSEQ, &on);
		} else {
			BM_LOGERR( BmString("Unknown error-result in utf8-encode: ") << errno);
			mHadError = true;
		}
	}

	BM_LOG3( BM_LogMailParse, "utf8-encode: done");
}



/********************************************************************************\
	BmQuotedPrintableDecoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableDecoder::BmQuotedPrintableDecoder( BmMemIBuf* input, 
																	 bool isEncodedWord, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
	,	mIsEncodedWord( isEncodedWord)
	,	mLastWasLinebreak( false)
	,	mSpacesThatMayNeedRemoval( 0)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableDecoder::Filter( const char* srcBuf, uint32& srcLen, 
													char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to decode quoted-printable of ") << srcLen << " bytes");
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	char c,c1,c2;
	const BmString qpChars("abcdef0123456789ABCDEF");
	for( ; src<srcEnd && dest<destEnd; ++src) {
		c = *src;
		if (mSpacesThatMayNeedRemoval && c!=' ') {
			if (c=='\r' || c=='\n') {
				// this group of spaces lives immediately before a newline,
				// which means that it needs to be removed:
				mSpacesThatMayNeedRemoval = 0;
			} else {
				// this group of spaces does not require removal, so it will
				// be added as all other chars:
				++mSpacesThatMayNeedRemoval;
				while( dest<destEnd && --mSpacesThatMayNeedRemoval)
					*dest++ = ' ';
			}
			if (dest>=destEnd)
				break;
		}
		if (c == '\n') {
			mLastWasLinebreak = true;
			*dest++ = c;
		} else if (mLastWasLinebreak && c == ' ') {
			mSpacesThatMayNeedRemoval++;
		} else if (mIsEncodedWord && c == '_') {
			// in encoded-words, underlines are really spaces (a real underline is encoded):
			*dest++ = ' ';
			mLastWasLinebreak = false;
		} else if (c == '=') {
			if (src>srcEnd-3 && !mInput->IsAtEnd())
				break;							// need two more characters in buffer
			if (src<=srcEnd-3 
			&& (c1=*(src+1))!=0 && qpChars.FindFirst(c1)!=B_ERROR 
			&& (c2=*(src+2))!=0 && qpChars.FindFirst(c2)!=B_ERROR) {
				// decode a single character:
				*dest++ = HEXDIGIT2CHAR(c1)*16 + HEXDIGIT2CHAR(c2);
				src += 2;
				mLastWasLinebreak = false;
			} else {
				// softbreak encountered, we keep note of it's position
				mLastWasLinebreak = true;
				if (src<=srcEnd-3)
					src += 2;
							// skip over "\r\n" in order to join the line
			}
		} else if (c == '\r') {
			// skip over carriage-returns
			continue;
		} else {
			*dest++ = c;
			mLastWasLinebreak = false;
		}
	}
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "qp-decode: done");
}



/********************************************************************************\
	BmQuotedPrintableEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableEncoder::BmQuotedPrintableEncoder( BmMemIBuf* input, 
																	 bool isEncodedWord, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
	,	mIsEncodedWord( isEncodedWord)
	,	mCurrLineLen( 0)
	,	mSpacesThatMayNeedEncoding( 0)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableEncoder::Filter( const char* srcBuf, uint32& srcLen, 
													char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to encode quoted-printable of ") << srcLen << " bytes");
	const char* safeChars = 
		mIsEncodedWord 
			? (ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
					? "%&/()?+*,.;:<>-"
					: "%&/()?+*,.;:<>-!\"#$@[]\\^'{|}~")
							// in encoded words, underscore has to be encoded, since it
							// is used for spaces!
			: (ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
					? "%&/()?+*,.;:<>-_"
					: "%&/()?+*,.;:<>-_!\"#$@[]\\^'{|}~");
							// in bodies, the underscore is safe, i.e. it need not be encoded.
	char add[4];
	int32 addLen = 0;
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	unsigned char c;
	for( ; src<srcEnd && dest<destEnd; ++src, addLen=0) {
		c = *src;
		if (mSpacesThatMayNeedEncoding && c!=' ') {
			if (c=='\r' || c=='\n') {
				// this group of spaces lives immediately before a newline,
				// which means that it requires encoding:
				++mSpacesThatMayNeedEncoding;
				while( dest<=destEnd-3 && --mSpacesThatMayNeedEncoding) {
					*dest++ = '=';
					*dest++ = (char)CHAR2HIGHNIBBLE(' ');
					*dest++ = (char)CHAR2LOWNIBBLE(' ');
				}
			} else {
				// this group of spaces does not require encoding:
				++mSpacesThatMayNeedEncoding;
				while( dest<destEnd && --mSpacesThatMayNeedEncoding)
					*dest++ = ' ';
			}
			if (dest>=destEnd)
				break;
		}
		if (isalnum(c) || strchr( safeChars, c)) {
			add[addLen++] = c;
		} else if (c == ' ') {
			// in encoded-words, we always replace SPACE by underline:
			if (mIsEncodedWord)
				add[addLen++] = '_';
			else {
				mSpacesThatMayNeedEncoding++;
				continue;
			}
		} else if (c=='\r') {
			continue;							// ignore '\r'
		} else if (c=='\n') {
			if (dest+2>=destEnd)
				break;
			*dest++ = '\r';
			*dest++ = '\n';
			mCurrLineLen = 0;
			continue;
		} else {
			add[0] = '='; 
			add[1] = (char)CHAR2HIGHNIBBLE(c); 
			add[2] = (char)CHAR2LOWNIBBLE(c);
			addLen=3;
		}
		if (!mIsEncodedWord && mCurrLineLen + addLen >= BM_MAX_LINE_LEN) {
			if (dest+addLen+3>=destEnd)
				break;
			// insert soft linebreak:
			*dest++ = '=';
			*dest++ = '\r';
			*dest++ = '\n';
			mCurrLineLen = 0;
		}
		for( int i=0; i<addLen; ++i)
			*dest++ = add[i];
		mCurrLineLen += addLen;
	}
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "qp-encode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableEncoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	while( dest<=destEnd-3 && mSpacesThatMayNeedEncoding--) {
		*dest++ = '=';
		*dest++ = (char)CHAR2HIGHNIBBLE(' ');
		*dest++ = (char)CHAR2LOWNIBBLE(' ');
	}
	destLen = dest-destBuf;
	mIsFinalized = true;
}


/********************************************************************************\
	BmBase64Decoder
\********************************************************************************/

const int32 BmBase64Decoder::nBase64Alphabet[256] = {
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	 -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	 -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Decoder::Filter( const char* srcBuf, uint32& srcLen, 
										char* destBuf, uint32& destLen) {

	BM_LOG3( BM_LogMailParse, BmString("starting to decode base64 of ") << srcLen << " bytes");

	int32 value;
	const unsigned char* src = (const unsigned char*)srcBuf;
	const unsigned char* srcEnd = (const unsigned char*)srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
		
	while( src<srcEnd && dest<=destEnd-3) {
		if ((value = nBase64Alphabet[*src++])<0)
			continue;
			
		mConcat |= (value << ((3-mIndex)*6));
		
		if (++mIndex == 4) {
			*dest++ = (mConcat & 0x00ff0000) >> 16;
			*dest++ = (mConcat & 0x0000ff00) >> 8;
			*dest++ = (mConcat & 0x000000ff);
			mConcat = mIndex = 0;
		}
	}
	srcLen = src-(unsigned char*)srcBuf;
	destLen = dest-destBuf;

	BM_LOG3( BM_LogMailParse, "base64-decode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Decoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mIndex) {
		// output remaining characters:
		BM_ASSERT( dest<=destEnd-3);
							// must be the case for mIndex!=0
		*dest++ = (mConcat & 0x00ff0000) >> 16;
		if (mIndex>2)
			*dest++ = (mConcat & 0x0000ff00) >> 8;
		mConcat = mIndex = 0;
	}
	destLen = dest-destBuf;
	mIsFinalized = true;
}



/********************************************************************************\
	BmBase64Encoder
\********************************************************************************/

const char BmBase64Encoder::nBase64Alphabet[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'+',
	'/'
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Encoder::Filter( const char* srcBuf, uint32& srcLen, 
										char* destBuf, uint32& destLen) {

	BM_LOG3( BM_LogMailParse, BmString("starting to encode base64 of ") << srcLen << " bytes");

	const unsigned char* src = (unsigned char*)srcBuf;
	const unsigned char* srcEnd = (unsigned char*)srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
		
	while( src<srcEnd && dest<=destEnd-6) {
		mConcat |= (*src++ << ((2-mIndex)*8));
		if (++mIndex == 3) {
			*dest++ = nBase64Alphabet[(mConcat >> 18) & 63];
			*dest++ = nBase64Alphabet[(mConcat >> 12) & 63];
			*dest++ = nBase64Alphabet[(mConcat >> 6) & 63];
			*dest++ = nBase64Alphabet[mConcat & 63];
			mConcat = mIndex = 0;
			mCurrLineLen += 4;
			if (mCurrLineLen >= BM_MAX_LINE_LEN) {
				*dest++ = '\r';
				*dest++ = '\n';
				mCurrLineLen = 0;
			}
		}
	}
	srcLen = src-(unsigned char*)srcBuf;
	destLen = dest-destBuf;

	BM_LOG3( BM_LogMailParse, "base64-encode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Encoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mIndex) {
		if (dest<=destEnd-4) {
			// output remaining bytes and pad if neccessary:
			*dest++ = nBase64Alphabet[(mConcat >> 18) & 63];
			*dest++ = nBase64Alphabet[(mConcat >> 12) & 63];
			if (mIndex==2)
				*dest++ = nBase64Alphabet[(mConcat >> 6) & 63];
			else
				*dest++ = '=';
			*dest++ = '=';
			mIsFinalized = true;
		}
	} else
		mIsFinalized = true;
	destLen = dest-destBuf;
}



/********************************************************************************\
	BmLinebreakDecoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmLinebreakDecoder::BmLinebreakDecoder( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmLinebreakDecoder::Filter( const char* srcBuf, uint32& srcLen, 
											char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to decode linebreaks of ") << srcLen << " bytes");
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	char c;
	for( ; src<srcEnd && dest<destEnd; ++src) {
		if ((c = *src) != '\r')
			*dest++ = c;
	}

	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "linebreak-decode: done");
}



/********************************************************************************\
	BmLinebreakEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmLinebreakEncoder::BmLinebreakEncoder( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize)
	,	mLastWasCR( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmLinebreakEncoder::Filter( const char* srcBuf, uint32& srcLen, 
											char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to encode linebreaks of ") << srcLen << " bytes");

	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	char c;
	for( ; src<srcEnd && dest<destEnd; ++src) {
		if ((c = *src)=='\n' && !mLastWasCR) {
			if (dest>destEnd-2)
				break;
			*dest++ = '\r';
		}
		*dest++ = c;
	}

	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "linebreak-encode: done");
}



/********************************************************************************\
	BmBinaryDecoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBinaryDecoder::BmBinaryDecoder( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBinaryDecoder::Filter( const char* srcBuf, uint32& srcLen, 
										char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to decode binary of ") << srcLen << " bytes");

	uint32 size = min( destLen, srcLen);
	memcpy( destBuf, srcBuf, size);

	srcLen = destLen = size;
	BM_LOG3( BM_LogMailParse, "binary-decode: done");
}



/********************************************************************************\
	BmBinaryEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBinaryEncoder::BmBinaryEncoder( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBinaryEncoder::Filter( const char* srcBuf, uint32& srcLen, 
										char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to encode binary of ") << srcLen << " bytes");

	uint32 size = min( destLen, srcLen);
	memcpy( destBuf, srcBuf, size);

	srcLen = destLen = size;
	BM_LOG3( BM_LogMailParse, "binary-encode: done");
}
