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
#include <String.h>

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
	bool IsUIDDownloaded( BString uid);
	void MarkUIDAsDownloaded( BString uid);
	BString GetFromAddress() const;
	BString GetDomainName() const;
	bool HandlesAddress( BString addr, bool needExactMatch=false) const;
	bool SanityCheck( BString& complaint, BString& fieldName) const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// debugging overrides:
#ifdef BM_REF_DEBUGGING
	void AddRef()								{ inherited::AddRef(); }
	void RemoveRef()							{ inherited::RemoveRef(); }
#endif // BM_REF_DEBUGGING

	// getters:
	inline const BString &AuthMethod() const	{ return mAuthMethod; }
	inline bool CheckMail() const 				{ return mCheckMail; }
	inline bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	inline const BString &MailAddr() const 	{ return mMailAddr; }
	inline const BString &MailAliases() const { return mMailAliases; }
	inline bool MarkedAsDefault() const			{ return mMarkedAsDefault; }
	inline bool MarkedAsBitBucket() const		{ return mMarkedAsBitBucket; }
	inline const BString &Name() const 			{ return Key(); }
	inline const BString &Password() const 	{ return mPassword; }
	inline const BString &POPServer() const	{ return mPOPServer; }
	inline uint16 PortNr() const					{ return mPortNr; }
	inline const BString &PortNrString() const{ return mPortNrString; }
	inline bool PwdStoredOnDisk() const			{ return mPwdStoredOnDisk; }
	inline const BString &RealName() const 	{ return mRealName; }
	inline const BString &SignatureName() const	 { return mSignatureName; }
	inline const BString &SMTPAccount() const	{ return mSMTPAccount; }
	inline const BString &Username() const 	{ return mUsername; }
	inline int16 CheckInterval() const 			{ return mCheckInterval; }
	inline const BString &CheckIntervalString() const{ return mCheckIntervalString; }

	// setters:
	inline void AuthMethod( const BString &s) { mAuthMethod = s; TellModelItemUpdated( UPD_ALL); }
	inline void CheckMail( bool b) 				{ mCheckMail = b;  TellModelItemUpdated( UPD_ALL); }
	inline void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b;  TellModelItemUpdated( UPD_ALL); }
	inline void MailAddr( const BString &s) 	{ mMailAddr = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MailAliases( const BString &s){ mMailAliases = s;  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsDefault( bool b)		{ mMarkedAsDefault = b;  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsBitBucket( bool b)		{ mMarkedAsBitBucket = b;  TellModelItemUpdated( UPD_ALL); }
	inline void Password( const BString &s) 	{ mPassword = s;  TellModelItemUpdated( UPD_ALL); }
	inline void POPServer( const BString &s)	{ mPOPServer = s;  TellModelItemUpdated( UPD_ALL); }
	inline void PortNr( uint16 i)					{ mPortNr = i; mPortNrString = BString()<<i;  TellModelItemUpdated( UPD_ALL); }
	inline void PwdStoredOnDisk( bool b)		{ mPwdStoredOnDisk = b;  TellModelItemUpdated( UPD_ALL); }
	inline void RealName( const BString &s) 	{ mRealName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SignatureName( const BString &s)	 { mSignatureName = s;  TellModelItemUpdated( UPD_ALL); }
	inline void SMTPAccount( const BString &s){ mSMTPAccount = s;  TellModelItemUpdated( UPD_ALL); }
	inline void Username( const BString &s) 	{ mUsername = s;  TellModelItemUpdated( UPD_ALL); }
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

	//BString mName;					// name is stored in key (base-class)
	BString mUsername;
	BString mPassword;
	BString mPOPServer;
	BString mSMTPAccount;			// name of BmSmtpAccount to use when sending 
											// mail "from" this POP-account
	BString mRealName;
	BString mMailAddr;				// address to use (instead of composed address)
	BString mMailAliases;			// addresses that belong to this POP-Account, too
	BString mSignatureName;			// name&path of signature file
	uint16 mPortNr;					// usually 110
	BString mPortNrString;			// mPortNr as String
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	BString mAuthMethod;				// authentication method
	bool mMarkedAsDefault;			// is this the default account?
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	bool mMarkedAsBitBucket;		// is this account the fallback-account for failed delivery?
	int16 mCheckInterval;			// check mail every ... minutes
	BString mCheckIntervalString;	// check-interval as String

	vector<BString> mUIDs;			// list of UIDs seen in this account
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
	void CheckMailFor( BString accName, bool isAutoCheck=false);
	BmRef<BmPopAccount> DefaultAccount();
	BmRef<BmPopAccount> FindAccountForAddress( const BString addr);
	void SetDefaultAccount( BString accName);
	//
	void ResetToSaved();
	
	// overrides of listmodel base:
	const BString SettingsFileName();
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
