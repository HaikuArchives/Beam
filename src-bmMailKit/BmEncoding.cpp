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


#include <ctype.h>

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

const int BM_MAX_LINE_LEN = 76;
							// as stated in [K. Johnson, p. 149f]

BmCharsetMap BmEncoding::TheCharsetMap;

BmString BmEncoding::DefaultCharset = "iso-8859-15";

static iconv_t ICONV_ERR = (iconv_t)0xFFFFFFFF;

/*------------------------------------------------------------------------------*\
	HandleOneCharset()
		-	this function is called during initialization of libiconv.
		-	for every passed charset (which can have more than one name) Beam checks
			if it prefers one of the given names over all the other (according to
			some regular expressions). 
			If so, the name Beam prefers is used, the first name otherwise.
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
		"iso-8859-1",
		"iso-8859-2",
		"iso-8859-3",
		"iso-8859-4",
		"iso-8859-5",
		"iso-8859-6",
		"iso-8859-7",
		"iso-8859-8",
		"iso-8859-9",
		"iso-8859-10",
		"macroman",
		"shift-jis",
		"euc-jp",
		"iso-2022-jp-2",
		"windows-1252",
		"ucs-4",
		"koi8-r",
		"windows-1251",
		"cp866",
		"cp850",
		"euc-kr",
		"iso-8859-13",
		"iso-8859-14",
		"iso-8859-15",
		"utf-8"
	};
	if (encoding >= 0 && encoding <= 24)
		return map[encoding];
	else
		return "utf-8";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertToUTF8( const BmString& srcCharset, const BmString& src,
										  BmString& dest) {
	if (srcCharset == "utf-8") {
		// source already is utf-8...
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
	if (destCharset == "utf-8") {
		// destination shall be utf-8, too...
		dest = src;
		return;
	}
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmUtf8Decoder decoder( &srcBuf, destCharset, blockSize);
	destBuf.Write( &decoder, blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Encode( BmString encodingStyle, const BmString& src, BmString& dest) {
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmMemFilterRef encoder = FindEncoderFor( &srcBuf, encodingStyle, blockSize);
	destBuf.Write( encoder.get(), blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Decode( BmString encodingStyle, const BmString& src, BmString& dest) {
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmMemFilterRef decoder = FindDecoderFor( &srcBuf, encodingStyle, false, blockSize);
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
	rx.expr( "=\\?(.+?)\\?(.)\\?(.*?)\\?=");
	rx.str( headerPart);
	const uint32 blockSize = max( (int32)128, headerPart.Length());
	BmStringOBuf utf8( blockSize, 2.0);
	
	if ((nm = rx.exec( Regexx::global))!=0) {
		Regexx rxWhite;
		int32 len=headerPart.Length();
		int32 curr=0;
		vector<RegexxMatch>::const_iterator i;
		for( i = rx.match.begin(); i != rx.match.end(); ++i) {
			if (curr < i->start()) {
				// copy the characters between start/curr match and next match,
				// unless it's all whitespace (rfc2047 requires whitespace between
				// encoded words to be removed):
				BmString chars( headerPart.String()+curr, i->start()-curr);
				if (!rxWhite.exec( chars, "^\\s*$"))
					utf8.Write( chars);
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
															 int32 fieldLen) {
	bool needsQuotedPrintable = false;
	BmString transferEncoding = "7bit";
	if (useQuotedPrintableIfNeeded)
		needsQuotedPrintable = NeedsEncoding( utf8Text);
	if (ThePrefs->GetBool( "Allow8BitMimeInHeader", false) && needsQuotedPrintable) {
		transferEncoding = "8bit";
		needsQuotedPrintable = false;
	}
	const uint32 blockSize = max( (int32)128, utf8Text.Length());
	BmStringIBuf srcBuf( utf8Text);
	BmUtf8Decoder textConverter( &srcBuf, charset);
	BmStringOBuf tempIO( blockSize, 2);
	if (needsQuotedPrintable) {
		// encoded-words (quoted-printable) are neccessary, since headerfield contains
		// non-ASCII chars:
		BmQpEncodedWordEncoder qpEncoder( &textConverter, blockSize, 
													 fieldLen+2, charset);
		tempIO.Write( &qpEncoder);
	} else {
		BmFoldedLineEncoder foldEncoder( &textConverter, BM_MAX_LINE_LEN, 
													blockSize, fieldLen+2);
		tempIO.Write( &foldEncoder);
	}
	BmString foldedString;
	foldedString.Adopt( tempIO.TheString());
	return foldedString;		
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
														 uint32 blockSize) {
	BmMemFilter* filter = NULL;
	if (encodingStyle.ICompare("q")==0 || encodingStyle.ICompare("quoted-printable")==0)
		filter = new BmQuotedPrintableEncoder( input, blockSize);
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
	if (mDestCharset.ICompare("utf8")==0)
		// common mistake: utf8 instead of utf-8:
		mDestCharset = "utf-8";

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
	if (!mDestCharset.Length()
	|| (mIconvDescr = iconv_open( toSet.String(), "utf-8")) == ICONV_ERR) {
		BM_SHOWERR( BmString("libiconv: unable to convert from utf-8 to ") << toSet);
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
			BM_SHOWERR( BmString("Unknown error-result in utf8-decode: ") << errno);
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
	if (mSrcCharset.ICompare("utf8")==0)
		// common mistake: utf8 instead of utf-8:
		mSrcCharset = "utf-8";

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
	BmString toSet = BmString("utf-8") + flag;
	if (!mSrcCharset.Length()
	|| (mIconvDescr = iconv_open( toSet.String(), mSrcCharset.String())) == ICONV_ERR) {
		BM_SHOWERR( BmString("libiconv: unable to convert from ") << mSrcCharset << " to " << toSet);
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
			BM_SHOWERR( BmString("Unknown error-result in utf8-encode: ") << errno);
			mHadError = true;
		}
	}

	BM_LOG3( BM_LogMailParse, "utf8-encode: done");
}



/********************************************************************************\
	BmQuotedPrintableDecoder
\********************************************************************************/

