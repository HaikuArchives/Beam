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

#include <UTF8.h>
#include "base64.h"

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
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
	dest.Truncate(0);
	int32 srcbuflen = src.Length();
	if (!srcbuflen)
		return;
	char* destBuf = NULL;
	int32 state=0;
	int32 lastSrcLen=-1;
	int32 buflen = MAX(128,(int32)(srcbuflen*1.5));
	status_t st;

	try {
		for( bool finished=false; !finished; ) {
			if (destBuf)
				buflen *= 2;
			(destBuf = dest.LockBuffer( buflen))
														|| BM_THROW_RUNTIME( "ConvertToUTF8(): unable to lock buffer");
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;		// to allow for the delimiting '\0' (see below)
			(st=convert_to_utf8( srcEncoding, src.String(), &srcLen, destBuf, 
										&destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BmString("error in convert_to_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen || lastSrcLen == srcLen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
			lastSrcLen = srcLen;
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
void BmEncoding::ConvertFromUTF8( uint32 destEncoding, const BmString& src, 
											 BmString& dest) {
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
	int32 lastSrcLen=-1;
	int32 buflen = MAX(128,srcbuflen);
	status_t st;

	try {
		for( bool finished=false; !finished; ) {
			if (destBuf)
				buflen *= 2;
			(destBuf = dest.LockBuffer( buflen))
														|| BM_THROW_RUNTIME( "ConvertFromUTF8(): unable to lock buffer");
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;
			(st=convert_from_utf8( destEncoding, src.String(), &srcLen, destBuf, &destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BmString("error in convert_from_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen || lastSrcLen == srcLen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
			lastSrcLen = srcLen;
			dest.UnlockBuffer( destLen);
		}
	} catch (...) {
		if (destBuf)
			dest.UnlockBuffer( 0);
		throw;
	}
}

void BmEncoding::Encode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord) {
	int32 srcLen = src.Length();
	dest.Truncate(0);
	const char* safeChars = 
		isEncodedWord 
			? (ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
					? "%&/()?+*,.;:<>-"
					: "%&/()?+*,.;:<>-!\"#$@[]\\^'{|}~")
							// in encoded words, underscore has to be encoded, since it
							// is used for spaces!
			: (ThePrefs->GetBool( "MakeQPSafeForEBCDIC", false)
					? "%&/()?+*,.;:<>-_"
					: "%&/()?+*,.;:<>-_!\"#$@[]\\^'{|}~");
							// in bodies, the underscore is safe, i.e. it need not be encoded.
	if (encodingStyle.ICompare("q")==0 || encodingStyle.ICompare("quoted-printable")==0) {
		// quoted printable:
		BM_LOG( BM_LogMailParse, BmString("starting to encode quoted-printable of ") << srcLen << " bytes");
		BmStrOStream tempIO( MAX(128,srcLen*1.2), 1.2);
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
				tempIO.Write( "\r\n", 2);
				qpLineLen = 0;
				p++;								// skip over '\n'
				continue;
			} else if (c == '\n') {
				// we encountered a newline without a preceding <CR>, we add that
				// [in effect converting local newline (LF) to network newline (CRLF)]:
				tempIO.Write( "\r\n", 2);
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
				tempIO.Write( "=\r\n", 3);
				qpLineLen = 0;
			}
			tempIO.Write( add, addLen);
			qpLineLen += addLen;
		}
		dest.Adopt( tempIO.TheString());
		BM_LOG( BM_LogMailParse, "qp-encode: done");
	} else if (encodingStyle.ICompare("b")==0 || encodingStyle.ICompare("base64")==0) {
		// base64:
		BM_LOG( BM_LogMailParse, BmString("starting to encode base64 of ") << srcLen << " bytes");
		int32 destLen = srcLen*5/3;		// just to be sure... (need to be a little over 4/3)
		char* buf = dest.LockBuffer( destLen);
		destLen = encode64( buf, src.String(), src.Length());
		destLen = MAX( 0, destLen);		// if errors should occur we don't want to crash
		buf[destLen] = '\0';
		dest.UnlockBuffer( destLen);
		BM_LOG( BM_LogMailParse, "base64-encode: done");
	} else if (encodingStyle.ICompare("7bit")==0 || encodingStyle.ICompare("8bit")==0) {
		// replace local newline (LF) by network newline (CRLF):
		BM_LOG2( BM_LogMailParse, "7bit/8bit-encode: converting linebreaks");
		dest.ConvertLinebreaksToCRLF( &src);
		BM_LOG( BM_LogMailParse, "7bit/8bit-encode: done");
	} else {
		if (encodingStyle.ICompare("binary")!=0) {
			// oops, we don't know this one:
			ShowAlert( BmString("Encode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nText will be passed through (not encoded).");
		}
		// no encoding needed/possible (binary or unknown):
		dest = src;
	}
}

void BmEncoding::Decode( BmString encodingStyle, const BmString& src, BmString& dest, 
								 bool isEncodedWord) {
	dest.Truncate(0);
	Regexx rx;
	if (encodingStyle.ICompare("q")==0 || encodingStyle.ICompare("quoted-printable")==0) {
		// quoted printable:
		BM_LOG( BM_LogMailParse, BmString("starting to decode quoted-printable of ") << src.Length() << " bytes");
		int32 destSize = src.Length();
		if (!destSize)
			return;
		char* buf = dest.LockBuffer( destSize);
		const char* pos = src.String();
		char* newPos = buf;
		char* lastSoftbreakPos = NULL;
		char c1,c2;
		const BmString qpChars("abcdef0123456789ABCDEF");
		while( *pos) {
			if (*pos == '\r' && *(pos+1) == '\n') {
				// make sure to remove any trailing whitespace:
				while(newPos>buf && (*(newPos-1) == ' ' || *(newPos-1) == '\t'))
					newPos--;
				// join lines that have been divided by a soft linebreak:
				if (lastSoftbreakPos>0) {
					newPos = lastSoftbreakPos;		// move back to '='-character
					lastSoftbreakPos = 0;
					pos++;						// skip '\n', too
				}
				pos++;							// skip '\r'
			} else if (isEncodedWord && *pos == '_') {
				// in encoded-words, underlines are really spaces (a real underline is encoded):
				*newPos++ = ' ';
				pos++;
			} else if (*pos == '=') {
				if ((c1=*(pos+1)) && qpChars.FindFirst(c1)!=B_ERROR 
				&& (c2=*(pos+2)) && qpChars.FindFirst(c2)!=B_ERROR) {
					*newPos++ = HEXDIGIT2CHAR(c1)*16 + HEXDIGIT2CHAR(c2);
					pos+=3;
				} else {
					// softbreak encountered, we keep note of it's position
					lastSoftbreakPos = newPos;
					*newPos++ = *pos++;
				}
			} else {
				*newPos++ = *pos++;
			}
		}
		*newPos = 0;
		dest.UnlockBuffer( newPos-buf);
		BM_LOG( BM_LogMailParse, "qp-decode: done");
	} else if (encodingStyle.ICompare("b")==0 || encodingStyle.ICompare("base64")==0) {
		// base64:
		BM_LOG( BM_LogMailParse, BmString("starting to decode base64 of ") << src.Length() << " bytes");
		off_t srcSize = src.Length();
		if (!srcSize)
			return;
		char* destBuf = dest.LockBuffer( srcSize);
		ssize_t destSize = decode64( destBuf, src.String(), srcSize);
		if (destSize<0) {
			BM_LOG( BM_LogMailParse, BmString("Unable to decode base64-string. Error: ") << strerror(destSize));
			destSize=0;
		}
		destBuf[destSize] = '\0';
		dest.UnlockBuffer( destSize);
		BM_LOG( BM_LogMailParse, "base64-decode: done");
	} else if (encodingStyle.ICompare("7bit")==0 || encodingStyle.ICompare("8bit")==0) {
		// we copy the buffer and replace network newline (CRLF) by local newline (LF):
		BM_LOG2( BM_LogMailParse, "7bit/8bit-decode: converting linebreaks");
		dest.ConvertLinebreaksToLF( &src);
		BM_LOG( BM_LogMailParse, "7bit/8bit-decode: done");
	} else {
		if (encodingStyle.ICompare("binary")!=0) {
			// oops, we don't know this one:
			BM_SHOWERR( BmString("Decode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo decoding will take place.");
		}
		// we simply copy the buffer, since the encoding type needs no conversion at all:
		dest = src;
	}
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
	BmString utf8;
	
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
			const BmString srcCharset( i->atom[0]);
			const BmString srcQuotingStyle( i->atom[1]);
			const BmString text( i->atom[2]);
			BmString decodedData;
			Decode( srcQuotingStyle, text, decodedData, true);
			if (decodedData.Length()) {
				BmString decodedAsUTF8;
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
BmString BmEncoding::ConvertUTF8ToHeaderPart( const BmString& utf8text, uint32 encoding,
															bool useQuotedPrintableIfNeeded,
															bool fold, int32 fieldLen) {
	bool needsQuotedPrintable = false;
	BmString charsetString;
	BmString transferEncoding = "7bit";
	ConvertFromUTF8( encoding, utf8text, charsetString);
	if (useQuotedPrintableIfNeeded)
		needsQuotedPrintable = NeedsEncoding( charsetString);
	if (ThePrefs->GetBool( "Allow8BitMimeInHeader", false) && needsQuotedPrintable) {
		transferEncoding = "8bit";
		needsQuotedPrintable = false;
	}
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
				foldedString << "=?" << charset << "?q?" << tmp << "?=\r\n";
				foldedString.Append( BM_SPACES, fieldLen+2);
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
				else
					foldPos++;					// leave space as last char on curr line
				BmString tmp;
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
bool BmEncoding::NeedsEncoding( const BmString& charsetString) {
	// check if string needs quoted-printable/base64 encoding
	// (which it does if it contains non-ASCII chars):
	for( const char* p = charsetString.String(); *p; ++p) {
		if (*p<32 && *p!='\r' && *p!='\n')
			// this is a signed char, so c<32 means [0-31] and [128-255]
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



/********************************************************************************\
	BmQuotedPrintableDecoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableDecoder::BmQuotedPrintableDecoder( BmMemIBuf& input, 
																	 bool isEncodedWord, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
	,	mIsEncodedWord( isEncodedWord)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmQuotedPrintableDecoder::DetermineBufferFixup( BmMemIBuf& input) {
	BmString peekStr = input.Peek( 2, -2);
	int32 pos = peekStr.FindFirst( '=');
	return pos != B_ERROR ? pos+1 : 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableDecoder::DoFilter( const char* srcBuf, uint32& srcLen, 
													  char* destBuf, uint32& destLen) {
	BM_LOG( BM_LogMailParse, BmString("starting to decode quoted-printable of ") << srcLen << " bytes");
	const char* pos = srcBuf;
	char* srcEnd = destBuf+srcLen;
	char* newPos = destBuf;
	char* destEnd = destBuf+destLen;
	char* lastSoftbreakPos = NULL;
	char c1,c2;
	const BmString qpChars("abcdef0123456789ABCDEF");
	while( pos<srcEnd && newPos<destEnd && *pos) {
		if (*pos == '\r' && *(pos+1) == '\n') {
			// make sure to remove any trailing whitespace:
			while(newPos>destBuf && (*(newPos-1) == ' ' || *(newPos-1) == '\t'))
				newPos--;
			// join lines that have been divided by a soft linebreak:
			if (lastSoftbreakPos>0) {
				newPos = lastSoftbreakPos;		// move back to '='-character
				lastSoftbreakPos = 0;
				pos++;						// skip '\n', too
			}
			pos++;							// skip '\r'
		} else if (mIsEncodedWord && *pos == '_') {
			// in encoded-words, underlines are really spaces (a real underline is encoded):
			*newPos++ = ' ';
			pos++;
		} else if (*pos == '=') {
			if ((c1=*(pos+1)) && qpChars.FindFirst(c1)!=B_ERROR 
			&& (c2=*(pos+2)) && qpChars.FindFirst(c2)!=B_ERROR) {
				*newPos++ = HEXDIGIT2CHAR(c1)*16 + HEXDIGIT2CHAR(c2);
				pos+=3;
			} else {
				// softbreak encountered, we keep note of it's position
				lastSoftbreakPos = newPos;
				*newPos++ = *pos++;
			}
		} else {
			*newPos++ = *pos++;
		}
	}
	srcLen = max( 0, (int)(pos-srcBuf));
	destLen = max( 0, (int)(newPos-destBuf));
	BM_LOG( BM_LogMailParse, "qp-decode: done");
}



/********************************************************************************\
	BmQuotedPrintableEncoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmQuotedPrintableEncoder::BmQuotedPrintableEncoder( BmMemIBuf& input, 
																	 bool isEncodedWord, 
																	 uint32 blockSize)
	:	inherited( input, blockSize)
	,	mIsEncodedWord( isEncodedWord)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmQuotedPrintableEncoder::DoFilter( const char* srcBuf, uint32& srcLen, 
													  char* destBuf, uint32& destLen) {
	BM_LOG( BM_LogMailParse, BmString("starting to encode quoted-printable of ") << srcLen << " bytes");
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
	int32 qpLineLen = 0;
	char add[4];
	int32 addLen = 0;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
	for( const char* p=srcBuf; p<srcEnd && dest<destEnd && *p; ++p, addLen=0) {
		unsigned char c = *p;
		if (isalnum(c) || strchr( safeChars, c)) {
			add[addLen++] = c;
		} else if (c == ' ') {
			// in encoded-words, we always replace SPACE by underline:
			if (c == ' ' && mIsEncodedWord)
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
		} else if ((c=='\r' && *(p+1)=='\n') || c=='\n') {
			if (dest+2>=destEnd)
				break;
			*dest++ = '\r';
			*dest++ = '\n';
			qpLineLen = 0;
			if (c=='\n')
				p++;								// skip over '\n'
			continue;
		} else {
			add[0] = '='; 
			add[1] = (char)CHAR2HIGHNIBBLE(c); 
			add[2] = (char)CHAR2LOWNIBBLE(c);
			addLen=3;
		}
		if (!mIsEncodedWord && qpLineLen + addLen >= BM_MAX_LINE_LEN) {
			if (dest+addLen+3>=destEnd)
				break;
			// insert soft linebreak:
			*dest++ = '=';
			*dest++ = '\r';
			*dest++ = '\n';
			qpLineLen = 0;
		}
		for( int i=0; i<addLen; ++i)
			*dest++ = add[i];
		qpLineLen += addLen;
	}
	BM_LOG( BM_LogMailParse, "qp-encode: done");
}



/********************************************************************************\
	BmBase64Decoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmBase64Decoder::DetermineBufferFixup( BmMemIBuf& input) {
	BmString peekStr = input.Peek( 100, -2);
	int32 pos = peekStr.FindFirst( "\r\n");
	return pos != B_ERROR ? pos : 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Decoder::DoFilter( const char* srcBuf, uint32& srcLen, 
										  char* destBuf, uint32& destLen) {
	uint32 concat = 0;
	int32 value;
	
	static int32 base64_alphabet[256] = { //----Fast lookup table
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
		 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
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

	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;
		
	while( src+(3-mIndex)<srcEnd && dest<=destEnd-3) {
		if ((value = base64_alphabet[(unsigned char)*src++])==-1)
			continue;
			
		concat |= (((uint32)value) << ((3-mIndex)*6));
		
		if (++mIndex == 4) {
			*dest++ = (concat & 0x00ff0000) >> 16;
			*dest++ = (concat & 0x0000ff00) >> 8;
			*dest++ = (concat & 0x000000ff);
			concat = mIndex = 0;
		}
	}
	
	srcLen = src-srcBuf;
	destLen = dest-destBuf;
}



/********************************************************************************\
	BmBase64Encoder
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBase64Encoder::DoFilter( const char* in, uint32& srcLen, 
										  char* out, uint32& destLen) {
	const int BASE64_LINELENGTH = 76;

	static char base64_alphabet[64] = { //----Fast lookup table
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'+',
		'/'
	};
 
	unsigned long concat;
	uint32 i = 0;
	uint32 k = 0;
	uint32 v;
	while ( i<srcLen && k<=destLen-6) {
		concat = ((in[i] & 0xff) << 16);
		if ((i+1) < srcLen)
			concat |= ((in[i+1] & 0xff) << 8);
		if ((i+2) < srcLen)
			concat |= (in[i+2] & 0xff);
		i += 3;
				
		out[k++] = base64_alphabet[(concat >> 18) & 63];
		out[k++] = base64_alphabet[(concat >> 12) & 63];
		out[k++] = base64_alphabet[(concat >> 6) & 63];
		out[k++] = base64_alphabet[concat & 63];

		if (i >= srcLen) {
			for (v = 0; v <= (i - srcLen); v++)
				out[k-v] = '=';
		}

		mCurrLineLen += 4;
		if (mCurrLineLen >= BASE64_LINELENGTH) {
			out[k++] = '\r';
			out[k++] = '\n';
			mCurrLineLen = 4;
		}
	}
	srcLen = i;
	destLen = k;
}

