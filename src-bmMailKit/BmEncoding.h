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
	
	const uint32 BM_UTF8_CONVERSION = 0xFFFF;

	uint32 CharsetToEncoding( const BString& charset);
	BString EncodingToCharset( const uint32 encoding);

	BString ConvertToUTF8( uint32 srcEncoding, const char* srcBuf);

	char* Encode( const BString& enodingStyle, char* text);
	void Decode( const BString& encodingStyle, BString& text, bool isEncodedWord,
					 bool isText);
	int32 DecodedLength( const BString& encodingStyle, const char* text, int32 length);

	BString ConvertHeaderPartToUTF8( const BString& headerPart, int32 defaultEncoding);
	BString ConvertUTF8ToHeaderPart( const BString& utf8text);
	
};

#endif