inline unsigned char HEXDIGIT2CHAR( unsigned int d) {
	return (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableDecoder::BmQuotedPrintableDecoder( BmMemIBuf* input, 
																	 bool isEncodedWord, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
	,	mIsEncodedWord( isEncodedWord)
	,	mSpacesThatMayNeedRemoval( 0)
	,	mSoftbreakPending( false)
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
		if (c == '\r') {
			// skip over carriage-returns:
			continue;
		}
		if (mSoftbreakPending) {
			if (c == '\n') {
				// softbreak, skip over "\n" in order to join the line:
				mSoftbreakPending = false;
				mSpacesThatMayNeedRemoval = 0;
				continue;
			} else if (c != ' ') {
				// some broken encoding, we insert the equal-sign...
				*dest++ = '=';
				// ...unset softbreak-mode...
				mSoftbreakPending = false;
				// ...and re-process the current char:
				--src;
				continue;
			}
		}
		if (mSpacesThatMayNeedRemoval && c != ' ') {
			if (c=='\n') {
				// this group of spaces lives immediately before a newline,
				// which means that it needs to be removed:
				mSpacesThatMayNeedRemoval = 0;
			} else {
				// this group of spaces does not require removal, so it will
				// be added as all other chars:
				while( dest<destEnd && mSpacesThatMayNeedRemoval--)
					*dest++ = ' ';
			}
			if (dest>=destEnd)
				break;
		}
		if (c == ' ')
			mSpacesThatMayNeedRemoval++;
		else {
			mSoftbreakPending = false;
			mSpacesThatMayNeedRemoval = 0;
			if (c == '=') {
				if (src>srcEnd-3 && !mInput->IsAtEnd())
					break;							// want two more characters in buffer
				if (src<=srcEnd-3 && (c1=*(src+1))!=0 && (c2=*(src+2))!=0) {
					if (qpChars.FindFirst(c1)!=B_ERROR && qpChars.FindFirst(c2)!=B_ERROR) {
						// decode a single character:
						*dest++ = HEXDIGIT2CHAR(c1)*16 + HEXDIGIT2CHAR(c2);
						src += 2;
					} else {
						// it's either a softbreak or broken encoding, we'll see:
						mSoftbreakPending = true;
					}
				} else {
					// characters missing at end (broken encoding), we just copy:
					*dest++ = c;
				}
			} else if (mIsEncodedWord && c == '_') {
				// in encoded-words, underlines are really spaces (a real underline is encoded):
				*dest++ = ' ';
			} else {
				// anything else is just copied:
				*dest++ = c;
			}
		}
	}
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "qp-decode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableDecoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mSoftbreakPending && dest < destEnd) {
		// broken encoding, we need to insert the equal-sign...
		*dest++ = '=';
		mSoftbreakPending = false;
	}
	destLen = dest-destBuf;
	mIsFinalized = !mSoftbreakPending;
}



