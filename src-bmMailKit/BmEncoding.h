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
	
	extern const char* BM_Encodings[];

	const uint32 BM_UTF8_CONVERSION = 0xFFFF;

	uint32 CharsetToEncoding( const BString& charset);
	BString EncodingToCharset( const uint32 encoding);

	void ConvertToUTF8( uint32 srcEncoding, const BString& src, BString& dest);
	void ConvertFromUTF8( uint32 destEncoding, const BString& src, BString& dest);

	void Encode( BString encodingStyle, const BString& src, BString& dest,
					 bool isEncodedWord=false);
	void Decode( BString encodingStyle, const BString& src, BString& dest,
					 bool isEncodedWord, bool isText);
//	int32 DecodedLength( const BString& encodingStyle, const char* text, int32 length);

	BString ConvertHeaderPartToUTF8( const BString& headerPart, int32 defaultEncoding);
	BString ConvertUTF8ToHeaderPart( const BString& utf8text, int32 encoding);
	
};

#endif

