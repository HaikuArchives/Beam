/*
	BmSmtpAccount.h

		$Id$
*/

#ifndef _BmSmtpAccount_h
#define _BmSmtpAccount_h

#include <stdexcept>

#include <Archivable.h>
#include <List.h>
#include <String.h>
#include <NetAddress.h>

#include "BmDataModel.h"

class BmSmtpAccountList;
/*------------------------------------------------------------------------------*\
	BmSmtpAccount 
		-	holds information about one specific SMTP-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmSmtpAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	// archivable components:
	static const char* const MSG_NAME = 			"bm:name";
	static const char* const MSG_USERNAME = 		"bm:username";
	static const char* const MSG_PASSWORD = 		"bm:password";
	static const char* const MSG_SMTP_SERVER = 	"bm:smtpserver";
	static const char* const MSG_DOMAIN = 			"bm:domain";
	static const char* const MSG_AUTH_METHOD = 	"bm:authmethod";
	static const char* const MSG_PORT_NR = 		"bm:portnr";

	static const int BM_SMTP_NO_AUTH = 0;
	static const int BM_SMTP_AFTER_POP = 1;
	static const int BM_SMTP_AUTH = 2;

public:
	BmSmtpAccount( const char* name, BmSmtpAccountList* model);
	BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model);
	virtual ~BmSmtpAccount() 				{}
	
	// native methods:
	bool NeedsAuthViaPopServer();

	// stuff needed for BArchivable:
	static BArchivable* Instantiate( BMessage* archive);
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const BString &Name() const 			{ return Key(); }
	const BString &Username() const 		{ return mUsername; }
	const BString &Password() const 		{ return mPassword; }
	const BString &SMTPServer() const	{ return mSMTPServer; }
	const BString &DomainToAnnounce() const 	{ return mDomainToAnnounce; }
	int16 PortNr() const 					{ return mPortNr; }
	int16 AuthMethod() const 				{ return mAuthMethod; }

	// setters:
	void Username( const BString &s) 	{ mUsername = s; }
	void Password( const BString &s) 	{ mPassword = s; }
	void SMTPServer( const BString &s)	{ mSMTPServer = s; }
	void DomainToAnnounce( const BString &s) 	{ mDomainToAnnounce = s; }
	void PortNr( int16 i) 					{ mPortNr = i; }
	void AuthMethod( int16 i) 				{ mAuthMethod = i; }

	bool GetSMTPAddress( BNetAddress* addr) const;

private:
	BmSmtpAccount();					// hide default constructor

	//BString mName;					// name is stored in key (base-class)
	BString mUsername;
	BString mPassword;
	BString mSMTPServer;				// 
	BString mDomainToAnnounce;		// domain-name that will be used when we announce
											// ourselves to the server (HELO/EHLO)
	int16 mPortNr;						// usually 25
	int16 mAuthMethod;				// 0 = none,
											// 1 = SMTPafterPOP,
											// 2 = AUTH-command
};


/*------------------------------------------------------------------------------*\
	BmSmtpAccountList 
		-	holds list of all Smtp-Accounts
		-	
\*------------------------------------------------------------------------------*/
class BmSmtpAccountList : public BmListModel {
	typedef BmListModel inherited;
public:
	// creator-func, c'tors and d'tor:
	static BmSmtpAccountList* CreateInstance();
	BmSmtpAccountList();
	~BmSmtpAccountList();
	
	// native methods:
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);

	static BmRef<BmSmtpAccountList> theInstance;

private:
};

#define TheSmtpAccountList BmSmtpAccountList::theInstance

#endif
