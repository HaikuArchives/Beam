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

class BmBodyPartList;
class BmMailRef;
class BmMailHeader;

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
	void MarkAs( const char* status);
	
	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	const BString& AccountName()			{ return mAccountName; }
	const BString& RawText() const		{ return mText; }
	const status_t InitCheck()	const		{ return mInitCheck; }
	BmBodyPartList* Body() const			{ return mBody.Get(); }
	BmMailHeader* Header() const			{ return mHeader; }
	int32 HeaderLength() const				{ return mHeaderLength; }
	const BString Status() const;

protected:
	void SetTo( BString &msgText, const BString &account);
	BString CreateBasicFilename();

private:
	BmMailHeader* mHeader;					// contains header-information
	int32 mHeaderLength;
	BmRef<BmBodyPartList> mBody;			// contains body-information (split into subparts)

	BString mText;								// text of complete message
	BString mAccountName;					// name of POP-account this message came from
	bool mHasAttachments;					// flag indicating the presence of attachments

	BEntry mParentEntry;						// filesystem-entry for mailfolder this mail currently lives in
	BEntry mMailEntry;						// filesystem-entry for this mail (N.B. the entry may
													// be set although the mail does not yet exist on disk)

	BmRef<BmMailRef> mMailRef;
	status_t mInitCheck;
};

#endif

