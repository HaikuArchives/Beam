/*
	BmSmtpAccount.cpp

		$Id$
*/

#include <ByteOrder.h>
#include <File.h>
#include <Message.h>

#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmSmtpAccount.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSmtpAccount
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmSmtpAccount()
		-	
\*------------------------------------------------------------------------------*/
BmSmtpAccount::BmSmtpAccount( const char* name, BmSmtpAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mPortNr( 25)
	,	mAuthMethod( 0)
{
}

/*------------------------------------------------------------------------------*\
	BmSmtpAccount( archive)
		-	constructs a BmSmtpAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmSmtpAccount::BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
{
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mSMTPServer = FindMsgString( archive, MSG_SMTP_SERVER);
	mDomainToAnnounce = FindMsgString( archive, MSG_DOMAIN);
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mAuthMethod = FindMsgInt16( archive, MSG_AUTH_METHOD);
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSmtpAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmSmtpAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_SMTP_SERVER, mSMTPServer.String())
		||	archive->AddString( MSG_DOMAIN, mDomainToAnnounce.String())
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddInt16( MSG_AUTH_METHOD, mAuthMethod));
	return ret;
}

/*------------------------------------------------------------------------------*\
	SMTPAddress()
		-	returns the SMTP-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
BNetAddress BmSmtpAccount::SMTPAddress() const {
	BNetAddress addr( mSMTPServer.String(), mPortNr);
	if (addr.InitCheck() == B_OK)
		return addr;
	else
		throw BM_runtime_error("BmSMTPAccount: Could not create SMTPAddress");
}

/*------------------------------------------------------------------------------*\
	NeedsAuthViaPopServer()
		-	
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::NeedsAuthViaPopServer() {
	return mAuthMethod == BM_SMTP_AFTER_POP;
}



/********************************************************************************\
	BmSmtpAccountList
\********************************************************************************/


BmRef< BmSmtpAccountList> BmSmtpAccountList::theInstance( NULL);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmSmtpAccountList* BmSmtpAccountList::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmSmtpAccountList;
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmSmtpAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmSmtpAccountList::BmSmtpAccountList()
	:	inherited( "SmtpAccountList")
{
}

/*------------------------------------------------------------------------------*\
	~BmSmtpAccountList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmSmtpAccountList::~BmSmtpAccountList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BString BmSmtpAccountList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Smtp Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BString("Start of InstantiateItems() for SmtpAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find smtp-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmSmtpAccount* newAcc = new BmSmtpAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BString("SmtpAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InstantiateMailRefs() for SmtpAccountList"));
	mInitCheck = B_OK;
}
