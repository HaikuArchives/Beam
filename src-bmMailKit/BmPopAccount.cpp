/*
	BmPopAccount.cpp

		$Id$
*/

#include <Message.h>
#include <ByteOrder.h>

#include "BmPopAccount.h"

/*------------------------------------------------------------------------------*\
	BmPopAccount( archive)
		-	constructs a BmPopAccount from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( BMessage *archive) 
	: BArchivable( archive)
{
	mName = FindMsgString( archive, MSG_NAME);
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mPOPServer = FindMsgString( archive, MSG_POP_SERVER);
	mSMTPServer = FindMsgString( archive, MSG_SMTP_SERVER);
	mRealName = FindMsgString( archive, MSG_REAL_NAME);
	mReplyTo = FindMsgString( archive, MSG_REPLY_TO);
	mSignatureName = FindMsgString( archive, MSG_SIGNATURE_NAME);
	mCheckMail = FindMsgBool( archive, MSG_CHECK_MAIL);
	mDeleteMailFromServer = FindMsgBool( archive, MSG_DELETE_MAIL);
	mPortNr = ntohs(FindMsgInt16( archive, MSG_PORT_NR));
	mSMTPPortNr = ntohs(FindMsgInt16( archive, MSG_SMTP_PORT_NR));
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPopAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPopAccount::Archive( BMessage *archive, bool deep) const {
	status_t ret = (BArchivable::Archive( archive, deep)
		||	archive->AddString("class", "BmPopAccount")
		||	archive->AddString( MSG_NAME, mName.String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_POP_SERVER, mPOPServer.String())
		||	archive->AddString( MSG_SMTP_SERVER, mSMTPServer.String())
		||	archive->AddString( MSG_REAL_NAME, mRealName.String())
		||	archive->AddString( MSG_REPLY_TO, mReplyTo.String())
		||	archive->AddString( MSG_SIGNATURE_NAME, mSignatureName.String())
		||	archive->AddBool( MSG_CHECK_MAIL, mCheckMail)
		||	archive->AddBool( MSG_DELETE_MAIL, mDeleteMailFromServer)
		||	archive->AddInt16( MSG_PORT_NR, htons(mPortNr))
		||	archive->AddInt16( MSG_SMTP_PORT_NR, htons(mSMTPPortNr)));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Instantiate( archive)
		-	(re-)creates a PopAccount from a given BMessage
\*------------------------------------------------------------------------------*/
BArchivable* BmPopAccount::Instantiate( BMessage *archive) {
	if (!validate_instantiation( archive, "BmPopAccount"))
		return NULL;
	return new BmPopAccount( archive);
}

/*------------------------------------------------------------------------------*\
	POPAddress()
		-	returns the POP3-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
BNetAddress BmPopAccount::POPAddress() const {
	BNetAddress addr( mPOPServer.String(), mPortNr);
	if (addr.InitCheck() == B_OK)
		return addr;
	else
		throw runtime_error("BmPopAccount: Could not create PopAddress");
}

/*------------------------------------------------------------------------------*\
	SMTPAddress()
		-	returns the SMTP-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
BNetAddress BmPopAccount::SMTPAddress() const {
	BNetAddress addr( mSMTPServer.String(), mSMTPPortNr);
	if (addr.InitCheck() == B_OK)
		return addr;
	else
		throw runtime_error("BmSMTPAccount: Could not create SMTPAddress");
}
