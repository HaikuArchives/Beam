/*
	BmEncoding.h
		$Id$
*/

#ifndef _BmEncoding_h
#define _BmEncoding_h

/*------------------------------------------------------------------------------*\
	BmEncoding 
\*------------------------------------------------------------------------------*/
namespace BmEncoding {
	
	int32 CharsetToEncoding( const BString& charset);
	BString EncodingToCharset( const int32 encoding);

	BString ConvertToUTF8( uint32 srcEncoding, const char* srcBuf);

	char* Encode( const BString& enodingStyle, char* text);
	void Decode( const BString& encodingStyle, BString& text, bool isEncodedWord,
					 bool isText);

	BString ConvertHeaderPartToUTF8( const BString& headerPart, int32 defaultEncoding);
	BString ConvertUTF8ToHeaderPart( const BString& utf8text);
	
};

#endif

