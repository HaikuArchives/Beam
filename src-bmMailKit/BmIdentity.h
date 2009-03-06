/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmIdentity_h
#define _BmIdentity_h

#include "BmMailKit.h"

#include <vector>

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include "BmRecvAccount.h"

using std::vector;

class BmIdentityList;

/*------------------------------------------------------------------------------*\
	BmIdentity 
		-	holds information about one specific identity
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmIdentity : public BmListModelItem {
	typedef BmListModelItem inherited;
	friend class BmIdentityList;

public:
	BmIdentity( const char* name, BmIdentityList* model);
	BmIdentity( BMessage* archive, BmIdentityList* model);
	virtual ~BmIdentity();
	
	// native methods:
	BmString GetFromAddress() const;
	BmString GetDomainName() const;
	bool HandlesAddrSpec( BmString addrSpec, bool needExactMatch=false) const;
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;
	//
	BmRef<BmRecvAccount> RecvAcc() const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BmString &MailAddr() const 	
													{ return mMailAddr; }
	inline bool MarkedAsBitBucket() const
													{ return mMarkedAsBitBucket; }
	inline const BmString &Name() const { return Key(); }
	inline const BmString &RecvAccount() const	
													{ return mRecvAccount; }
	inline const BmString &RealName() const 	
													{ return mRealName; }
	inline const BmString &ReplyTo() const 	
													{ return mReplyTo; }
	inline const BmString &SignatureName() const	 
													{ return mSignatureName; }
	inline const BmString &SMTPAccount() const	
													{ return mSMTPAccount; }
	inline const BmString &SpecialHeaders() const 	
													{ return mSpecialHeaders; }
	inline const BmString &MailAliases() const 
													{ return mMailAliases; }

	// setters:
	inline void MailAddr( const BmString &s) 	
													{ mMailAddr = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsBitBucket( bool b)		
													{ mMarkedAsBitBucket = b;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void RecvAccount( const BmString &s)
													{ mRecvAccount = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void RealName( const BmString &s) 	
													{ mRealName = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void ReplyTo( const BmString &s) 	
													{ mReplyTo = s;
													  TellModelItemUpdated( UPD_ALL); }
	inline void SignatureName( const BmString &s)
													{ mSignatureName = s;
													  TellModelItemUpdated( UPD_ALL); }
	inline void SMTPAccount( const BmString &s)
													{ mSMTPAccount = s;
													  TellModelItemUpdated( UPD_ALL); }
	inline void SpecialHeaders( const BmString &s)
													{ mSpecialHeaders = s;
													  TellModelItemUpdated( UPD_ALL); }
	inline void MailAliases( const BmString &s)
													{ mMailAliases = s;
													  _SplitMailAliases();
													  TellModelItemUpdated( UPD_ALL); }

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_RECV_ACCOUNT;
	static const char* const MSG_SMTP_ACCOUNT;
	static const char* const MSG_REAL_NAME;
	static const char* const MSG_MAIL_ADDR;
	static const char* const MSG_SIGNATURE_NAME;
	static const char* const MSG_MARK_BUCKET;
	static const char* const MSG_MAIL_ALIASES;
	static const char* const MSG_REPLY_TO;
	static const char* const MSG_SPECIAL_HEADERS;
	static const int16 nArchiveVersion;

private:
	BmIdentity();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmIdentity( const BmIdentity&);
	BmIdentity operator=( const BmIdentity&);

	void SetupIntervalRunner();
	void _SplitMailAliases();

	BmString mRecvAccount;			// name of BmRecvAccount to use with this Identity
	BmString mSMTPAccount;			// name of BmSmtpAccount to use with this Identity
	BmString mRealName;
	BmString mMailAddr;				// address to use (instead of composed address)
	BmString mSignatureName;		// name of signature file
	bool mMarkedAsBitBucket;		// is this account a catch-all-account for failed delivery?
	BmString mMailAliases;			// addresses that belong to this identity, too
	vector<BmString> mMailAliasesVect;
	BmString mReplyTo;
	BmString mSpecialHeaders;
};


/*------------------------------------------------------------------------------*\
	BmIdentityList 
		-	holds list of all Identities
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmIdentityList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmIdentityList* CreateInstance();
	BmIdentityList();
	~BmIdentityList();
	
	// native methods:
	BmRef<BmIdentity> FindIdentityForRecvAccount( const BmString accName);
	BmString FindFromAddressForRecvAccount( const BmString accName);
	BmRef<BmIdentity> FindIdentityForAddrSpec( const BmString addr);
	BmRef<BmRecvAccount> FindRecvAccountForAddrSpec( const BmString addr);
	//
	void ResetToSaved();
	
	// getters
	inline const BmRef<BmIdentity> CurrIdentity()  { return mCurrIdentity; }

	// setters:
	inline void CurrIdentity( BmIdentity* i) { 
		mCurrIdentity = i; 
		mNeedsStore = true;
	}

	// overrides of listmodel base:
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, const BmString& newVal);
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	void InstantiateItems( BMessage* archive);
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmIdentityList> theInstance;

	static const char* const MSG_CURR_IDENTITY;

private:
	// Hide copy-constructor and assignment:
	BmIdentityList( const BmIdentityList&);
	BmIdentityList operator=( const BmIdentityList&);
	
	BmIdentity* mCurrIdentity;
};

#define TheIdentityList BmIdentityList::theInstance

#endif
