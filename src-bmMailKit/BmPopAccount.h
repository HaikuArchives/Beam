/*
	BmPopAccount.h

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


#ifndef _BmPopAccount_h
#define _BmPopAccount_h

#include <stdexcept>
#include <vector>

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include <socket.h>
#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif

#include <NetAddress.h>

#include "BmDataModel.h"

class BHandler;
class BmPopAccountList;


#define BM_JOBWIN_POP					'bmea'
						// sent to JobMetaController in order to start pop-connection

class BMessageRunner;
/*------------------------------------------------------------------------------*\
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPopAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	friend BmPopAccountList;

	// archivable components:
	static const char* const MSG_NAME = 			"bm:name";
	static const char* const MSG_USERNAME = 		"bm:username";
	static const char* const MSG_PASSWORD = 		"bm:password";
	static const char* const MSG_POP_SERVER = 	"bm:popserver";
	static const char* const MSG_SMTP_ACCOUNT = 	"bm:smtpacc";
	static const char* const MSG_REAL_NAME = 		"bm:realname";
	static const char* const MSG_MAIL_ADDR = 		"bm:mailaddr";
	static const char* const MSG_SIGNATURE_NAME = "bm:signaturename";
	static const char* const MSG_CHECK_MAIL = 	"bm:checkmail";
	static const char* const MSG_DELETE_MAIL = 	"bm:deletemail";
	static const char* const MSG_PORT_NR = 		"bm:portnr";
	static const char* const MSG_UID = 				"bm:uid";
	static const char* const MSG_AUTH_METHOD = 	"bm:authmethod";
	static const char* const MSG_MARK_DEFAULT = 	"bm:markdefault";
	static const char* const MSG_STORE_PWD = 		"bm:storepwd";
	static const char* const MSG_MAIL_ALIASES = 	"bm:mailaliases";
	static const char* const MSG_MARK_BUCKET = 	"bm:markbucket";
	static const char* const MSG_CHECK_INTERVAL = "bm:checkinterval";
	static const int16 nArchiveVersion = 2;

public:
	BmPopAccount( const char* name, BmPopAccountList* model);
	BmPopAccount( BMessage* archive, BmPopAccountList* model);
	virtual ~BmPopAccount();
	
	// native methods:
	bool IsUIDDownloaded( BmString uid);
	void MarkUIDAsDownloaded( BmString uid);
	BmString GetFromAddress() const;
	BmString GetDomainName() const;
	bool HandlesAddress( BmString addr, bool needExactMatch=false) const;
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// debugging overrides:
#ifdef BM_REF_DEBUGGING
	void AddRef()								{ inherited::AddRef(); }
	void RemoveRef()							{ inherited::RemoveRef(); }
#endif // BM_REF_DEBUGGING

	// getters:
	inline const BmString &AuthMethod() const	{ return mAuthMethod; }
	inline bool CheckMail() const 				{ return mCheckMail; }
	inline bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	inline const BmString &MailAddr() const 	{ return mMailAddr; }
	inline const BmString &MailAliases() const { return mMailAliases; }
	inline bool MarkedAsDefault() const			{ return mMarkedAsDefault; }
	inline bool MarkedAsBitBucket() const		{ return mMarkedAsBitBucket; }
	inline const BmString &Name() const 			{ return Key(); }
	inline const BmString &Password() const 	{ return mPassword; }
	inline const BmString &POPServer() const	{ return mPOPServer; }
	inline uint16 PortNr() const					{ return mPortNr; }
	inline const BmString &PortNrString() const{ return mPortNrString; }
	inline bool PwdStoredOnDisk() const			{ return mPwdStoredOnDisk; }
	inline const BmString &RealName() const 	{ return mRealName; }
	inline const BmString &SignatureName() const	 { return mSignatureName; }
	inline const BmString &SMTPAccount() const	{ return mSMTPAccount; }
	inline const BmString &Username() const 	{ return mUsername; }
	inline int16 CheckInterval() const 			{ return mCheckInterval; }
	inline const BmString &CheckIntervalString() const{ return mCheckIntervalString; }

	// setters:
	inline void AuthMethod( const BmString &s) { mAuthMethod = s; TellModelItemUpdated( UPD_ALL); }
	inline void CheckMail( bool b) 				{ mCheckMail = b;  TellModelItemUpdated( UPD_ALL); }
	inline void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b;  TellModelItemUpdated( UPD_ALL); }
	inline void MailAddr( const BmString &s) 	{ mMailAddr = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MailAliases( const BmString &s){ mMailAliases = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsDefault( bool b)		{ mMarkedAsDefault = b;  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsBitBucket( bool b)		{ mMarkedAsBitBucket = b;  TellModelItemUpdated( UPD_ALL); }
	inline void Password( const BmString &s) 	{ mPassword = s;  TellModelItemUpdated( UPD_ALL); }
	inline void POPServer( const BmString &s)	{ mPOPServer = s;  TellModelItemUpdated( UPD_ALL); }
	inline void PortNr( uint16 i)					{ mPortNr = i; mPortNrString = BmString()<<i;  TellModelItemUpdated( UPD_ALL); }
	inline void PwdStoredOnDisk( bool b)		{ mPwdStoredOnDisk = b;  TellModelItemUpdated( UPD_ALL); }
	inline void RealName( const BmString &s) 	{ mRealName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SignatureName( const BmString &s)	 { mSignatureName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SMTPAccount( const BmString &s){ mSMTPAccount = s;  TellModelItemUpdated( UPD_ALL); }
	inline void Username( const BmString &s) 	{ mUsername = s;  TellModelItemUpdated( UPD_ALL); }
	void CheckInterval( int16 i);

	bool GetPOPAddress( BNetAddress* addr) const;

	static const char* const AUTH_POP3 = "POP3";
	static const char* const AUTH_APOP = "APOP";

private:
	BmPopAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmPopAccount( const BmPopAccount&);
	BmPopAccount operator=( const BmPopAccount&);

	void SetupIntervalRunner();

	//BmString mName;					// name is stored in key (base-class)
	BmString mUsername;
	BmString mPassword;
	BmString mPOPServer;
	BmString mSMTPAccount;			// name of BmSmtpAccount to use when sending 
											// mail "from" this POP-account
	BmString mRealName;
	BmString mMailAddr;				// address to use (instead of composed address)
	BmString mMailAliases;			// addresses that belong to this POP-Account, too
	BmString mSignatureName;			// name&path of signature file
	uint16 mPortNr;					// usually 110
	BmString mPortNrString;			// mPortNr as String
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	BmString mAuthMethod;				// authentication method
	bool mMarkedAsDefault;			// is this the default account?
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	bool mMarkedAsBitBucket;		// is this account the fallback-account for failed delivery?
	int16 mCheckInterval;			// check mail every ... minutes
	BmString mCheckIntervalString;	// check-interval as String

	vector<BmString> mUIDs;			// list of UIDs seen in this account
	BMessageRunner* mIntervalRunner;
};


/*------------------------------------------------------------------------------*\
	BmPopAccountList 
		-	holds list of all Pop-Accounts
		-	includes functionality for checking some/all POP-servers for new mail
\*------------------------------------------------------------------------------*/
class BmPopAccountList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 1;

public:
	// creator-func, c'tors and d'tor:
	static BmPopAccountList* CreateInstance( BLooper* jobMetaController);
	BmPopAccountList( BLooper* jobMetaController);
	~BmPopAccountList();
	
	// native methods:
	void CheckMail( bool allAccounts=false);
	void CheckMailFor( BmString accName, bool isAutoCheck=false);
	BmRef<BmPopAccount> DefaultAccount();
	BmRef<BmPopAccount> FindAccountForAddress( const BmString addr);
	void SetDefaultAccount( BmString accName);
	//
	void ResetToSaved();
	
	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmPopAccountList> theInstance;

	static const char* const MSG_AUTOCHECK 	=	"bm:auto";

private:
	// Hide copy-constructor and assignment:
	BmPopAccountList( const BmPopAccountList&);
	BmPopAccountList operator=( const BmPopAccountList&);
	
	BLooper* mJobMetaController;

};

#define ThePopAccountList BmPopAccountList::theInstance

#endif
