/*
	BmMail.h
		$Id$
*/

#ifndef _BmMail_h
#define _BmMail_h

#include <Entry.h>

#include "BmDataModel.h"
#include "BmUtil.h"

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
	
	// overrides of jobmodel base:
	void StartJob();

	// getters:
	const BString& AccountName()			{ return mAccountName; }
	BString& Text()							{ return mText; }
	status_t InitCheck()						{ return mInitCheck; }

protected:
	void Set( BString &msgText, const BString &account);
	BString CreateBasicFilename();

private:
	BmMailHeader* mHeader;					// contains header-information
	int32 mHeaderLength;

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

