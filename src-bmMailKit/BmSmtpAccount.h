/*
	BmSmtpAccount.h

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
	inline const BmString &AuthMethod() const 	
													{ return mAuthMethod; }
	inline uint16 PortNr() const			{ return mPortNr; }
	inline const BmString &PortNrString() const
													{ return mPortNrString; }
	inline const BmString &AccForSmtpAfterPop() const
													{ return mAccForSmtpAfterPop; }

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

	void AddressInfo( BmString& server, uint16& port) const;

	static const char* const AUTH_AUTO;
	static const char* const AUTH_SMTP_AFTER_POP;
	static const char* const AUTH_PLAIN;
	static const char* const AUTH_LOGIN;
	static const char* const AUTH_CRAM_MD5;
	static const char* const AUTH_DIGEST_MD5;

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_USERNAME;
	static const char* const MSG_PASSWORD;
	static const char* const MSG_SMTP_SERVER;
	static const char* const MSG_DOMAIN;
	static const char* const MSG_AUTH_METHOD;
	static const char* const MSG_PORT_NR;
	static const char* const MSG_ACC_FOR_SAP;
	static const char* const MSG_STORE_PWD;
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
	BmString mAuthMethod;			// authentication method to use
	uint16 mPortNr;					// usually 25
	BmString mPortNrString;			// Port-Nr as String
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	BmString mAccForSmtpAfterPop;	// pop-account to use for authentication
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
