/*
	BmPopAccount.h
		$Id$
*/

#ifndef _BmPopAccount_h
#define _BmPopAccount_h

#include <string>
#include <stdexcept>

#include <Archivable.h>
#include <NetAddress.h>

#include "BmUtil.h"

// -----------------------------------------------
class BmPopAccount : public BArchivable {
public:
	static char* const MSG_NAME = 			"bm:name";
	static char* const MSG_USERNAME = 		"bm:username";
	static char* const MSG_PASSWORD = 		"bm:password";
	static char* const MSG_POPSERVER = 		"bm:popserver";
	static char* const MSG_SMTPSERVER = 	"bm:smtpserver";
	static char* const MSG_REALNAME = 		"bm:realname";
	static char* const MSG_REPLYTO = 		"bm:replyto";
	static char* const MSG_SIGNATURENAME = "bm:signaturename";
	static char* const MSG_CHECKMAIL = 		"bm:checkmail";
	static char* const MSG_DELETEMAIL = 	"bm:deletemail";
	static char* const MSG_PORTNR = 			"bm:portnr";
public:
	BmPopAccount( void) : BArchivable() 
			{}
	BmPopAccount( BMessage *archive)
			;
	virtual ~BmPopAccount() 
			{}

	static BArchivable *Instantiate( BMessage *archive)
			;
	virtual status_t Archive( BMessage *archive, bool deep = true) const
			;

	// getters:
	const BString &Name() const 		{ return mName; }
	const BString &Username() const 	{ return mUsername; }
	const BString &Password() const 	{ return mPassword; }
	const BString &POPServer() const	{ return mPOPServer; }
	const BString &SMTPServer() const { return mSMTPServer; }
	const BString &RealName() const 	{ return mRealName; }
	const BString &ReplyTo() const 	{ return mReplyTo; }
	const BString &SignatureName() const		 { return mSignatureName; }
	bool CheckMail() const 				{ return mCheckMail; }
	bool DeleteMailFromServer() const	 { return mDeleteMailFromServer; }
	int16 PortNr() const 				{ return mPortNr; }

	// setters:
	void Name( const BString &s) 		{ mName = s; }
	void Username( const BString &s) 	{ mUsername = s; }
	void Password( const BString &s) 	{ mPassword = s; }
	void POPServer( const BString &s)	{ mPOPServer = s; }
	void SMTPServer( const BString &s){ mSMTPServer = s; }
	void RealName( const BString &s) 	{ mRealName = s; }
	void ReplyTo( const BString &s) 	{ mReplyTo = s; }
	void SignatureName( const BString &s)	 { mSignatureName = s; }
	void CheckMail( bool b) 			{ mCheckMail = b; }
	void DeleteMailFromServer( bool b)	 { mDeleteMailFromServer = b; }
	void PortNr( int16 i) 				{ mPortNr = i; }

	BNetAddress POPAddress() const
			;
	BNetAddress SMTPAddress() const
			;
private:
	BString mName;
	BString mUsername;
	BString mPassword;
	BString mPOPServer;
	BString mSMTPServer;
	BString mRealName;
	BString mReplyTo;
	BString mSignatureName;
	bool mCheckMail;
	bool mDeleteMailFromServer;
	int16 mPortNr;
};

#endif
