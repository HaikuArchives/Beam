/*
	BmSmtpAccount.h

		$Id$
*/

#ifndef _BmSmtpAccount_h
#define _BmSmtpAccount_h

#include <stdexcept>
#include <vector>

#include <Archivable.h>
#include <List.h>
#include <String.h>

// <needed to compile under BONE>
#include <socket.h>
#ifdef BONE_VERSION
#include <netinet/in.h>
#endif
// </needed to compile under BONE>

#include <NetAddress.h>

#include "BmDataModel.h"
#include "BmMail.h"

class BmSmtpAccountList;
/*------------------------------------------------------------------------------*\
	BmSmtpAccount 
		-	holds information about one specific SMTP-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmSmtpAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	typedef vector< BmRef< BmMail> > BmMailVect;

	// archivable components:
	static const char* const MSG_NAME = 			"bm:name";
	static const char* const MSG_USERNAME = 		"bm:username";
	static const char* const MSG_PASSWORD = 		"bm:password";
	static const char* const MSG_SMTP_SERVER = 	"bm:smtpserver";
	static const char* const MSG_DOMAIN = 			"bm:domain";
	static const char* const MSG_AUTH_METHOD = 	"bm:authmethod";
	static const char* const MSG_PORT_NR = 		"bm:portnr";
	static const char* const MSG_ACC_FOR_SAP = 	"bm:accForSmtpAfterPop";
	static const char* const MSG_STORE_PWD = 		"bm:storepwd";
	static const int16 nArchiveVersion = 1;

public:
	BmSmtpAccount( const char* name, BmSmtpAccountList* model);
	BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model);
	virtual ~BmSmtpAccount() 				{}
	
	// native methods:
	bool NeedsAuthViaPopServer();
	void SendQueuedMail();

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	const BString &Name() const 			{ return Key(); }
	const BString &Username() const 		{ return mUsername; }
	const BString &Password() const 		{ return mPassword; }
	bool PwdStoredOnDisk() const			{ return mPwdStoredOnDisk; }
	const BString &SMTPServer() const	{ return mSMTPServer; }
	const BString &DomainToAnnounce() const 	{ return mDomainToAnnounce; }
	const BString &AuthMethod() const 	{ return mAuthMethod; }
	int16 PortNr() const 					{ return mPortNr; }
	const BString &AccForSmtpAfterPop() const 	{ return mAccForSmtpAfterPop; }

	// setters:
	void Username( const BString &s) 	{ mUsername = s; }
	void Password( const BString &s) 	{ mPassword = s; }
	void PwdStoredOnDisk( bool b)			{ mPwdStoredOnDisk = b; }
	void SMTPServer( const BString &s)	{ mSMTPServer = s; }
	void DomainToAnnounce( const BString &s) 	{ mDomainToAnnounce = s; }
	void AuthMethod( const BString &s) 	{ mAuthMethod = s; }
	void PortNr( int16 i) 					{ mPortNr = i; }
	void AccForSmtpAfterPop( const BString &s) 	{ mAccForSmtpAfterPop = s; }

	bool GetSMTPAddress( BNetAddress* addr) const;

	BmMailVect mMailVect;			// vector with mails that shall be sent

private:
	BmSmtpAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmSmtpAccount( const BmSmtpAccount&);
	BmSmtpAccount operator=( const BmSmtpAccount&);

	//BString mName;					// name is stored in key (base-class)
	BString mUsername;
	BString mPassword;
	BString mSMTPServer;				// 
	BString mDomainToAnnounce;		// domain-name that will be used when we announce
											// ourselves to the server (HELO/EHLO)
	BString mAuthMethod;				// authentication method to use
	int16 mPortNr;						// usually 25
	BString mAccForSmtpAfterPop;	// POP-account to use for SmtpAfterPop
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?

};


/*------------------------------------------------------------------------------*\
	BmSmtpAccountList 
		-	holds list of all Smtp-Accounts
		-	
\*------------------------------------------------------------------------------*/
class BmSmtpAccountList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 1;

public:
	// creator-func, c'tors and d'tor:
	static BmSmtpAccountList* CreateInstance();
	BmSmtpAccountList();
	~BmSmtpAccountList();
	
	// native methods:
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmSmtpAccountList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmSmtpAccountList( const BmSmtpAccountList&);
	BmSmtpAccountList operator=( const BmSmtpAccountList&);

};

#define TheSmtpAccountList BmSmtpAccountList::theInstance

#endif
