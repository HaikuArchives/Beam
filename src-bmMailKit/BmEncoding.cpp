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


#include <string.h>

#include <E-mail.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmUtil.h"

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
	{ "utf8", BM_UTF8_CONVERSION },
	{ NULL, 0 }
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmEncoding::CharsetToEncoding( const BString& charset) {
	BString set( charset);
	if (set.Length())
		// check introduced for Dano compatibility, otherwise "mysterious things"(TM) happen:
		set.ToLower();
	for( int i=0; BM_Encodings[i].charset; ++i)
		if (charset == BM_Encodings[i].charset)
			return( BM_Encodings[i].encoding);
	return B_ISO1_CONVERSION;		// just anything that is compatible with us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::EncodingToCharset( const uint32 encoding) {
	for( int i=0; BM_Encodings[i].charset; ++i)
		if (encoding == BM_Encodings[i].encoding)
			return( BM_Encodings[i].charset);
	return "us-ascii";						// to indicate us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertToUTF8( uint32 srcEncoding, const BString& src,
										  BString& dest) {
	if (srcEncoding == BM_UTF8_CONVERSION) {
		// source already is UTF8...
		dest = src;
		return;
	}
	dest.Truncate(0);
	int32 srcbuflen = src.Length();
	if (!srcbuflen)
		return;
	char* destBuf = NULL;
	int32 state=0;
	int32 buflen = MAX(128,(int32)(srcbuflen*1.5));
	status_t st;

	try {
		for( bool finished=false; !finished; ) {
			if (destBuf)
				buflen *= 2;
			destBuf = dest.LockBuffer( buflen);
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;		// to allow for the delimiting '\0' (see below)
			(st=convert_to_utf8( srcEncoding, src.String(), &srcLen, destBuf, 
										&destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BString("error in convert_to_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
			dest.UnlockBuffer( destLen);
		}
	} catch (...) {
		dest.UnlockBuffer( 0);
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertFromUTF8( uint32 destEncoding, const BString& src, 
											 BString& dest) {
	if (destEncoding == BM_UTF8_CONVERSION) {
		// source already is UTF8...
		dest = src;
		return;
	}
	dest.Truncate( 0);
	char* destBuf = NULL;
	int32 srcbuflen = src.Length();
	if (!srcbuflen)
		return;
	int32 state=0;
	int32 buflen = MAX(128,srcbuflen);
	status_t st;

	try {
		for( bool finished=false; !finished; ) {
			if (destBuf)
				buflen *= 2;
			destBuf = dest.LockBuffer( buflen);
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;
			(st=convert_from_utf8( destEncoding, src.String(), &srcLen, destBuf, &destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BString("error in convert_from_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
			dest.UnlockBuffer( destLen);
		}
	} catch (...) {
		dest.UnlockBuffer( 0);
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Encode( BString encodingStyle, const BString& src, BString& dest, 
								 bool isEncodedWord) {
	int32 srcLen = src.Length();
	encodingStyle.ToUpper();
	dest.Truncate(0);
	const char* safeChars = ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
									? "%&/()?+*,.;:<>-_"
									: "%&/()?+*,.;:<>-_!\"#$@[]\\^'{|}~";
	if (encodingStyle == "Q" || encodingStyle == "QUOTED-PRINTABLE") {
		// quoted printable:
		BM_LOG( BM_LogMailParse, BString("starting to encode quoted-printable of ") << srcLen << " bytes");
		int32 destLen = srcLen*3;
		char* buf = dest.LockBuffer( destLen+1);
		int32 destCount = 0;
		int32 qpLineLen = 0;
		char add[4];
		int32 addLen = 0;
		for( const char* p=src.String(); *p; ++p, addLen=0) {
			unsigned char c = *p;
			if (isalnum(c) || strchr( safeChars, c)) {
				add[addLen++] = c;
			} else if (c == ' ') {
				// in encoded-words, we always replace SPACE by underline:
				if (c == ' ' && isEncodedWord)
					add[addLen++] = '_';
				else {
					// check if whitespace is at end of line (thus needs encoding):
					bool needsEncoding = true;
					for( const char* p2=p+1; *p2; ++p2) {
						if (*p2 == '\r' && *(p2+1)=='\n')
							break;
						if (*p2 != ' ' && *p2 != '\t') {
							needsEncoding = false;
							break;
						}
					}
					if (needsEncoding) {
						add[0] = '='; 
						add[1] = (char)CHAR2HIGHNIBBLE(c); 
						add[2] = (char)CHAR2LOWNIBBLE(c);
						addLen=3;
					} else {
						add[addLen++] = c;
					}
				}
			} else if (c == '\r' && *(p+1) == '\n') {
				buf[destCount++] = '\r';
				buf[destCount++] = '\n';
				qpLineLen = 0;
				p++;								// skip over '\n'
				continue;
			} else if (c == '\n') {
				// we encountered a newline without a preceding <CR>, we add that
				// [in effect converting local newline (LF) to network newline (CRLF)]:
				buf[destCount++] = '\r';
				buf[destCount++] = '\n';
				qpLineLen = 0;
				continue;
			} else {
				add[0] = '='; 
				add[1] = (char)CHAR2HIGHNIBBLE(c); 
				add[2] = (char)CHAR2LOWNIBBLE(c);
				addLen=3;
			}
			if (!isEncodedWord && qpLineLen + addLen >= BM_MAX_LINE_LEN) {
				// insert soft linebreak:
				buf[destCount++] = '=';
				buf[destCount++] = '\r';
				buf[destCount++] = '\n';
				qpLineLen = 0;
			}
			memcpy( buf+destCount, add, addLen);
			destCount += addLen;
			qpLineLen += addLen;
		}
		buf[destCount] = '\0';
		dest.UnlockBuffer( destCount);
		BM_LOG( BM_LogMailParse, "qp-encode: done");
	} else if (encodingStyle == "B" || encodingStyle == "BASE64") {
		// base64:
		BM_LOG( BM_LogMailParse, BString("starting to encode base64 of ") << srcLen << " bytes");
		int32 destLen = srcLen*5/3;		// just to be sure... (need to be a little over 4/3)
		char* buf = dest.LockBuffer( destLen+1);
		destLen = encode_base64( buf, const_cast<char*>(src.String()), src.Length());
		destLen = MAX( 0, destLen);		// if errors should occur we don't want to crash
		buf[destLen] = '\0';
		dest.UnlockBuffer( destLen);
		BM_LOG( BM_LogMailParse, "base64-encode: done");
	} else if (encodingStyle == "7BIT" || encodingStyle == "8BIT") {
		// replace local newline (LF) by network newline (CRLF):
		BM_LOG2( BM_LogMailParse, "7bit/8bit-encode: converting linebreaks");
		ConvertLinebreaksToCRLF( src, dest);
		BM_LOG( BM_LogMailParse, "7bit/8bit-encode: done");
	} else {
		if (encodingStyle != "BINARY") {
			// oops, we don't know this one:
			ShowAlert( BString("Encode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nText will be passed through (not encoded).");
		}
		// no encoding needed/possible (binary or unknown):
		dest = src;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Decode( BString encodingStyle, const BString& src, BString& dest, 
								 bool isEncodedWord, bool isText) {
	encodingStyle.ToUpper();
	dest.Truncate(0);
	Regexx rx;
	if (encodingStyle == "Q" || encodingStyle == "QUOTED-PRINTABLE") {
		// quoted printable:
		BM_LOG( BM_LogMailParse, BString("starting to decode quoted-printable of ") << src.Length() << " bytes");
		int32 destSize = src.Length();
		if (!destSize)
			return;
		char* buf = dest.LockBuffer( destSize);
		const char* pos = src.String();
		char* newPos = buf;
		char c1,c2;
		const BString qpChars("abcdef0123456789ABCDEF");
		while( *pos) {
			if (*pos == '\r' && *(pos+1) == '\n') {
				// make sure to remove any trailing whitespace:
				while(newPos>buf && (*(newPos-1) == ' ' || *(newPos-1) == '\t'))
					newPos--;
				// join lines that have been divided by a soft linebreak:
				if (newPos>buf && *(newPos-1) == '=') {
					newPos--;					// remove '=' from output
					pos++;						// skip '\n'
				}
				pos++;							// skip '\r'
			} else if (isEncodedWord && *pos == '_') {
				// in encoded-words, underlines are really spaces (a real underline is encoded):
				*newPos++ = ' ';
				pos++;
			} else if (*pos == '=' 
						  && (c1=*(pos+1)) && qpChars.FindFirst(c1)!=B_ERROR 
						  && (c2=*(pos+2)) && qpChars.FindFirst(c2)!=B_ERROR) {
				*newPos++ = HEXDIGIT2CHAR(c1)*16 + HEXDIGIT2CHAR(c2);
				pos+=3;
			} else {
				*newPos++ = *pos++;
			}
		}
		*newPos = 0;
		dest.UnlockBuffer( newPos-buf);
		BM_LOG( BM_LogMailParse, "qp-decode: done");
	} else if (encodingStyle == "B" || encodingStyle == "BASE64") {
		// base64:
		BM_LOG( BM_LogMailParse, BString("starting to decode base64 of ") << src.Length() << " bytes");
		bool convertCRs = isText;
		off_t srcSize = src.Length();
		if (!srcSize)
			return;
		char* destBuf = dest.LockBuffer( srcSize);
		int32 destSize = decode_base64( destBuf, const_cast<char*>(src.String()), srcSize, convertCRs);
		destBuf[destSize] = '\0';
		dest.UnlockBuffer( destSize);
		BM_LOG( BM_LogMailParse, "base64-decode: done");
	} else if (encodingStyle == "7BIT" || encodingStyle == "8BIT") {
		// we copy the buffer and replace network newline (CRLF) by local newline (LF):
		BM_LOG2( BM_LogMailParse, "7bit/8bit-decode: converting linebreaks");
		ConvertLinebreaksToLF( src, dest);
		BM_LOG( BM_LogMailParse, "7bit/8bit-decode: done");
	} else {
		if (encodingStyle != "BINARY") {
			// oops, we don't know this one:
			BM_SHOWERR( BString("Decode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo decoding will take place.");
		}
		// we simply copy the buffer, since the encoding type needs no conversion at all:
		dest = src;
	}
}

/*------------------------------------------------------------------------------*\
	DecodedLength()
		-	returns length of given encoded text after it has been decoded
		-	value will be computed, NO actual decoding happening here!
\*------------------------------------------------------------------------------*/
/*
int32 BmEncoding::DecodedLength( const BString& encodingStyle, const char* text, 
											int32 length) {
	if (encodingStyle.ICompare( "Q")==0 || encodingStyle.ICompare("Quoted-Printable")==0) {
		// quoted printable:
		int32 skipCount=0, encodedCount=0;
		int32 pos=0;
		for( const char* s=text; pos<length; s++, pos++) {
			if (*s == '\r') {
				// count trailing whitespace in order to subtract it later:
				const char* s2=s-1;
				while( s2>=s && isspace(*s--))
					skipCount++;
			}
			if (*s=='=' && pos+2<length) {
				if (isalnum(*(s+1)) && isalnum(*(s+2)))
					encodedCount++;
			}
		}
		return length-encodedCount*2-skipCount;
	} else if (encodingStyle.ICompare( "B") == 0 || encodingStyle.ICompare("Base64")==0) {
		// base64:
		int32 count=0;
		int32 padding=0;
		int32 pos=0;
		for( const char* s=text; pos<length; s++, pos++) {
			char c = *s;
			if (!isspace(c))
				count++;
			if (c=='=')
				padding++;
		}
		return count/4*3-padding;
	} else if (encodingStyle.ICompare( "7bit") && encodingStyle.ICompare( "8bit")
				  && encodingStyle.ICompare( "binary")) {
		// oops, we don't know this one:
		BM_SHOWERR( BString("DecodedLength(): Unrecognized encoding-style <")<<encodingStyle<<"> found.");
	}
	return length;
}
*/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::ConvertHeaderPartToUTF8( const BString& headerPart, 
															uint32 defaultEncoding) {
	int32 nm;
	Regexx rx;
	rx.expr( "=\\?(.+?)\\?(.)\\?(.+?)\\?=\\s*");
	rx.str( headerPart);
	BString utf8;
	
	if ((nm = rx.exec( Regexx::global))) {
		int32 len=headerPart.Length();
		int32 curr=0;
		vector<RegexxMatch>::const_iterator i;
		for( i = rx.match.begin(); i != rx.match.end(); ++i) {
			if (curr < i->start()) {
				// copy the characters between start/curr match and first match:
				utf8.Append( headerPart.String()+curr, i->start()-curr);
			}
			// convert the match (an encoded word) into UTF8:
			const BString srcCharset( i->atom[0]);
			const BString srcQuotingStyle( i->atom[1]);
			const BString text( i->atom[2]);
			BString decodedData;
			Decode( srcQuotingStyle, text, decodedData, true, true);
			if (decodedData.Length()) {
				BString decodedAsUTF8;
				ConvertToUTF8( CharsetToEncoding(srcCharset), decodedData, decodedAsUTF8);
				utf8 << decodedAsUTF8;
			}
			curr = i->start()+i->Length();
		}
		if (curr<len) {
			utf8.Append( headerPart.String()+curr, len-curr);
		}
	} else {
		ConvertToUTF8( defaultEncoding, headerPart, utf8);
	}
	return utf8;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::ConvertUTF8ToHeaderPart( const BString& utf8text, uint32 encoding,
															bool useQuotedPrintableIfNeeded,
															bool fold, int32 fieldLen) {
	bool needsQuotedPrintable = false;
	BString charsetString;
	ConvertFromUTF8( encoding, utf8text, charsetString);
	if (useQuotedPrintableIfNeeded)
		needsQuotedPrintable = NeedsEncoding( charsetString);
	BString charset = EncodingToCharset( encoding);
	BString encodedString;
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
			BString foldedString;
			while( encodedString.Length() > maxChars) {
				BString tmp;
				int32 foldPos = maxChars;
				int32 lastEqualSignPos = encodedString.FindLast( '=', foldPos);
				if (lastEqualSignPos != B_ERROR && foldPos - lastEqualSignPos < 3) {
					// we avoid folding within an encoded char (=XX) by moving the
					// folding point just before the equal-sign:
					foldPos = lastEqualSignPos;
				}
				encodedString.MoveInto( tmp, 0, foldPos);
				foldedString << "=?" << charset << "?q?" << tmp << "?=\r\n";
				foldedString.Append( BM_SPACES, fieldLen+2);
			}
			foldedString << "=?" << charset << "?q?" << encodedString << "?=";
			return foldedString;		
		} else
			return BString("=?") + charset + "?q?" + encodedString + "?=";
	} else {
		// simpler case, no encoded-words neccessary:
		Encode( "7BIT", charsetString, encodedString, true);
		int32 maxChars = BM_MAX_LINE_LEN		// 76 chars maximum
							- (fieldLen + 2);		// space used by "fieldname: "
		if (fold && encodedString.Length() > maxChars) {
			// fold header-line, since it is too long:
			BString foldedString;
			while( encodedString.Length() > maxChars) {
				int32 foldPos = encodedString.FindLast( ' ', maxChars-1);
				if (foldPos == B_ERROR)
					foldPos = maxChars;
				else
					foldPos++;					// leave space as last char on curr line
				BString tmp;
				encodedString.MoveInto( tmp, 0, foldPos);
				foldedString << tmp << "\r\n";
				foldedString.Append( BM_SPACES, fieldLen+2);
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
bool BmEncoding::NeedsEncoding( const BString& charsetString) {
	// check if string needs quoted-printable/base64 encoding
	// (which it does if it contains non-ASCII chars):
	for( const char* p = charsetString.String(); *p; ++p) {
		if (*p<32 && *p!='\r' && *p!='\n')
			// this is a signed char, so c<32 means [0-31] and [128-255]
			return true;
	}
	return false;
}
