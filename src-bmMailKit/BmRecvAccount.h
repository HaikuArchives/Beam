/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmRecvAccount_h
#define _BmRecvAccount_h

#include "BmMailKit.h"

#include <map>

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include "BmDataModel.h"

class BHandler;
class BNetAddress;
class BmRecvAccountList;

class BMessageRunner;
/*------------------------------------------------------------------------------*\
	BmRecvAccount 
		-	holds information about one specific IMAP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmRecvAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	friend BmRecvAccountList;

	typedef map<BmString, time_t> BmUidMap;
public:
	BmRecvAccount( const char* name, BmRecvAccountList* model);
	BmRecvAccount( BMessage* archive, BmRecvAccountList* model);
	virtual ~BmRecvAccount();
	
	// native methods:
	bool IsUIDDownloaded( const BmString& uid) const;
	void MarkUIDAsDownloaded( const BmString& uid);
	bool ShouldUIDBeDeletedFromServer( const BmString& uid, 
												  BmString& logOutput) const;
	BmString AdjustToCurrentServerUids( const vector<BmString>& serverUids);
	//	
	BmString GetDomainName() const;
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;

	virtual const char* Type() const	= 0;
	virtual int32 JobType() const	= 0;
	virtual const char* DefaultPort(bool encrypted) const = 0;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	void ExecuteAction( BMessage* action);

	// getters:
	inline const BmString &EncryptionType() const	
													{ return mEncryptionType; }
	inline const BmString &AuthMethod() const	
													{ return mAuthMethod; }
	inline bool CheckMail() const 		{ return mCheckMail; }
	inline bool DeleteMailFromServer() const	
													{ return mDeleteMailFromServer; }
	inline uint16 DeleteMailDelay() const
													{ return mDeleteMailDelay; }
	inline const BmString &DeleteMailDelayString() const
													{ return mDeleteMailDelayString; }
	inline bool MarkedAsDefault() const	{ return mMarkedAsDefault; }
	inline const BmString &Name() const { return Key(); }
	inline const BmString &Password() const
													{ return mPassword; }
	inline const BmString &Server() const	
													{ return mServer; }
	inline uint16 PortNr() const			{ return mPortNr; }
	inline const BmString &PortNrString() const
													{ return mPortNrString; }
	inline bool PwdStoredOnDisk() const	{ return mPwdStoredOnDisk; }
	inline const BmString &Username() const 	
													{ return mUsername; }
	inline int16 CheckInterval() const 	{ return mCheckInterval; }
	inline const BmString &CheckIntervalString() const
													{ return mCheckIntervalString; }
	inline const BmString &FilterChain() const
													{ return mFilterChain; }
	inline const BmString &HomeFolder() const 
													{ return mHomeFolder; }
	inline const BmString &ClientCertificate() const 
													{ return mClientCertificate; }
	inline const BmString &AcceptedCertID() const 
													{ return mAcceptedCertID; }

	// setters:
	inline void EncryptionType( const BmString &s)
													{ mEncryptionType = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void AuthMethod( const BmString &s)
													{ mAuthMethod = s; 
													  TellModelItemUpdated( UPD_ALL); }
	inline void CheckMail( bool b) 		{ mCheckMail = b;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void DeleteMailFromServer( bool b)	
													{ mDeleteMailFromServer = b;
													  TellModelItemUpdated( UPD_ALL); }
	inline void DeleteMailDelay( uint16 i)			
													{ mDeleteMailDelay = i; 
													  mDeleteMailDelayString 
													  		= BmString()<<(uint32)i;
													  TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsDefault( bool b){ mMarkedAsDefault = b;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void Password( const BmString &s) 	
													{ mPassword = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void Server( const BmString &s)	
													{ mServer = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void PortNr( uint16 i)			{ mPortNr = i; 
													  mPortNrString 
													  		= BmString()<<(uint32)i;
													  TellModelItemUpdated( UPD_ALL); }
	inline void PwdStoredOnDisk( bool b){ mPwdStoredOnDisk = b;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void Username( const BmString &s) 	
													{ mUsername = s;  
													  TellModelItemUpdated( UPD_ALL); }
	void CheckInterval( int16 i);
	inline void FilterChain( const BmString &s)
													{ mFilterChain = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void HomeFolder( const BmString &s)
													{ mHomeFolder = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void ClientCertificate( const BmString &s)
													{ mClientCertificate = s;  
													  TellModelItemUpdated( UPD_ALL); }
	inline void AcceptedCertID( const BmString &s)
													{ mAcceptedCertID = s;  
													  TellModelItemUpdated( UPD_ALL); }

	static const char* const ENCR_AUTO;
	static const char* const ENCR_STARTTLS;
	static const char* const ENCR_TLS;
	static const char* const ENCR_SSL;

	static const char* const AUTH_AUTO;
	static const char* const AUTH_CRAM_MD5;
	static const char* const AUTH_DIGEST_MD5;
	static const char* const AUTH_NONE;

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_USERNAME;
	static const char* const MSG_PASSWORD;
	static const char* const MSG_SERVER;
	static const char* const MSG_CHECK_MAIL;
	static const char* const MSG_DELETE_MAIL;
	static const char* const MSG_PORT_NR;
	static const char* const MSG_UID;
	static const char* const MSG_UID_TIME;
	static const char* const MSG_ENCRYPTION_TYPE;
	static const char* const MSG_AUTH_METHOD;
	static const char* const MSG_MARK_DEFAULT;
	static const char* const MSG_STORE_PWD;
	static const char* const MSG_CHECK_INTERVAL;
	static const char* const MSG_FILTER_CHAIN;
	static const char* const MSG_HOME_FOLDER;
	static const char* const MSG_DELETE_DELAY;
	static const char* const MSG_TYPE;
	static const char* const MSG_CLIENT_CERT;
	static const char* const MSG_ACCEPTED_CERT;
	static const int16 nArchiveVersion;

protected:
	void SetupIntervalRunner();

	//BmString mName;					// name is stored in key (base-class)
	BmString mUsername;
	BmString mPassword;
	BmString mServer;
	uint16 mPortNr;					// usually 110 or 143
	BmString mPortNrString;			// mPortNr as String
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	int16 mDeleteMailDelay;			// delete delay in days
	BmString mDeleteMailDelayString;		// mDeleteMailDelay as String
	BmString mEncryptionType;		// type of encryption
	BmString mAuthMethod;			// authentication method
	bool mMarkedAsDefault;			// is this the default account?
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	int16 mCheckInterval;			// check mail every ... minutes
	BmString mCheckIntervalString;// check-interval as String
	BmString mFilterChain;			// the filter-chain to be used by this account
	BmString mHomeFolder;			// the folder where mails from this account
											// shall be stored into (by default, filters 
											// may change that)
	BmString mClientCertificate;	// the client certificate that is going to
											// be used during SSL/TLS handshake
	BmString mAcceptedCertID;		// ID of certificate that has been explicitly
											// accepted by user

	BmUidMap mUIDs;					// maps UIDs to time downloaded
	BMessageRunner* mIntervalRunner;

private:
	BmRecvAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmRecvAccount( const BmRecvAccount&);
	BmRecvAccount operator=( const BmRecvAccount&);
};


/*------------------------------------------------------------------------------*\
	BmRecvAccountList 
		-	holds list of all Recv-Accounts
		-	includes functionality for checking some/all servers for new mail
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmRecvAccountList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmRecvAccountList* CreateInstance();
	BmRecvAccountList();
	~BmRecvAccountList();
	
	// native methods:
	void CheckMail( bool allAccounts=false);
	void CheckMailFor( BmString accName, bool isAutoCheck=false);
	//
	void ResetToSaved();
	
	// overrides of listmodel base:
	void ForeignKeyChanged( const BmString& key, const BmString& oldVal, 
									const BmString& newVal);
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmRecvAccountList> theInstance;

	static const char* const MSG_AUTOCHECK;

private:
	// Hide copy-constructor and assignment:
	BmRecvAccountList( const BmRecvAccountList&);
	BmRecvAccountList operator=( const BmRecvAccountList&);
	
};

#define TheRecvAccountList BmRecvAccountList::theInstance

#endif
