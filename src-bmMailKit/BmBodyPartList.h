/*
	BmBodyPartList.h
		$Id$
*/

#ifndef _BmBodyPartList_h
#define _BmBodyPartList_h

#include "BmDataModel.h"

class BmMail;
class BmMailHeader;

/*------------------------------------------------------------------------------*\
	BmContentField
		-	
\*------------------------------------------------------------------------------*/
class BmContentField {
	typedef map< BString, BString> BmParamMap;

public:
	// c'tors and d'tor:
	BmContentField()						{ mInitCheck = B_NO_INIT; }
	BmContentField( const BString cfString);
	
	// native methods:
	void SetTo( const BString cfString);

	// getters:
	status_t InitCheck() const				{ return mInitCheck; }
	const BString& Value() const			{ return mValue; }
	const BString& Param( BString key) const;

private:
	BString mValue;
	BmParamMap mParams;

	status_t mInitCheck;
};



class BmBodyPartList;
/*------------------------------------------------------------------------------*\
	BmBodyPart
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPart : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	// c'tors and d'tor:
	BmBodyPart( BmBodyPartList* model, BmListModelItem* parent=NULL);
	BmBodyPart( BmBodyPartList* model, const BString& msgtext, int32 s, int32 l, 
					BmMailHeader* mHeader=NULL, BmListModelItem* parent=NULL);

	// native methods:
	void SetTo( const BString& msgtext, int32 s, int32 l, BmMailHeader* mHeader=NULL);
	void* DecodedData( int32* dataLen=NULL) const;
	bool IsText() const;
	bool IsPlainText() const;
	bool ShouldBeShownInline()	const;
	entry_ref WriteToTempFile( BString filename="");

	// getters:
	bool IsMultiPart() const				{ return mIsMultiPart; }
	const char* PosInRawText() const		{ return mPosInRawText; }
	int32 Length() const						{ return mLength; }
	int32 DecodedLength() const			{ return mDecodedLength; }

	const BString& MimeType() const		{ return mContentType.Value(); }
	const BString& Disposition() const	{ return mContentDisposition.Value(); }
	const BString& FileName() const		{ return mFileName; }
	const BString& Encoding() const		{ return mContentTransferEncoding; }
	const BString& ID() const				{ return mContentId; }
	const BString& Description() const	{ return mContentDescription; }
	const BString& Language() const		{ return mContentLanguage; }
	const BString& TypeParam( BString key) const		{ return mContentType.Param( key); }
	const BString& DispositionParam( BString key) const	{ return mContentDisposition.Param( key); }

private:
	bool mIsMultiPart;
	const char* mPosInRawText;
	int32 mLength;
	int32 mDecodedLength;
	BmContentField mContentType;
	BString mContentTransferEncoding;
	BString mContentId;
	BmContentField mContentDisposition;
	BString mContentDescription;
	BString mContentLanguage;
	BString mFileName;

	static int32 nCounter;

};



/*------------------------------------------------------------------------------*\
	BmBodyPartList
		-	class 
\*------------------------------------------------------------------------------*/
class BmBodyPartList : public BmListModel {
	typedef BmListModel inherited;

public:
	// c'tors and d'tor
	BmBodyPartList( BmMail* mail);
	virtual ~BmBodyPartList();

	// native methods:

	//	overrides of listmodel base:
	bool StartJob();
	const BString SettingsFileName()		{ return NULL; }

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

private:
	BmMail* mMail;
	status_t mInitCheck;

};


#endif
