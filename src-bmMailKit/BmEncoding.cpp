/*
	BmEncoding.cpp
		$Id$
*/

#include <string.h>

#include <E-mail.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
#include "BmLogHandler.h"
#include "BmUtil.h"

#define HEXDIGIT2CHAR(d) (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0)

const char* BmEncoding::BM_Encodings[] = {
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
	"Macintosh",
	"SHIFT_JIS",
	"EUC-JP",
	"ISO-2022-JP",
	"WINDOWS-1252",
	"KOI8-R",
	"WINDOWS-1251",
	"IBM866",
	"IBM850",
	"EUC-KR",
	"ISO-8859-13",
	"ISO-8859-14",
	"ISO-8859-15",
	NULL
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmEncoding::CharsetToEncoding( const BString& charset) {
	BString set( charset);
	set.ToUpper();
	if (set == "ISO-8859-1") 		return( B_ISO1_CONVERSION);
	if (set == "ISO-8859-2") 		return( B_ISO2_CONVERSION);
	if (set == "ISO-8859-3") 		return( B_ISO3_CONVERSION);
	if (set == "ISO-8859-4") 		return( B_ISO4_CONVERSION);
	if (set == "ISO-8859-5") 		return( B_ISO5_CONVERSION);
	if (set == "ISO-8859-6") 		return( B_ISO6_CONVERSION);
	if (set == "ISO-8859-7") 		return( B_ISO7_CONVERSION);
	if (set == "ISO-8859-8") 		return( B_ISO8_CONVERSION);
	if (set == "ISO-8859-9") 		return( B_ISO9_CONVERSION);
	if (set == "ISO-8859-10") 		return( B_ISO10_CONVERSION);
/*	
	[zooey]:	does not seem to be registered with MIME ?!? */
	if (set == "macintosh") 		return( B_MAC_ROMAN_CONVERSION);
	if (set == "SHIFT_JIS") 		return( B_SJIS_CONVERSION);
	if (set == "EUC-JP") 			return( B_EUC_CONVERSION);
	if (set == "ISO-2022-JP")	 	return( B_JIS_CONVERSION);
	if (set == "WINDOWS-1252") 	return( B_MS_WINDOWS_CONVERSION);
	if (set == "KOI8-R") 			return( B_KOI8R_CONVERSION);
	if (set == "WINDOWS-1251") 	return( B_MS_WINDOWS_1251_CONVERSION);
	if (set == "IBM866") 			return( B_MS_DOS_866_CONVERSION);
	if (set == "IBM850") 			return( B_MS_DOS_CONVERSION);
	if (set == "EUC-KR") 			return( B_EUC_KR_CONVERSION);
	if (set == "ISO-8859-13") 		return( B_ISO13_CONVERSION);
	if (set == "ISO-8859-14") 		return( B_ISO14_CONVERSION);
	if (set == "ISO-8859-15") 		return( B_ISO15_CONVERSION);

	if (set == "UTF8") 				return( BM_UTF8_CONVERSION);

	return B_ISO1_CONVERSION;		// just anything that is compatible with us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::EncodingToCharset( const uint32 encoding) {
	if (encoding == B_ISO1_CONVERSION) 		return( "ISO-8859-1");
	if (encoding == B_ISO2_CONVERSION) 		return( "ISO-8859-2");
	if (encoding == B_ISO3_CONVERSION) 		return( "ISO-8859-3");
	if (encoding == B_ISO4_CONVERSION) 		return( "ISO-8859-4");
	if (encoding == B_ISO5_CONVERSION) 		return( "ISO-8859-5");
	if (encoding == B_ISO6_CONVERSION) 		return( "ISO-8859-6");
	if (encoding == B_ISO7_CONVERSION) 		return( "ISO-8859-7");
	if (encoding == B_ISO8_CONVERSION) 		return( "ISO-8859-8");
	if (encoding == B_ISO9_CONVERSION) 		return( "ISO-8859-9");
	if (encoding == B_ISO10_CONVERSION) 	return( "ISO-8859-10");
/*	
	[zooey]:	although it does not seem to be registered with MIME ?!? */
	if (encoding == B_MAC_ROMAN_CONVERSION)	return( "macintosh");
	if (encoding == B_SJIS_CONVERSION) 		return( "SHIFT_JIS");
	if (encoding == B_EUC_CONVERSION) 		return( "EUC-JP");
	if (encoding == B_JIS_CONVERSION)	 	return( "ISO-2022-JP");
	if (encoding == B_MS_WINDOWS_CONVERSION) 	return( "WINDOWS-1252");
	if (encoding == B_KOI8R_CONVERSION) 	return( "KOI8-R");
	if (encoding == B_MS_WINDOWS_1251_CONVERSION) 	return( "WINDOWS-1251");
	if (encoding == B_MS_DOS_866_CONVERSION)	return( "IBM866");
	if (encoding == B_MS_DOS_CONVERSION) 	return( "IBM850");
	if (encoding == B_EUC_KR_CONVERSION) 	return( "EUC-KR");
	if (encoding == B_ISO13_CONVERSION) 	return( "ISO-8859-13");
	if (encoding == B_ISO14_CONVERSION) 	return( "ISO-8859-14");
	if (encoding == B_ISO15_CONVERSION) 	return( "ISO-8859-15");

	if (encoding == BM_UTF8_CONVERSION) 		return( "UTF8");

	return "US-ASCII";		// to indicate us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::ConvertToUTF8( uint32 srcEncoding, const BString& src,
										  BString& dest) {
	dest.Truncate(0);
	if (srcEncoding == BM_UTF8_CONVERSION)
		return;									// source already is UTF8, we do nothing
	int32 srcbuflen = src.Length();
	if (!srcbuflen)
		return;
	char* destBuf = NULL;
	int32 state=0;
	int32 buflen = srcbuflen>92 ? (int32)(srcbuflen*1.5) : 128;
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
	dest.Truncate( 0);
	if (destEncoding == BM_UTF8_CONVERSION)
		return;									// nothing to do!
	char* destBuf = NULL;
	int32 srcbuflen = src.Length();
	if (!srcbuflen)
		return;
	int32 state=0;
	int32 buflen = srcbuflen;
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
	dest.Truncate(0,false);
	if (encodingStyle == "Q" || encodingStyle == "QUOTED-PRINTABLE") {
		// quoted printable:
		dest = src;
	} else if (encodingStyle == "B" || encodingStyle == "BASE64") {
		// base64:
		int32 destLen = srcLen*5/3;		// just to be sure... (need to be a little over 4/3)
		char* buf = dest.LockBuffer( destLen+1);
		destLen = encode_base64( buf, const_cast<char*>(src.String()), src.Length());
		destLen = MAX( 0, destLen);		// if errors should occur we don't want to crash
		buf[destLen] = '\0';
		dest.UnlockBuffer( destLen);
	} else {
		if (!(encodingStyle == "7BIT" || encodingStyle == "8BIT" || encodingStyle == "BINARY")) {
			// oops, we don't know this one:
			ShowAlert( BString("Encode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nText will be passed through (not encoded).");
		}
		// no encoding (7bit, or 8bit)
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
	dest.Truncate(0,false);
	if (encodingStyle == "Q" || encodingStyle == "QUOTED-PRINTABLE") {
		// quoted printable:
		Regexx rx;
		// remove trailing whitespace from all lines (may have been added during mail-transport):
		BString text = rx.replace( src, "[\\t ]+(?=\\r\\n)", "", Regexx::newline | Regexx::global);
		// join together lines that end with a softbreak:
		text = rx.replace( text, "=\\r\\n", "", Regexx::newline | Regexx::global);
		if (isEncodedWord) {
			// in encoded-words, underlines are really spaces (a real underline is encoded):
			text.ReplaceAll( "_", " ");
		}
		// now we decode the quoted printables:
		rx.expr( "=([0-9A-F][0-9A-F])");
		rx.str( text);
		int32 len=text.Length();
		const char* buf = text.String();
		int32 nm;
		
		if ((nm = rx.exec( Regexx::global | Regexx::nocase))) {
			unsigned char ch;
			char* destBuf = dest.LockBuffer( len+1);
			int32 neg_offs=0;
			int32 curr=0;
			vector<RegexxMatch>::const_iterator i;
			for( i = rx.match.begin(); i != rx.match.end(); ++i) {
				int32 pos = i->start();
				if (curr<pos) {
					memcpy( destBuf+curr-neg_offs, buf+curr, pos-curr);
				}
				ch = HEXDIGIT2CHAR(buf[pos+1])*16 + HEXDIGIT2CHAR(buf[pos+2]);
				destBuf[pos-neg_offs] = ch;
				curr = pos+3;
				neg_offs += 2;
			}
			if (curr<len) {
				memcpy( destBuf+curr-neg_offs, buf+curr, len-curr);
			}
			destBuf[len-neg_offs] = '\0';
			dest.UnlockBuffer( len-neg_offs);
		} else {
			dest = text;
		}
	} else if (encodingStyle == "B" || encodingStyle == "BASE64") {
		// base64:
		bool convertCRs = isText;
		off_t srcSize = src.Length();
		if (!srcSize)
			return;
		char* destBuf = dest.LockBuffer( srcSize);
		int32 destSize = decode_base64( destBuf, const_cast<char*>(src.String()), srcSize, convertCRs);
		dest[destSize] = '\0';
		dest.UnlockBuffer( destSize);
	} else {
		if (!(encodingStyle == "7BIT" || encodingStyle == "8BIT" || encodingStyle == "BINARY")) {
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
															int32 defaultEncoding) {
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
BString BmEncoding::ConvertUTF8ToHeaderPart( const BString& utf8text, int32 encoding) {
//	BString quotedPrintable = Encode( );
	
	return utf8text;
}
