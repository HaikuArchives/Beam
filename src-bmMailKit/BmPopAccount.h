/*
	BmPopAccount.h

		$Id$
*/

#ifndef _BmPopAccount_h
#define _BmPopAccount_h

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

class BmPopAccountList;
/*------------------------------------------------------------------------------*\
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPopAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
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
	static const int16 nArchiveVersion = 1;

public:
	BmPopAccount( const char* name, BmPopAccountList* model);
	BmPopAccount( BMessage* archive, BmPopAccountList* model);
	virtual ~BmPopAccount() 				{}
	
	// native methods:
	bool IsUIDDownloaded( BString uid);
	void MarkUIDAsDownloaded( BString uid);
	BString GetFromAddress() const;

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BString &AuthMethod() const	{ return mAuthMethod; }
	inline bool CheckMail() const 				{ return mCheckMail; }
	inline bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	inline const BString &MailAddr() const 	{ return mMailAddr; }
	inline bool MarkedAsDefault() const			{ return mMarkedAsDefault; }
	inline const BString &Name() const 			{ return Key(); }
	inline const BString &Password() const 	{ return mPassword; }
	inline const BString &POPServer() const	{ return mPOPServer; }
	inline int16 PortNr() const 					{ return mPortNr; }
	inline bool PwdStoredOnDisk() const			{ return mPwdStoredOnDisk; }
	inline const BString &RealName() const 	{ return mRealName; }
	inline const BString &SignatureName() const	 { return mSignatureName; }
	inline const BString &SMTPAccount() const	{ return mSMTPAccount; }
	inline const BString &Username() const 	{ return mUsername; }

	// setters:
	inline void AuthMethod( const BString &s) { mAuthMethod = s; }
	inline void CheckMail( bool b) 				{ mCheckMail = b; }
	inline void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b; }
	inline void MailAddr( const BString &s) 	{ mMailAddr = s; }
	inline void MarkedAsDefault( bool b)		{ mMarkedAsDefault = b; }
	inline void Password( const BString &s) 	{ mPassword = s; }
	inline void POPServer( const BString &s)	{ mPOPServer = s; }
	inline void PortNr( int16 i) 					{ mPortNr = i; }
	inline void PwdStoredOnDisk( bool b)		{ mPwdStoredOnDisk = b; }
	inline void RealName( const BString &s) 	{ mRealName = s; }
	inline void SignatureName( const BString &s)	 { mSignatureName = s; }
	inline void SMTPAccount( const BString &s){ mSMTPAccount = s; }
	inline void Username( const BString &s) 	{ mUsername = s; }

	bool GetPOPAddress( BNetAddress* addr) const;

private:
	BmPopAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmPopAccount( const BmPopAccount&);
	BmPopAccount operator=( const BmPopAccount&);

	//BString mName;					// name is stored in key (base-class)
	BString mUsername;
	BString mPassword;
	BString mPOPServer;
	BString mSMTPAccount;			// name of BmSmtpAccount to use when sending 
											// mail "from" this POP-account
	BString mRealName;
	BString mMailAddr;				// address to use (instead of composed address)
	BString mSignatureName;			// name&path of signature file
	int16 mPortNr;						// usually 110
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	BString mAuthMethod;				// authentication method
	bool mMarkedAsDefault;			// is this the default account?
	bool mPwdStoredOnDisk;			// store Passwords unsafely on disk?

	vector<BString> mUIDs;			// list of UIDs seen in this account

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
	static BmPopAccountList* CreateInstance();
	BmPopAccountList();
	~BmPopAccountList();
	
	// native methods:
	void CheckMail();
	void CheckMailFor( BString accName);
	BmRef<BmPopAccount> DefaultAccount();
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmPopAccountList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmPopAccountList( const BmPopAccountList&);
	BmPopAccountList operator=( const BmPopAccountList&);

};

#define ThePopAccountList BmPopAccountList::theInstance

#endif
