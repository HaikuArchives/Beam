/*
	BmPopAccount.h

		$Id$
*/

#ifndef _BmPopAccount_h
#define _BmPopAccount_h

#include <stdexcept>

#include <Archivable.h>
#include <String.h>
#include <NetAddress.h>

#include "BmUtil.h"

/*------------------------------------------------------------------------------
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
  ------------------------------------------------------------------------------*/
class BmPopAccount : public BArchivable {
	// archivable components:
	static char* const MSG_NAME = 			"bm:name";
	static char* const MSG_USERNAME = 		"bm:username";
	static char* const MSG_PASSWORD = 		"bm:password";
	static char* const MSG_POP_SERVER = 	"bm:popserver";
	static char* const MSG_SMTP_SERVER = 	"bm:smtpserver";
	static char* const MSG_REAL_NAME = 		"bm:realname";
	static char* const MSG_REPLY_TO = 		"bm:replyto";
	static char* const MSG_SIGNATURE_NAME = "bm:signaturename";
	static char* const MSG_CHECK_MAIL = 	"bm:checkmail";
	static char* const MSG_DELETE_MAIL = 	"bm:deletemail";
	static char* const MSG_PORT_NR = 		"bm:portnr";
	static char* const MSG_SMTP_PORT_NR = 	"bm:smtpportnr";
public:
	BmPopAccount( void) 
		: BArchivable() 
		, mCheckMail( false)
		, mDeleteMailFromServer( false)
			{}
	BmPopAccount( BMessage *archive)
			;
	virtual ~BmPopAccount() 
			{}

	// stuff needed for BArchivable:
	static BArchivable *Instantiate( BMessage *archive)
			;
	virtual status_t Archive( BMessage *archive, bool deep = true) const
			;

	// getters:
	const BString &Name() const 			{ return mName; }
	const BString &Username() const 		{ return mUsername; }
	const BString &Password() const 		{ return mPassword; }
	const BString &POPServer() const		{ return mPOPServer; }
	const BString &SMTPServer() const	{ return mSMTPServer; }
	const BString &RealName() const 		{ return mRealName; }
	const BString &ReplyTo() const 		{ return mReplyTo; }
	const BString &SignatureName() const	 { return mSignatureName; }
	bool CheckMail() const 					{ return mCheckMail; }
	bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	int16 PortNr() const 					{ return mPortNr; }
	int16 SMTPPortNr() const 				{ return mSMTPPortNr; }

	// setters:
	void Name( const BString &s) 			{ mName = s; }
	void Username( const BString &s) 	{ mUsername = s; }
	void Password( const BString &s) 	{ mPassword = s; }
	void POPServer( const BString &s)	{ mPOPServer = s; }
	void SMTPServer( const BString &s)	{ mSMTPServer = s; }
	void RealName( const BString &s) 	{ mRealName = s; }
	void ReplyTo( const BString &s) 		{ mReplyTo = s; }
	void SignatureName( const BString &s)	 { mSignatureName = s; }
	void CheckMail( bool b) 				{ mCheckMail = b; }
	void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b; }
	void PortNr( int16 i) 					{ mPortNr = i; }
	void SMTPPortNr( int16 i) 				{ mSMTPPortNr = i; }

	BNetAddress POPAddress() const
			;
	BNetAddress SMTPAddress() const
			;
private:
	BString mName;						// name of this POP-account
	BString mUsername;
	BString mPassword;
	BString mPOPServer;
	BString mSMTPServer;				// SMTP-Server to use when sending 
											// mail "from" this POP-account
	BString mRealName;
	BString mReplyTo;
	BString mSignatureName;			// name&path of signature file
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	int16 mPortNr;						// usually 110
	int16 mSMTPPortNr;				// usually 25
};

#endif
