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


#include <assert.h>
#include <cstring>

#include <UTF8.h>

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

BmEncoding::BmEncodingPair BmEncoding::BM_Encodings[] = {
	{ "iso-8859-1", B_ISO1_CONVERSION },
	{ "iso-8859-2", B_ISO2_CONVERSION },
	{ "iso-8859-3", B_ISO3_CONVERSION },
	{ "iso-8859-4", B_ISO4_CONVERSION },
	{ "iso-8859-5", B_ISO5_CONVERSION },
	{ "iso-8859-6", B_ISO6_CONVERSION },
	{ "iso-8859-7", B_ISO7_CONVERSION },
	{ "iso-8859-8", B_ISO8_CONVERSION },
	{ "iso-8859-9", B_ISO9_CONVERSION },
	{ "iso-8859-10", B_ISO10_CONVERSION },
	{ "macintosh", B_MAC_ROMAN_CONVERSION },
	{ "windows-1252", B_MS_WINDOWS_CONVERSION },
	{ "windows-1251", B_MS_WINDOWS_1251_CONVERSION },
	{ "ibm866", B_MS_DOS_866_CONVERSION },
	{ "ibm850", B_MS_DOS_CONVERSION },
	{ "shift_jis", B_SJIS_CONVERSION },
	{ "euc-jp", B_EUC_CONVERSION },
	{ "iso-2022-jp", B_JIS_CONVERSION },
	{ "koi8-r", B_KOI8R_CONVERSION },
	{ "euc-kr", B_EUC_KR_CONVERSION },
	{ "iso-8859-13", B_ISO13_CONVERSION },
	{ "iso-8859-14", B_ISO14_CONVERSION },
	{ "iso-8859-15", B_ISO15_CONVERSION },
	{ "utf-8", BM_UTF8_CONVERSION },
	{ "utf8", BM_UTF8_CONVERSION },
	{ NULL, 0 }
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmEncoding::CharsetToEncoding( const BmString& charset) {
	BmString set( charset);
	set.ToLower();
	for( int i=0; BM_Encodings[i].charset; ++i)
		if (set == BM_Encodings[i].charset)
			return( BM_Encodings[i].encoding);
	return B_ISO1_CONVERSION;		// just anything that is compatible with us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmEncoding::EncodingToCharset( const uint32 encoding) {
	for( int i=0; BM_Encodings[i].charset; ++i)
		if (encoding == BM_Encodings[i].encoding)
			return( BM_Encodings[i].charset);
	return "us-ascii";						// to indicate us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertToUTF8( uint32 srcEncoding, const BmString& src,
										  BmString& dest) {
	if (srcEncoding == BM_UTF8_CONVERSION) {
		// source already is UTF8...
		dest = src;
		return;
	}
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmUtf8Encoder encoder( &srcBuf, srcEncoding, blockSize);
	destBuf.Write( &encoder, blockSize);
	dest.Adopt( destBuf.TheString());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertFromUTF8( uint32 destEncoding, const BmString& src, 
											 BmString& dest) {
	if (destEncoding == BM_UTF8_CONVERSION) {
		// destination shall be UTF8, too...
		dest = src;
		return;
	}
	BmStringIBuf srcBuf( src);
	const uint32 blockSize = max( (int32)128, src.Length());
	BmStringOBuf destBuf( blockSize);
	BmUtf8Decoder encoder( &srcBuf, destEncoding, blockSize);
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
															 uint32 defaultEncoding) {
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
			BmUtf8Encoder textConverter( decoder.get(), CharsetToEncoding( srcCharset), blockSize);
			utf8.Write( &textConverter, blockSize);
			curr = i->start()+i->Length();
		}
		if (curr<len) {
			utf8.Write( headerPart.String()+curr, len-curr);
		}
	} else {
		BmStringIBuf text( headerPart);
		BmUtf8Encoder textConverter( &text, defaultEncoding);
		utf8.Write( &textConverter);
	}
	return utf8.TheString();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmEncoding::ConvertUTF8ToHeaderPart( const BmString& utf8Text, uint32 encoding,
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
	ConvertFromUTF8( encoding, utf8Text, charsetString);
	BmString charset = EncodingToCharset( encoding);
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
		if (fold && encodedString.Length() > maxChars) {
			// fold header-line, since it is too long:
			BmString foldedString;
			while( encodedString.Length() > maxChars) {
				int32 foldPos = encodedString.FindLast( ' ', maxChars-1);
				if (foldPos == B_ERROR)
					foldPos = maxChars;
				BmString tmp;
				encodedString.MoveInto( tmp, 0, foldPos);
				foldedString << tmp << "\r\n ";
				// now compute maxChars for lines without the leading fieldname:
				maxChars = BM_MAX_LINE_LEN			// 76 chars maximum
								- 1;						// a single space
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
	else {
		if (encodingStyle.ICompare("binary")!=0) {
			BM_SHOWERR( BmString("FindDecoderFor(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo decoding will take place.");
		}
		filter = new BmBinaryDecoder( input, blockSize);
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
	else {
		if (encodingStyle.ICompare("binary")!=0) {
			BM_SHOWERR( BmString("FindDecoderFor(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo decoding will take place.");
		}
		filter = new BmBinaryEncoder( input, blockSize);
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
BmUtf8Decoder::BmUtf8Decoder( BmMemIBuf* input, uint32 destEncoding, 
										uint32 blockSize)
	:	inherited( input, blockSize)
	,	mDestEncoding( destEncoding)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Decoder::Filter( const char* srcBuf, uint32& srcLen, 
									 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to decode utf8 of ") << srcLen << " bytes");

	if (mDestEncoding == BM_UTF8_CONVERSION) {
		// destination shall be UTF8, too...
		uint32 size = min( srcLen, destLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
		return;
	}

	int32 state = 0;
	status_t err = convert_from_utf8( mDestEncoding, srcBuf, (int32*)&srcLen, 
												 destBuf, (int32*)&destLen, &state);
	if (err != B_OK) {
		BM_LOG( BM_LogMailParse, BmString("error in utf8-decode: ") << strerror(err));
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
BmUtf8Encoder::BmUtf8Encoder( BmMemIBuf* input, uint32 srcEncoding, 
										uint32 blockSize)
	:	inherited( input, blockSize)
	,	mSrcEncoding( srcEncoding)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmUtf8Encoder::Filter( const char* srcBuf, uint32& srcLen, 
									 char* destBuf, uint32& destLen) {
	BM_LOG3( BM_LogMailParse, BmString("starting to encode utf8 of ") << srcLen << " bytes");

	if (mSrcEncoding == BM_UTF8_CONVERSION) {
		// source already is UTF8...
		uint32 size = min( srcLen, destLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
		return;
	}

	int32 state = 0;
	status_t err = convert_to_utf8( mSrcEncoding, srcBuf, (int32*)&srcLen, 
											  destBuf, (int32*)&destLen, &state);
	if (err != B_OK) {
		BM_LOG( BM_LogMailParse, BmString("error in utf8-encode: ") << strerror(err));
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
		assert( dest<=destEnd-3);
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
