/*
	BmPopAccount.cpp		-	$Id$
*/

#include <be/app/Message.h>
#include <be/support/ByteOrder.h>
#include "BmPopAccount.h"

//---------------------------------------
BmPopAccount::BmPopAccount( BMessage *archive) 
	: BArchivable( archive)
{
	mName = FindMsgString( archive, MSG_NAME);
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mPOPServer = FindMsgString( archive, MSG_POPSERVER);
	mSMTPServer = FindMsgString( archive, MSG_SMTPSERVER);
	mRealName = FindMsgString( archive, MSG_REALNAME);
	mReplyTo = FindMsgString( archive, MSG_REPLYTO);
	mCheckMail = FindMsgBool( archive, MSG_CHECKMAIL);
	mDeleteMailFromServer = FindMsgBool( archive, MSG_DELETEMAIL);
	mPortNr = ntohs(FindMsgInt16( archive, MSG_PORTNR));
}

//---------------------------------------
status_t BmPopAccount::Archive( BMessage *archive, bool deep) const {
	status_t ret = (BArchivable::Archive( archive, deep)
		||	archive->AddString("class", "BmPopAccount")
		||	archive->AddString( MSG_NAME, mName)
		||	archive->AddString( MSG_USERNAME, mUsername)
		||	archive->AddString( MSG_PASSWORD, mPassword)
		||	archive->AddString( MSG_POPSERVER, mPOPServer)
		||	archive->AddString( MSG_SMTPSERVER, mSMTPServer)
		||	archive->AddString( MSG_REALNAME, mRealName)
		||	archive->AddString( MSG_REPLYTO, mReplyTo)
		||	archive->AddString( MSG_SIGNATURENAME, mSignatureName)
		||	archive->AddBool( MSG_CHECKMAIL, mCheckMail)
		||	archive->AddBool( MSG_DELETEMAIL, mDeleteMailFromServer)
		||	archive->AddInt16( MSG_PORTNR, htons(mPortNr)));
	return ret;
}

//---------------------------------------
BArchivable* BmPopAccount::Instantiate( BMessage *archive) {
	if (!validate_instantiation( archive, "BmPopAccount"))
		return NULL;
	return new BmPopAccount( archive);
}

//---------------------------------------
BNetAddress BmPopAccount::POPAddress() const {
	BNetAddress addr( mPOPServer.String(), mPortNr);
	if (addr.InitCheck() == B_OK)
		return addr;
	else
		throw runtime_error("BmPopAccount: Could not create PopAddress");
}

//---------------------------------------
BNetAddress BmPopAccount::SMTPAddress() const {
	BNetAddress addr( mSMTPServer.String(), 25);
	if (addr.InitCheck() == B_OK)
		return addr;
	else
		throw runtime_error("BmSMTPAccount: Could not create SMTPAddress");
}

