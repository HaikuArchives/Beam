/*
	BmEncoding.cpp
		$Id$
*/

#include <string.h>

#include <UTF8.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmEncoding.h"
#include "BmUtil.h"

#define HEXDIGIT2CHAR(d) (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0)


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 BmEncoding::CharsetToEncoding( const BString& charset) {
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
	[zooey]:	does not seem to be registered with MIME ?!?
	if (charset == "macintosh") 		return( B_MAC_ROMAN_CONVERSION);
*/
	if (set == "SHIFT_JIS") 		return( B_SJIS_CONVERSION);
	if (set == "EUC-JP") 			return( B_EUC_CONVERSION);
	if (set == "ISO-2022-JP")	 	return( B_JIS_CONVERSION);
	if (set == "WINDOWS-1252") 	return( B_MS_WINDOWS_CONVERSION);
/*	
	[zooey]:	makes no sense within e-mail ?!?:
	if (charset == "") 		return( B_UNICODE_CONVERSION);
*/
	if (set == "KOI8-R") 			return( B_KOI8R_CONVERSION);
	if (set == "WINDOWS-1251") 	return( B_MS_WINDOWS_1251_CONVERSION);
	if (set == "IBM866") 			return( B_MS_DOS_866_CONVERSION);
	if (set == "IBM850") 			return( B_MS_DOS_CONVERSION);
	if (set == "EUC-KR") 			return( B_EUC_KR_CONVERSION);
	if (set == "ISO-8859-13") 		return( B_ISO13_CONVERSION);
	if (set == "ISO-8859-14") 		return( B_ISO14_CONVERSION);
	if (set == "ISO-8859-15") 		return( B_ISO15_CONVERSION);

	return -1;		// to indicate us-ascii (which is the fallback)
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
char* BmEncoding::ConvertToUTF8( uint32 srcEncoding, const char* srcBuf) {
	char* destBuf = NULL;
	int32 srcbuflen = strlen(srcBuf);
	int32 state=0;
	int32 buflen = (int32)(srcbuflen*1.5);
	status_t st;

	try {
		for( bool finished=false; !finished; ) {
			if (!destBuf) {
				(destBuf = (char*)malloc( buflen))
														|| BM_THROW_RUNTIME( "Unable to allocate memory in ConvertToUTF8()");
			} else {
				buflen *= 2;
				(destBuf = (char*)realloc( destBuf, buflen))
														|| BM_THROW_RUNTIME( "Unable to re-allocate memory in ConvertToUTF8()");
			}
			int32 srcLen = srcbuflen;
			int32 destLen = buflen-1;
			(st=convert_to_utf8( srcEncoding, srcBuf, &srcLen, destBuf, &destLen, &state)) == B_OK
														|| BM_THROW_RUNTIME( BString("error in convert_to_utf8(): ") << strerror( st));
			if (srcLen == srcbuflen) {
				finished = true;
				destBuf[destLen] = '\0';
			}
		}
	} catch (...) {
		if (destBuf)
			free( destBuf);
		throw;
	}
	return destBuf;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
char* BmEncoding::Encode( const BString& encodingStyle, char* text) {
	if (encodingStyle.ICompare( "Q") == 0) {
		// quoted printable:
		return text;
	} else if (encodingStyle.ICompare( "B") == 0) {
		// base64:
		return text;
	} else {
		// oops, we don't know this one:
		ShowAlert( BString("Encode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nEncoded text will be passed through.");
		return text;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmEncoding::Decode( const BString& encodingStyle, BString& text) {
	if (encodingStyle.ICompare( "Q") == 0) {
		// quoted printable:
		text.ReplaceAll( "_", " ");
		int32 len=text.Length();
		int32 nm;
		Regexx rx;
		rx.expr( "=([0-9A-F][0-9A-F])");
		rx.str( text);
		if ((nm = rx.exec( Regexx::global | Regexx::nocase))) {
			char* buf = text.LockBuffer(len+1);
			unsigned char ch;
			int32 neg_offs=0;
			int32 curr=0;
			vector<RegexxMatch>::const_iterator i;
			for( i = rx.match.begin(); i != rx.match.end(); ++i) {
				int32 pos = i->start();
				if (curr<pos && neg_offs) {
					memmove( buf+curr-neg_offs, buf+curr, pos-curr);
				}
				ch = HEXDIGIT2CHAR(buf[pos+1])*16 + HEXDIGIT2CHAR(buf[pos+2]);
				buf[pos-neg_offs] = ch;
				curr = pos-neg_offs+1;
				neg_offs += 2;
			}
			if (curr<len-neg_offs) {
				memmove( buf+curr-neg_offs, buf+curr, len-neg_offs-curr);
			}
			buf[len-neg_offs] = '\0';
			text.UnlockBuffer( -1);
		}
	} else if (encodingStyle.ICompare( "B") == 0) {
		// base64:
	} else {
		// oops, we don't know this one:
		ShowAlert( BString("Decode(): Unrecognized encoding-style <")<<encodingStyle<<"> found.\nNo encoding will take place.");
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::ConvertHeaderPartToUTF8( const BString& headerPart) {
	int32 nm;
	Regexx rx;
	rx.expr( "=\\?(.+?)\\?(.)\\?(.+?)\\?=\\s*");
	rx.str( headerPart);

	if ((nm = rx.exec( Regexx::global))) {
		BString utf8;
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
			Decode( srcQuotingStyle, text);
			
			utf8 += ConvertToUTF8( CharsetToEncoding(srcCharset), text.String());
			curr = i->start()+i->Length();
		}
		if (curr<len) {
			utf8.Append( headerPart.String()+curr, len-curr);
		}
		return utf8;
	} else {
		return headerPart;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmEncoding::ConvertUTF8ToHeaderPart( const BString& utf8text) {
	BString headerPart;
	return headerPart;
}
