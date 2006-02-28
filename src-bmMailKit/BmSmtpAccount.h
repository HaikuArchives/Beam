/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmSmtpAccount_h
#define _BmSmtpAccount_h

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include "BmDataModel.h"
#include "BmMail.h"

enum {
	BM_JOBWIN_SMTP			= 'bmeb'
						// sent to JobMetaController in order to start
						// smtp-connection
};

class BNetAddress;
class BmSmtpAccountList;
/*------------------------------------------------------------------------------*\
	BmSmtpAccount 
		-	holds information about one specific SMTP-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmSmtpAccount : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmSmtpAccount( const char* name, BmSmtpAccountList* model);
	BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model);
	virtual ~BmSmtpAccount();
	
	// native methods:
	bool NeedsAuthViaPopServer();
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;
	//
	void SendMail( const entry_ref& eref);
	void SendPendingMails();

	const char* DefaultPort(bool encrypted) const {
		return encrypted ? "465" : "25";
	}

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BmString &Name() const { return Key(); }
	inline const BmString &Username() const 	
													{ return mUsername; }
	inline const BmString &Password() const 	
													{ return mPassword; }
	inline bool PwdStoredOnDisk() const	{ return mPwdStoredOnDisk; }
	inline const BmString &SMTPServer() const	
													{ return mSMTPServer; }
	inline const BmString &DomainToAnnounce() const 	
													{ return mDomainToAnnounce; }
	inline const BmString &EncryptionType() const 	
													{ return mEncryptionType; }
	inline const BmString &AuthMethod() const 	
													{ return mAuthMethod; }
	inline uint16 PortNr() const			{ return mPortNr; }
	inline const BmString &PortNrString() const
													{ return mPortNrString; }
	inline const BmString &AccForSmtpAfterPop() const
													{ return mAccForSmtpAfterPop; }
	inline const BmString &ClientCertificate() const 
													{ return mClientCertificate; }

	// setters:
	inline void Username( const BmString &s) 	
													{ mUsername = s;   
													  TellModelItemUpdated( UPD_ALL); }
	inline void Password( const BmString &s) 	
													{ mPassword = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void PwdStoredOnDisk( bool b){ mPwdStoredOnDisk = b; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void SMTPServer( const BmString &s)	
													{ mSMTPServer = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void DomainToAnnounce( const BmString &s) 	
													{ mDomainToAnnounce = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void EncryptionType( const BmString &s) 
													{ mEncryptionType = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void AuthMethod( const BmString &s) 
													{ mAuthMethod = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void PortNr( uint16 i)			{ mPortNr = i; 
													  mPortNrString 
													  		= BmString()<<(uint32)i;
													  TellModelItemUpdated( UPD_ALL); }
	inline void AccForSmtpAfterPop( const BmString &s)	
													{ mAccForSmtpAfterPop = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void ClientCertificate( const BmString &s)
													{ mClientCertificate = s;  
													  TellModelItemUpdated( UPD_ALL); }

	static const char* const ENCR_AUTO;
	static const char* const ENCR_STARTTLS;
	static const char* const ENCR_TLS;
	static const char* const ENCR_SSL;

	static const char* const AUTH_AUTO;
	static const char* const AUTH_SMTP_AFTER_POP;
	static const char* const AUTH_PLAIN;
	static const char* const AUTH_LOGIN;
	static const char* const AUTH_CRAM_MD5;
	static const char* const AUTH_DIGEST_MD5;
	static const char* const AUTH_NONE;

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_USERNAME;
	static const char* const MSG_PASSWORD;
	static const char* const MSG_SMTP_SERVER;
	static const char* const MSG_DOMAIN;
	static const char* const MSG_ENCRYPTION_TYPE;
	static const char* const MSG_AUTH_METHOD;
	static const char* const MSG_PORT_NR;
	static const char* const MSG_ACC_FOR_SAP;
	static const char* const MSG_STORE_PWD;
	static const char* const MSG_CLIENT_CERT;
	static const int16 nArchiveVersion;

	// message field names:
	static const char* const MSG_REF;

private:
	BmSmtpAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmSmtpAccount( const BmSmtpAccount&);
	BmSmtpAccount operator=( const BmSmtpAccount&);

	//BmString mName;					// name is stored in key (base-class)
	BmString mUsername;
	BmString mPassword;
	BmString mSMTPServer;			// 
	BmString mDomainToAnnounce;	// domain-name that will be used when we
											// announce ourselves to the server (HELO/EHLO)
	BmString mEncryptionType;		// type of encryption to use
	BmString mAuthMethod;			// authentication method to use
	uint16 mPortNr;					// usually 25
	BmString mPortNrString;			// Port-Nr as String
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	BmString mAccForSmtpAfterPop;	// pop-account to use for authentication
	BmString mClientCertificate;	// the client certificate that is going to
											// be used during SSL/TLS handshake
};


/*------------------------------------------------------------------------------*\
	BmSmtpAccountList 
		-	holds list of all Smtp-Accounts
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmSmtpAccountList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmSmtpAccountList* CreateInstance();
	BmSmtpAccountList();
	~BmSmtpAccountList();
	
	// native methods:
	void SendPendingMails();
	void SendPendingMailsFor( const BmString accName);
	
	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	
	static BmRef<BmSmtpAccountList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmSmtpAccountList( const BmSmtpAccountList&);
	BmSmtpAccountList operator=( const BmSmtpAccountList&);
};

#define TheSmtpAccountList BmSmtpAccountList::theInstance

#endif
