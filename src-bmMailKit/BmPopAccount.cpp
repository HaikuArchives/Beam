/*
	BmPopAccount.cpp

		$Id$
*/

#include <ByteOrder.h>
#include <File.h>
#include <Message.h>

#include "BmBasics.h"
#include "BmPopAccount.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPopAccount
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmPopAccount()
		-	
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( const char* name, BmPopAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mCheckMail( false)
	,	mDeleteMailFromServer( false)	
{
}

/*------------------------------------------------------------------------------*\
	BmPopAccount( archive)
		-	constructs a BmPopAccount from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( BMessage* archive, BmPopAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
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
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mSMTPPortNr = FindMsgInt16( archive, MSG_SMTP_PORT_NR);
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPopAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPopAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
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
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddInt16( MSG_SMTP_PORT_NR, mSMTPPortNr));
	return ret;
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
		throw BM_runtime_error("BmPopAccount: Could not create PopAddress");
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
		throw BM_runtime_error("BmSMTPAccount: Could not create SMTPAddress");
}



/********************************************************************************\
	BmPopAccountList
\********************************************************************************/


BmRef< BmPopAccountList> BmPopAccountList::theInstance( NULL);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmPopAccountList* BmPopAccountList::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmPopAccountList;
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmPopAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmPopAccountList::BmPopAccountList()
	:	inherited( "PopAccountList")
{
	StartJob();
	BmPopAccount* acc = new BmPopAccount( "testaccount", this);
		acc->Name( "mailtest@kiwi:110");
		acc->Username( "mailtest");
		acc->Password( "mailtest");
		acc->POPServer( "kiwi");
		acc->PortNr( 110);
		acc->SMTPPortNr( 25);
	AddItemToList( acc, NULL);
	Store();

}

/*------------------------------------------------------------------------------*\
	~BmPopAccountList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPopAccountList::~BmPopAccountList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BString BmPopAccountList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Pop Accounts";
}
