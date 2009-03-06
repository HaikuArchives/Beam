/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmEncoding_h
#define _BmEncoding_h

#include "BmMailKit.h"

#include <memory>
#include <map>
#include <vector>

#include <iconv.h>

#include "BmString.h"
#include "BmMemIO.h"
#include "BmUtil.h"

using std::auto_ptr;
using std::map;
using std::vector;

/*------------------------------------------------------------------------------*\
	BmEncoding 
\*------------------------------------------------------------------------------*/
namespace BmEncoding {
	
	extern IMPEXPBMMAILKIT const int16 BM_MAX_HEADER_LINE_LEN;
	extern IMPEXPBMMAILKIT const int16 BM_MAX_BODY_LINE_LEN;

	typedef map< BmString, bool> BmCharsetMap;
	extern IMPEXPBMMAILKIT BmCharsetMap TheCharsetMap;

	extern IMPEXPBMMAILKIT BmString DefaultCharset;
	
	typedef vector< BmString> BmCharsetVect;
	IMPEXPBMMAILKIT 
	void GetPreferredCharsets( BmCharsetVect& charsetVect, 
										const BmString& nativeCharset,
										bool outbound = false);

	IMPEXPBMMAILKIT 
	void InitCharsetMap();

	IMPEXPBMMAILKIT 
	const char* ConvertFromBeosToLibiconv( uint32 encoding);

	IMPEXPBMMAILKIT 
	void ConvertToUTF8( const BmString& srcCharset, const BmString& src, 
							  BmString& dest);
	
	IMPEXPBMMAILKIT 
	void ConvertFromUTF8( const BmString& destCharset, const BmString& src, 
								 BmString& dest);

	IMPEXPBMMAILKIT 
	void Encode( BmString encodingStyle, const BmString& src, BmString& dest,
					 const BmString& tags=BM_DEFAULT_STRING);
	IMPEXPBMMAILKIT 
	void Decode( BmString encodingStyle, const BmString& src, BmString& dest,
					 const BmString& tags=BM_DEFAULT_STRING);

	IMPEXPBMMAILKIT 
	BmString ConvertHeaderPartToUTF8( const BmString& headerPart, 
												 const BmString& defaultCharset,
												 bool& hadConversionError);
	IMPEXPBMMAILKIT 
	BmString ConvertUTF8ToHeaderPart( const BmString& utf8text, 
												 const BmString& charset,
												 bool useQuotedPrintableIfNeeded, 
												 int32 fieldLen);
	
	IMPEXPBMMAILKIT 
	bool NeedsQuotedPrintableEncoding( const BmString& utf8String, 
												  uint16 maxLineLen=0);
	IMPEXPBMMAILKIT 
	bool IsCompatibleWithText( const BmString& s);

	typedef auto_ptr<BmMemFilter> BmMemFilterRef;
	IMPEXPBMMAILKIT 
	BmMemFilterRef FindDecoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 uint32 blockSize=BmMemFilter::nBlockSize,
											 const BmString& tags=BM_DEFAULT_STRING);
	IMPEXPBMMAILKIT 
	BmMemFilterRef FindEncoderFor( BmMemIBuf* input, 
											 const BmString& encodingStyle,
											 uint32 blockSize=BmMemFilter::nBlockSize,
											 const BmString& tags=BM_DEFAULT_STRING);

}


