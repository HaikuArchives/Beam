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
	status_t InitCheck()					{ return mInitCheck; }

	BString mValue;
	BmParamMap mParams;

private:
	status_t mInitCheck;
};



/*------------------------------------------------------------------------------*\
	BmBodyPart
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPart : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	// c'tors and d'tor:
	BmBodyPart( BmListModelItem* parent=NULL);
	BmBodyPart( const BString& msgtext, int32 s, int32 l, 
					BmMailHeader* mHeader=NULL, BmListModelItem* parent=NULL);

	// native methods:
	void SetTo( const BString& msgtext, int32 s, int32 l, BmMailHeader* mHeader=NULL);
	BString DecodedData() const;
	bool IsText() const;
	bool IsPlainText() const;
	bool ShouldBeShownInline()	const;

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

private:
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

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

private:
	BmMail* mMail;
	status_t mInitCheck;

};


#endif
