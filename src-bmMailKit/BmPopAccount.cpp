/*
	BmPopAccount.cpp

		$Id$
*/

#include <ByteOrder.h>
#include <File.h>
#include <Message.h>

#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmMsgTypes.h"
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
	const char* uid;
	for( int32 i=0; archive->FindString( MSG_UID, i, &uid) == B_OK; ++i) {
		mUIDs.push_back( uid);
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPopAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPopAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
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
	int32 count = mUIDs.size();
	for( int i=0; ret==B_OK && i<count; ++i) {
		ret = archive->AddString( MSG_UID, mUIDs[i].String());
	}
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

/*------------------------------------------------------------------------------*\
	IsUIDDownloaded()
		-	
\*------------------------------------------------------------------------------*/
bool BmPopAccount::IsUIDDownloaded( BString uid) {
	uid << " ";									// append a space to avoid matching only in parts
	int32 uidLen = uid.Length();
	int32 count = mUIDs.size();
	for( int32 i=count-1; i>=0; --i) {
		if (!mUIDs[i].Compare( uid, uidLen))
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	MarkUIDAsDownloaded()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccount::MarkUIDAsDownloaded( BString uid) {
	mUIDs.push_back( uid << " " << system_time());
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
	// ugly HACK to ensure that add-messages are being sent during Job:
	mFrozenCount = -100;						
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

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BString("Start of InstantiateItems() for PopAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find pop-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmPopAccount* newAcc = new BmPopAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BString("PopAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InstantiateMailRefs() for PopAccountList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	CheckMail()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMail() {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (acc->CheckMail()) {
			CheckMailFor( acc->Name());
		}
	}
}

/*------------------------------------------------------------------------------*\
	CheckMailFor()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMailFor( BString accName) {
	BMessage archive(BM_JOBWIN_FETCHPOP);
	archive.AddString( BmJobStatusWin::MSG_JOB_NAME, accName.String());
	TheJobStatusWin->PostMessage( &archive);
}