/*------------------------------------------------------------------------------*\
	class BmUtf8Decoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmUtf8Decoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmUtf8Decoder( BmMemIBuf* input, const BmString& destCharset, 
						uint32 blockSize=nBlockSize, 
						const BmString& tags=BM_DEFAULT_STRING);
	~BmUtf8Decoder();
	
	// native methods:
	void InitConverter();
	void SetTransliterate( bool transliterate);
	void SetDiscard( bool discard);

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);

	static IMPEXPBMMAILKIT const char* nTagTransliterate;
	static IMPEXPBMMAILKIT const char* nTagDiscard;

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	BmString mDestCharset;
	iconv_t mIconvDescr;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;
};

/*------------------------------------------------------------------------------*\
	class BmUtf8Encoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmUtf8Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmUtf8Encoder( BmMemIBuf* input, const BmString& srcCharset, 
						uint32 blockSize=nBlockSize, 
						const BmString& tags=BM_DEFAULT_STRING);
	~BmUtf8Encoder();

	// native methods:
	void InitConverter();
	void SetTransliterate( bool transliterate);
	void SetDiscard( bool discard);

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

	static IMPEXPBMMAILKIT const char* nTagTransliterate;
	static IMPEXPBMMAILKIT const char* nTagDiscard;

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	BmString mSrcCharset;
	iconv_t mIconvDescr;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;
	bool mHaveResetToInitialState;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableDecoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmQuotedPrintableDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQuotedPrintableDecoder( BmMemIBuf* input, uint32 blockSize=nBlockSize,
									  const BmString& tags=BM_DEFAULT_STRING);

	static IMPEXPBMMAILKIT const char* nTagIsEncodedWord;
protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	int mSpacesThatMayNeedRemoval;
	bool mSoftbreakPending;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmQuotedPrintableEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQuotedPrintableEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	void Queue( const char* chars, uint32 len);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	BmRingBuf mQueuedChars;
	int mSpacesThatMayNeedEncoding;
	int mLastAddedLen;
	int mCurrAddedLen;
	bool mNeedFlush;
	int mKeepLen;
};

/*------------------------------------------------------------------------------*\
	class BmQuotedPrintableEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmQpEncodedWordEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmQpEncodedWordEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize, 
									int startAtOffset=0,
									const BmString& charset=BM_DEFAULT_STRING);
	virtual ~BmQpEncodedWordEncoder();

	// getters:
	bool HadToDiscardChars()				{ return mHadToDiscardChars; }
	int32 FirstDiscardedPos()				{ return mFirstDiscardedPos; }

protected:
	// native methods:
	void InitConverter();
	void EncodeConversionBuf();
	void Queue( const char* chars, uint32 len);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	// overrides of BmMemFilter base:
	void Reset( BmMemIBuf* input);
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	BmString mEncodedWord;
	BmRingBuf mQueuedChars;
	int mStartAtOffset;
	int mLastAddedLen;
	int mCurrAddedLen;
	bool mNeedFlush;
	int mKeepLen;
	int mCharacterLen;
	//
	BmString mDestCharset;
	iconv_t mIconvDescr;
	bool mHadToDiscardChars;
	int32 mFirstDiscardedPos;
	bool mStoppedOnMultibyte;

	BmString mConversionBuf;
};

/*------------------------------------------------------------------------------*\
	class BmFoldedLineEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmFoldedLineEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmFoldedLineEncoder( BmMemIBuf* input, int lineLen,
							   uint32 blockSize=nBlockSize, int startAtOffset=0);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	bool OutputLineIfNeeded( char* &dest, const char* destEnd);

	int mStartAtOffset;
	int mMaxLineLen;
	int mLastSpacePos;
	BmRingBuf mQueuedChars;
	bool mNeedFlush;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Decoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmBase64Decoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBase64Decoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

	static const int32 nBase64Alphabet[256];

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);

	uint32 mConcat;
	uint32 mIndex;
};

/*------------------------------------------------------------------------------*\
	class BmBase64Encoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmBase64Encoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBase64Encoder( BmMemIBuf* input, uint32 blockSize=nBlockSize, 
						  const BmString& tags=BM_DEFAULT_STRING);

	static const char nBase64Alphabet[64];

	static IMPEXPBMMAILKIT const char* nTagOnSingleLine;

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
	void Finalize( char* destBuf, uint32& destLen);
	
	uint32 mConcat;
	uint32 mIndex;
	int mCurrLineLen;
};

/*------------------------------------------------------------------------------*\
	class BmLinebreakDecoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmLinebreakDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmLinebreakDecoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

};

/*------------------------------------------------------------------------------*\
	class BmLinebreakEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmLinebreakEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmLinebreakEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
};

/*------------------------------------------------------------------------------*\
	class BmMailtextCleaner
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailtextCleaner : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmMailtextCleaner( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

private:
	bool mLastWasStartOfShiftSpace;
};

/*------------------------------------------------------------------------------*\
	class BmBinaryDecoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmBinaryDecoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBinaryDecoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

};

/*------------------------------------------------------------------------------*\
	class BmBinaryEncoder
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmBinaryEncoder : public BmMemFilter {
	typedef BmMemFilter inherited;

public:
	BmBinaryEncoder( BmMemIBuf* input, uint32 blockSize=nBlockSize);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);
};

#endif

