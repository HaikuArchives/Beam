/*
	BmIdentity.h

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#ifndef _BmIdentity_h
#define _BmIdentity_h

#include "BmMailKit.h"

#include <vector>

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include "BmPopAccount.h"

class BmIdentityList;

/*------------------------------------------------------------------------------*\
	BmIdentity 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmIdentity : public BmListModelItem {
	typedef BmListModelItem inherited;
	friend BmIdentityList;

public:
	BmIdentity( const char* name, BmIdentityList* model);
	BmIdentity( BMessage* archive, BmIdentityList* model);
	virtual ~BmIdentity();
	
	// native methods:
	BmString GetFromAddress() const;
	bool HandlesAddrSpec( BmString addrSpec, bool needExactMatch=false) const;
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;
	//
	BmRef<BmPopAccount> PopAcc() const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BmString &MailAddr() const 	{ return mMailAddr; }
	inline bool MarkedAsBitBucket() const		{ return mMarkedAsBitBucket; }
	inline const BmString &Name() const 		{ return Key(); }
	inline const BmString &POPAccount() const	{ return mPOPAccount; }
	inline const BmString &RealName() const 	{ return mRealName; }
	inline const BmString &ReplyTo() const 	{ return mReplyTo; }
	inline const BmString &SignatureName() const	 { return mSignatureName; }
	inline const BmString &SMTPAccount() const	{ return mSMTPAccount; }
	inline const BmString &MailAliases() const { return mMailAliases; }

	// setters:
	inline void MailAddr( const BmString &s) 	{ mMailAddr = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsBitBucket( bool b)		{ mMarkedAsBitBucket = b;  TellModelItemUpdated( UPD_ALL); }
	inline void POPAccount( const BmString &s){ mPOPAccount = s; TellModelItemUpdated( UPD_ALL); }
	inline void RealName( const BmString &s) 	{ mRealName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void ReplyTo( const BmString &s) 	{ mReplyTo = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SignatureName( const BmString &s)	 { mSignatureName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SMTPAccount( const BmString &s){ mSMTPAccount = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MailAliases( const BmString &s){ mMailAliases = s;  TellModelItemUpdated( UPD_ALL); }

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_POP_ACCOUNT;
	static const char* const MSG_SMTP_ACCOUNT;
	static const char* const MSG_REAL_NAME;
	static const char* const MSG_MAIL_ADDR;
	static const char* const MSG_SIGNATURE_NAME;
	static const char* const MSG_MARK_BUCKET;
	static const char* const MSG_MAIL_ALIASES;
	static const char* const MSG_REPLY_TO;
	static const int16 nArchiveVersion;

private:
	BmIdentity();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmIdentity( const BmIdentity&);
	BmIdentity operator=( const BmIdentity&);

	void SetupIntervalRunner();

	//BmString mName;					// name is stored in key (base-class)
	BmString mPOPAccount;			// name of BmPopAccount to use with this Identity
	BmString mSMTPAccount;			// name of BmSmtpAccount to use with this Identity
	BmString mRealName;
	BmString mMailAddr;				// address to use (instead of composed address)
	BmString mSignatureName;		// name of signature file
	bool mMarkedAsBitBucket;		// is this account a catch-all-account for failed delivery?
	BmString mMailAliases;			// addresses that belong to this identity, too
	BmString mReplyTo;
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
	BmRef<BmIdentity> FindIdentityForPopAccount( const BmString accName);
	BmString FindFromAddressForPopAccount( const BmString accName);
	BmRef<BmIdentity> FindIdentityForAddrSpec( const BmString addr);
	BmRef<BmPopAccount> FindPopAccountForAddrSpec( const BmString addr);
	//
	void ResetToSaved();
	
	// getters
	inline const BmRef<BmIdentity> CurrIdentity()  { return mCurrIdentity; }

	// setters:
	inline void CurrIdentity( BmIdentity* i) { mCurrIdentity = i; }

	// overrides of listmodel base:
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, const BmString& newVal);
	const BmString SettingsFileName();
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
