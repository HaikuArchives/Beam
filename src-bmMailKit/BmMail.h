/*
	BmMail.h
		$Id$
*/

#ifndef _BmMail_h
#define _BmMail_h

#include <vector>

#include <Entry.h>
#include <Mime.h>

#include "BmDataModel.h"
#include "BmUtil.h"

class BmMailRef;
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
class BmBodyPart {
	typedef vector<BmBodyPart> BmBodyPartVect;

public:
	// c'tors and d'tor:
	BmBodyPart();
	BmBodyPart( const BString& msgtext, int32 s, int32 l, BmMailHeader* mHeader=NULL);

	// native methods:
	void SetTo( const BString& msgtext, int32 s, int32 l, BmMailHeader* mHeader=NULL);
	BString DecodedData() const;
	bool IsText() const;

	bool mIsMultiPart;
	const char* mPosInRawText;
	int32 mLength;
	BmContentField mContentType;
	BString mContentTransferEncoding;
	BString mContentId;
	BmContentField mContentDisposition;
	BString mContentDescription;
	BString mContentLanguage;
	BmBodyPartVect mBodyPartVect;
};

/*------------------------------------------------------------------------------*\
	BmMail 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
\*------------------------------------------------------------------------------*/
class BmMail : public BmJobModel {
	typedef BmJobModel inherited;

public:
	BmMail( );
	BmMail( BmMailRef* ref);
	BmMail( BString &msgText, const BString &account);
	virtual ~BmMail();

	// native methods:
	bool Store();
	void StoreAttributes( BFile& mailFile);
	
	// overrides of jobmodel base:
	void StartJob();

	// getters:
	const BString& AccountName()			{ return mAccountName; }
	const BString& RawText() const		{ return mText; }
	const status_t InitCheck()	const		{ return mInitCheck; }
	const BmBodyPart& Body() const		{ return mBody; }
	BmMailHeader* Header() const			{ return mHeader; }

protected:
	void SetTo( BString &msgText, const BString &account);
	BString CreateBasicFilename();

private:
	BmMailHeader* mHeader;					// contains header-information
	int32 mHeaderLength;
	BmBodyPart mBody;							// contains body-information (split into subparts)

	BString mText;								// text of complete message
	BString mAccountName;					// name of POP-account this message came from
	BString mStatus;							// status of this mail (client-status that is, e.g. "Read" or "New")
	bool mHasAttachments;					// flag indicating the presence of attachments

	BEntry mParentEntry;						// filesystem-entry for mailfolder this mail currently lives in
	BEntry mMailEntry;						// filesystem-entry for this mail (N.B. the entry may
													// be set although the mail does not yet exist on disk)

	BmMailRef* mMailRef;
	status_t mInitCheck;
};

#endif

