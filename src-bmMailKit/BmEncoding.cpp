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
BString BmEncoding::ConvertToUTF8( uint32 srcEncoding, const char* srcBuf) {
	BString utf8String;
	char* destBuf = NULL;
	int32 srcbuflen = strlen(srcBuf);
	int32 state=0;
	int32 buflen = srcbuflen>92 ? (int32)(srcbuflen*1.5) : 128;
	status_t st;
	if (!srcbuflen)
		return "";
	if (srcEncoding == BM_UTF8_CONVERSION)
		return srcBuf;							// source already is UTF8, we do nothing

	try {
		for( bool finished=false; !finished; ) {
			if (destBuf)
				buflen *= 2;
			destBuf = utf8String.LockBuffer( buflen);
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;
			(st=convert_to_utf8( srcEncoding, srcBuf, &srcLen, destBuf, &destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BString("error in convert_to_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
			utf8String.UnlockBuffer( destLen);
		}
	} catch (...) {
		utf8String.UnlockBuffer( 0);
		throw;
	}
	return utf8String;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const char* BmEncoding::Encode( const BString& encodingStyle, void* data, int32 dataLen) {
	if (encodingStyle.ICompare( "Q") == 0) {
		// quoted printable:
		return "";
	} else if (encodingStyle.ICompare( "B") == 0) {
		// base64:
		return "";
	} else {
		// oops, we don't know this one:
		ShowAlert( BString("Encode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nEncoded text will be passed through.");
		return "";
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void* BmEncoding::Decode( const BString& encodingStyle, BString& text, 
								  bool isEncodedWord, bool isText, ssize_t& outSize) {
	char* dest = (char*)malloc( text.Length()+1);
	if (encodingStyle.ICompare( "Q")==0 || encodingStyle.ICompare("Quoted-Printable")==0) {
		// quoted printable:
		Regexx rx;
		// remove trailing whitespace (was added during mail-transport):
		text = rx.replace( text, "[\\t ]+(?=\\r\\n)", "", Regexx::newline | Regexx::global);
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
			int32 neg_offs=0;
			int32 curr=0;
			vector<RegexxMatch>::const_iterator i;
			for( i = rx.match.begin(); i != rx.match.end(); ++i) {
				int32 pos = i->start();
				if (curr<pos) {
					memcpy( dest+curr-neg_offs, buf+curr, pos-curr);
				}
				ch = HEXDIGIT2CHAR(buf[pos+1])*16 + HEXDIGIT2CHAR(buf[pos+2]);
				dest[pos-neg_offs] = ch;
				curr = pos+3;
				neg_offs += 2;
			}
			if (curr<len) {
				memcpy( dest+curr-neg_offs, buf+curr, len-curr);
			}
			dest[len-neg_offs] = '\0';
			outSize = len-neg_offs;
		} else {
			memcpy( dest, buf, len+1);
			outSize = len;
		}
	} else if (encodingStyle.ICompare( "B") == 0 || encodingStyle.ICompare("Base64")==0) {
		// base64:
		bool convertCRs = isText;
		off_t inSize = text.Length();
		if (!inSize)
			return dest;
		char* in = text.LockBuffer(0);
		outSize = decode_base64( dest, in, inSize, convertCRs);
		dest[outSize] = '\0';
		text.UnlockBuffer( inSize);
	} else {
		if (encodingStyle.ICompare( "7bit") && encodingStyle.ICompare( "8bit")
		&& encodingStyle.ICompare( "binary")) {
			// oops, we don't know this one:
			BM_SHOWERR( BString("Decode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo decoding will take place.");
			outSize = 0;
		} else {
			// we simply copy the buffer, since the encoding type needs no conversion at all:
			memcpy( dest, text.String(), text.Length()+1);
			outSize = text.Length();
		}
	}
	return dest;
}

/*------------------------------------------------------------------------------*\
	DecodedLength()
		-	returns length of given encoded text after it has been decoded
		-	value will be computed, NO actual decoding happening here!
\*------------------------------------------------------------------------------*/
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
			BString srcCharset( i->atom[0]);
			BString srcQuotingStyle( i->atom[1]);
			BString text( i->atom[2]);
			int32 decodedSize;
			char* decodedData = (char*)Decode( srcQuotingStyle, text, true, true, decodedSize);
			if (decodedData) {
				utf8 += ConvertToUTF8( CharsetToEncoding(srcCharset), decodedData);
				free( decodedData);
			}
			curr = i->start()+i->Length();
		}
		if (curr<len) {
			utf8.Append( headerPart.String()+curr, len-curr);
		}
	} else {
		utf8 = ConvertToUTF8( defaultEncoding, headerPart.String());
	}
	return utf8;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::ConvertUTF8ToHeaderPart( const BString& utf8text) {
	BString headerPart;
	return headerPart;
}
