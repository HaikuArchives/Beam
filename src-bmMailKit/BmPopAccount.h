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
	const string &Name() const 			{ return mName; }
	const string &Username() const 		{ return mUsername; }
	const string &Password() const 		{ return mPassword; }
	const string &POPServer() const		{ return mPOPServer; }
	const string &SMTPServer() const	{ return mSMTPServer; }
	const string &RealName() const 		{ return mRealName; }
	const string &ReplyTo() const 		{ return mReplyTo; }
	const string &SignatureName() const	 { return mSignatureName; }
	bool CheckMail() const 					{ return mCheckMail; }
	bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	int16 PortNr() const 					{ return mPortNr; }
	int16 SMTPPortNr() const 				{ return mSMTPPortNr; }

	// setters:
	void Name( const string &s) 			{ mName = s; }
	void Username( const string &s) 	{ mUsername = s; }
	void Password( const string &s) 	{ mPassword = s; }
	void POPServer( const string &s)	{ mPOPServer = s; }
	void SMTPServer( const string &s)	{ mSMTPServer = s; }
	void RealName( const string &s) 	{ mRealName = s; }
	void ReplyTo( const string &s) 		{ mReplyTo = s; }
	void SignatureName( const string &s)	 { mSignatureName = s; }
	void CheckMail( bool b) 				{ mCheckMail = b; }
	void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b; }
	void PortNr( int16 i) 					{ mPortNr = i; }
	void SMTPPortNr( int16 i) 				{ mSMTPPortNr = i; }

	BNetAddress POPAddress() const
			;
	BNetAddress SMTPAddress() const
			;
private:
	string mName;						// name of this POP-account
	string mUsername;
	string mPassword;
	string mPOPServer;
	string mSMTPServer;				// SMTP-Server to use when sending 
											// mail "from" this POP-account
	string mRealName;
	string mReplyTo;
	string mSignatureName;			// name&path of signature file
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	int16 mPortNr;						// usually 110
	int16 mSMTPPortNr;				// usually 25
};

#endif
