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

#include "BmMailKit.h"

#include <vector>

#include <Archivable.h>
#include <List.h>
#include "BmString.h"

#include "BmDataModel.h"

class BHandler;
class BNetAddress;
class BmPopAccountList;


enum {
	BM_JOBWIN_POP				= 'bmea'
							// sent to JobMetaController in order to 
							// start pop-connection
};

class BMessageRunner;
struct BmUidInfo {
	BmString uid;
	int32 timeDownloaded;
};
typedef vector<BmUidInfo> BmUidVect;
/*------------------------------------------------------------------------------*\
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmPopAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	friend BmPopAccountList;


public:
	BmPopAccount( const char* name, BmPopAccountList* model);
	BmPopAccount( BMessage* archive, BmPopAccountList* model);
	virtual ~BmPopAccount();
	
	// native methods:
	bool IsUIDDownloaded( const BmString& uid, time_t* downloadTime=NULL);
	void MarkUIDAsDownloaded( const BmString& uid);
	BmString AdjustToCurrentServerUids( const vector<BmString>& serverUids);
	//	
	BmString GetDomainName() const;
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	void IntegrateAppendedArchive( BMessage* archive);

	// getters:
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
	inline const BmString &POPServer() const	
													{ return mPOPServer; }
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

	// setters:
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
	inline void POPServer( const BmString &s)	
													{ mPOPServer = s;  
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

	void AddressInfo( BmString& server, uint16& port) const;

	static const char* const AUTH_POP3;
	static const char* const AUTH_APOP;

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_USERNAME;
	static const char* const MSG_PASSWORD;
	static const char* const MSG_POP_SERVER;
	static const char* const MSG_CHECK_MAIL;
	static const char* const MSG_DELETE_MAIL;
	static const char* const MSG_PORT_NR;
	static const char* const MSG_UID;
	static const char* const MSG_UID_TIME;
	static const char* const MSG_AUTH_METHOD;
	static const char* const MSG_MARK_DEFAULT;
	static const char* const MSG_STORE_PWD;
	static const char* const MSG_CHECK_INTERVAL;
	static const char* const MSG_FILTER_CHAIN;
	static const char* const MSG_HOME_FOLDER;
	static const char* const MSG_DELETE_DELAY;
	static const int16 nArchiveVersion;

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
	uint16 mPortNr;					// usually 110
	BmString mPortNrString;			// mPortNr as String
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	int16 mDeleteMailDelay;			// delete delay in days
	BmString mDeleteMailDelayString;		// mDeleteMailDelay as String
	BmString mAuthMethod;			// authentication method
	bool mMarkedAsDefault;			// is this the default account?
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?
	int16 mCheckInterval;			// check mail every ... minutes
	BmString mCheckIntervalString;// check-interval as String
	BmString mFilterChain;			// the filter-chain to be used by this account
	BmString mHomeFolder;			// the folder where mails from this account
											// shall be stored into (by default, filters 
											// may change that)

	BmUidVect mUIDs;					// list of UIDs downloaded by this account
	BMessageRunner* mIntervalRunner;
};


/*------------------------------------------------------------------------------*\
	BmPopAccountList 
		-	holds list of all Pop-Accounts
		-	includes functionality for checking some/all POP-servers for new mail
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmPopAccountList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmPopAccountList* CreateInstance();
	BmPopAccountList();
	~BmPopAccountList();
	
	// native methods:
	void CheckMail( bool allAccounts=false);
	void CheckMailFor( BmString accName, bool isAutoCheck=false);
	//
	void ResetToSaved();
	
	// overrides of listmodel base:
	void ForeignKeyChanged( const BmString& key, const BmString& oldVal, 
									const BmString& newVal);
	const BmString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmPopAccountList> theInstance;

	static const char* const MSG_AUTOCHECK;

private:
	// Hide copy-constructor and assignment:
	BmPopAccountList( const BmPopAccountList&);
	BmPopAccountList operator=( const BmPopAccountList&);
	
};

#define ThePopAccountList BmPopAccountList::theInstance

#endif
