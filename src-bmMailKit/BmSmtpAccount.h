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

#define BM_JOBWIN_SMTP					'bmeb'
						// sent to JobMetaController in order to start smtp-connection

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
	static const int16 nArchiveVersion = 2;

public:
	BmSmtpAccount( const char* name, BmSmtpAccountList* model);
	BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model);
	virtual ~BmSmtpAccount();
	
	// native methods:
	bool NeedsAuthViaPopServer();

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BString &Name() const 			{ return Key(); }
	inline const BString &Username() const 	{ return mUsername; }
	inline const BString &Password() const 	{ return mPassword; }
	inline bool PwdStoredOnDisk() const			{ return mPwdStoredOnDisk; }
	inline const BString &SMTPServer() const	{ return mSMTPServer; }
	inline const BString &DomainToAnnounce() const 	{ return mDomainToAnnounce; }
	inline const BString &AuthMethod() const 	{ return mAuthMethod; }
	inline int16 PortNr() const 					{ return mPortNr; }
	inline const BString &PortNrString() const{ return mPortNrString; }
	inline const BString &AccForSmtpAfterPop() const{ return mAccForSmtpAfterPop; }

	// setters:
	inline void Username( const BString &s) 	{ mUsername = s;   TellModelItemUpdated( UPD_ALL); }
	inline void Password( const BString &s) 	{ mPassword = s;   TellModelItemUpdated( UPD_ALL); }
	inline void PwdStoredOnDisk( bool b)		{ mPwdStoredOnDisk = b;   TellModelItemUpdated( UPD_ALL); }
	inline void SMTPServer( const BString &s)	{ mSMTPServer = s;   TellModelItemUpdated( UPD_ALL); }
	inline void DomainToAnnounce( const BString &s) 	{ mDomainToAnnounce = s;   TellModelItemUpdated( UPD_ALL); }
	inline void AuthMethod( const BString &s) { mAuthMethod = s;   TellModelItemUpdated( UPD_ALL); }
	inline void PortNr( int16 i) 					{ mPortNr = i; mPortNrString = BString()<<i;  TellModelItemUpdated( UPD_ALL); }
	inline void AccForSmtpAfterPop( const BString &s)	{ mAccForSmtpAfterPop = s;   TellModelItemUpdated( UPD_ALL); }

	bool GetSMTPAddress( BNetAddress* addr) const;

	BmMailVect mMailVect;			// vector with mails that shall be sent

	static const char* const AUTH_SMTP_AFTER_POP = 	"SMTP-AFTER-POP";
	static const char* const AUTH_PLAIN = 				"PLAIN";
	static const char* const AUTH_LOGIN = 				"LOGIN";

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
	BString mPortNrString;			// Port-Nr as String
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	BString mAccForSmtpAfterPop;	// pop-account to use for authentication

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
	static BmSmtpAccountList* CreateInstance( BLooper* jobMetaController);
	BmSmtpAccountList( BLooper* jobMetaController);
	~BmSmtpAccountList();
	
	// native methods:
	void SendQueuedMailFor( const BString accName);
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	
	static BmRef<BmSmtpAccountList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmSmtpAccountList( const BmSmtpAccountList&);
	BmSmtpAccountList operator=( const BmSmtpAccountList&);

	BLooper* mJobMetaController;
};

#define TheSmtpAccountList BmSmtpAccountList::theInstance

#endif