/********************************************************************************\
	BmQuotedPrintableEncoder
\********************************************************************************/

inline unsigned int CHAR2HIGHNIBBLE( unsigned char c) {
	return (((c) > 0x9F ? 'A'-10 : '0')+((c)>>4));
}
inline unsigned int CHAR2LOWNIBBLE( unsigned char c) {
	return ((((c)&0x0F) > 9 ? 'A'-10 : '0')+((c)&0x0F));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableEncoder::BmQuotedPrintableEncoder( BmMemIBuf* input, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
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
				(ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
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
			mSpacesThatMayNeedEncoding++;
			continue;
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
		if (mCurrLineLen + addLen >= BM_MAX_LINE_LEN) {
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
	++mSpacesThatMayNeedEncoding;
	while( dest<=destEnd-3 && --mSpacesThatMayNeedEncoding) {
		*dest++ = '=';
		*dest++ = (char)CHAR2HIGHNIBBLE(' ');
		*dest++ = (char)CHAR2LOWNIBBLE(' ');
	}
	destLen = dest-destBuf;
	mIsFinalized = (mSpacesThatMayNeedEncoding==0);
}



/********************************************************************************\
	BmQpEncodedWordEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQpEncodedWordEncoder::BmQpEncodedWordEncoder( BmMemIBuf* input, 
																uint32 blockSize,
																int startAtOffset,
																const BmString& charset)
	:	inherited( input, blockSize)
	,	mCurrLineLen( startAtOffset)
{
	mEncodedWord = BmString("=?")<<charset<<"?q?";
	mQueuedChars = mEncodedWord;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQpEncodedWordEncoder::Filter( const char* srcBuf, uint32& srcLen, 
												 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to qp-word-encode ") << srcLen << " bytes");
	const char* safeChars = 
			 (ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
					? "%&/()+*,.;:<>-"
					: "%&/()+*,.;:<>-!\"#$@[]\\^'{|}~");
							// in encoded words, underscore has to be encoded, since it
							// is used for spaces!
							// and the question-mark has to be encoded, too, since it
							// is part of the encoded-word markers!
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mQueuedChars.Length()) {
		if (mCurrLineLen + mQueuedChars.Length() + 2 > BM_MAX_LINE_LEN) {
			if (dest<=destEnd-5) {
				*dest++ = '?';
				*dest++ = '=';
				*dest++ = '\r';
				*dest++ = '\n';
				*dest++ = ' ';
				mCurrLineLen = 1;
				mQueuedChars.Prepend( mEncodedWord);
			} else
				goto out;
		}
		while( dest<destEnd && mQueuedChars.Length()) {
			*dest++ = mQueuedChars[0];
			mQueuedChars.Remove( 0, 1);
			mCurrLineLen++;
		}
	}
	
	unsigned char c;
	char add[4];
	int32 addLen;
	for( addLen=0; src<srcEnd && dest<destEnd; ++src, addLen=0) {
		c = *src;
		if (isalnum(c) || strchr( safeChars, c)) {
			add[addLen++] = c;
		} else if (c == ' ') {
			// in encoded-words, we always replace SPACE by underline:
			add[addLen++] = '_';
		} else {
			add[addLen++] = '=';
			add[addLen++] = (char)CHAR2HIGHNIBBLE(c);
			add[addLen++] = (char)CHAR2LOWNIBBLE(c);
		}
		if (mCurrLineLen + mQueuedChars.Length() + 2 + addLen >= BM_MAX_LINE_LEN) {
			if (!mQueuedChars.Length() && dest<=destEnd-5) {
				*dest++ = '?';
				*dest++ = '=';
				*dest++ = '\r';
				*dest++ = '\n';
				*dest++ = ' ';
				mCurrLineLen = 1;
				mQueuedChars.Prepend( mEncodedWord);
			}
			break;
		}
		mQueuedChars.Append( add, addLen);
	}
out:
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "qp-word-encode: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQpEncodedWordEncoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mQueuedChars.Length()) {
		if (mCurrLineLen + mQueuedChars.Length() + 2 > BM_MAX_LINE_LEN) {
			if (dest<=destEnd-5) {
				*dest++ = '?';
				*dest++ = '=';
				*dest++ = '\r';
				*dest++ = '\n';
				*dest++ = ' ';
				mCurrLineLen = 1;
				mQueuedChars.Prepend( mEncodedWord);
			} else
				goto out;
		}
		while( dest<destEnd && mQueuedChars.Length()) {
			*dest++ =mQueuedChars[0];
			mCurrLineLen++;
			mQueuedChars.Remove( 0, 1);
		}
		if (dest<=destEnd-2) {
			*dest++ = '?';
			*dest++ = '=';
		}
	}
out:
	destLen = dest-destBuf;
	mIsFinalized = (mQueuedChars.Length()==0);
}



/********************************************************************************\
	BmFoldedLineEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFoldedLineEncoder::BmFoldedLineEncoder( BmMemIBuf* input, int lineLen,
														uint32 blockSize, int startAtOffset)
	:	inherited( input, blockSize)
	,	mCurrLineLen( startAtOffset)
	,	mMaxLineLen( lineLen)
	,	mAddQueuedChars( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmFoldedLineEncoder::Filter( const char* srcBuf, uint32& srcLen, 
											 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to line-fold ") << srcLen << " bytes");
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mAddQueuedChars) {
		int corr = (mQueuedChars[mQueuedChars.Length()-1]==' ' ? 0 : 1);
		// if last char is not space, line is full and needs to be folded:
		if (mCurrLineLen + mQueuedChars.Length() + corr > mMaxLineLen) {
			if (dest<=destEnd-3) {
				*dest++ = '\r';
				*dest++ = '\n';
				*dest++ = ' ';
				mCurrLineLen = 1;
			} else
				goto out;
		}
		while( dest<destEnd && mQueuedChars.Length()) {
			*dest++ = mQueuedChars[0];
			mQueuedChars.Remove( 0, 1);
			mCurrLineLen++;
		}
		if (!mQueuedChars.Length())
			mAddQueuedChars = false;
	}
	unsigned char c;
	for( ; src<srcEnd && dest<destEnd && !mAddQueuedChars; ++src) {
		c = *src;
		if (c == ' ') {
			mQueuedChars.Append( ' ', 1);
			mAddQueuedChars = true;
		} else {
			mQueuedChars.Append( c, 1);
			if (mCurrLineLen + mQueuedChars.Length() >= mMaxLineLen)
				mAddQueuedChars = true;		// line is full
		}
	}
out:
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogMailParse, "line-fold: done");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmFoldedLineEncoder::Finalize( char* destBuf, uint32& destLen) {
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	if (mCurrLineLen + mQueuedChars.Length() > mMaxLineLen) {
		if (dest<=destEnd-3) {
			*dest++ = '\r';
			*dest++ = '\n';
			*dest++ = ' ';
			mCurrLineLen = 1;
		} else
			goto out;
	}
	while( dest<destEnd && mQueuedChars.Length()) {
		*dest++ = mQueuedChars[0];
		mQueuedChars.Remove( 0, 1);
		mCurrLineLen++;
	}
out:
	destLen = dest-destBuf;
	mIsFinalized = (mQueuedChars.Length()==0);
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
		-	this code is based on the one used by MDR (MailDaemonReplacement)
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
