/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Application.h>
#include <ByteOrder.h>
#include <File.h>
#include <Message.h>
#include <MessageRunner.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmIdentity
\********************************************************************************/

const char* const BmIdentity::MSG_NAME = 				"bm:name";
const char* const BmIdentity::MSG_RECV_ACCOUNT = 	"bm:recvacc";
const char* const BmIdentity::MSG_SMTP_ACCOUNT = 	"bm:smtpacc";
const char* const BmIdentity::MSG_REAL_NAME = 		"bm:realname";
const char* const BmIdentity::MSG_MAIL_ADDR = 		"bm:mailaddr";
const char* const BmIdentity::MSG_SIGNATURE_NAME = "bm:signaturename";
const char* const BmIdentity::MSG_MARK_BUCKET = 	"bm:markbucket";
const char* const BmIdentity::MSG_MAIL_ALIASES = "bm:mailaliases";
const char* const BmIdentity::MSG_REPLY_TO = 		"bm:replyto";
const char* const BmIdentity::MSG_SPECIAL_HEADERS = "bm:spechead";
const int16 BmIdentity::nArchiveVersion = 4;

/*------------------------------------------------------------------------------*\
	BmIdentity()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmIdentity::BmIdentity( const char* name, BmIdentityList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mMarkedAsBitBucket( false)
{
}

/*------------------------------------------------------------------------------*\
	BmIdentity( archive)
		-	c'tor
		-	constructs a BmIdentity from a BMessage
\*------------------------------------------------------------------------------*/
BmIdentity::BmIdentity( BMessage* archive, BmIdentityList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mSMTPAccount = FindMsgString( archive, MSG_SMTP_ACCOUNT);
	mRealName = FindMsgString( archive, MSG_REAL_NAME);
	mMailAddr = FindMsgString( archive, MSG_MAIL_ADDR);
	mSignatureName = FindMsgString( archive, MSG_SIGNATURE_NAME);
	mMarkedAsBitBucket = FindMsgBool( archive, MSG_MARK_BUCKET);
	mMailAliases = FindMsgString( archive, MSG_MAIL_ALIASES);
	_SplitMailAliases();
	if (version >= 2) {
		mReplyTo = FindMsgString( archive, MSG_REPLY_TO);
	}
	if (version >= 3) {
		mSpecialHeaders = FindMsgString( archive, MSG_SPECIAL_HEADERS);
	}
	if (version < 4) {
		// with version 4 we renamed popacc to recvacc:
		mRecvAccount = FindMsgString( archive, "bm:popacc");
	} else {
		mRecvAccount = FindMsgString( archive, MSG_RECV_ACCOUNT);
	}
}

/*------------------------------------------------------------------------------*\
	~BmIdentity()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmIdentity::~BmIdentity() {
}

/*------------------------------------------------------------------------------*\
	RecvAcc()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmRecvAccount> BmIdentity::RecvAcc() const {
	BmRef<BmListModelItem> accRef 
		= TheRecvAccountList->FindItemByKey( mRecvAccount);
	return dynamic_cast< BmRecvAccount*>( accRef.Get());
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmIdentity into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmIdentity::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_RECV_ACCOUNT, mRecvAccount.String())
		||	archive->AddString( MSG_SMTP_ACCOUNT, mSMTPAccount.String())
		||	archive->AddString( MSG_REAL_NAME, mRealName.String())
		||	archive->AddString( MSG_MAIL_ADDR, mMailAddr.String())
		||	archive->AddString( MSG_SIGNATURE_NAME, mSignatureName.String())
		||	archive->AddBool( MSG_MARK_BUCKET, mMarkedAsBitBucket)
		||	archive->AddString( MSG_REPLY_TO, mReplyTo.String())
		||	archive->AddString( MSG_SPECIAL_HEADERS, mSpecialHeaders.String())
		||	archive->AddString( MSG_MAIL_ALIASES, mMailAliases.String());
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetFromAddress()
		-	returns the constructed from - address for this identity
\*------------------------------------------------------------------------------*/
BmString BmIdentity::GetFromAddress() const {
	BmRef<BmRecvAccount> recvAcc = RecvAcc();
	if (!recvAcc)
		return "";
	BmString addr( mRealName);
	BmString domainPart = GetDomainName();
	if (domainPart.Length())
		domainPart.Prepend( "@");
	if (addr.Length()) {
		if (mMailAddr.Length())
			addr << " <" << mMailAddr << ">";
		else
			addr << " <" << recvAcc->Username() << domainPart << ">";
	} else {
		if (mMailAddr.Length())
			addr << mMailAddr;
		else
			addr << recvAcc->Username() << domainPart;
	}
	return addr;
}

/*------------------------------------------------------------------------------*\
	GetDomainName()
		-	returns the domain part of this identity's address
\*------------------------------------------------------------------------------*/
BmString BmIdentity::GetDomainName() const {
	BmString domainName;
	int32 atPos = mMailAddr.FindFirst("@");
	if (atPos >= 0) {
		// fetch domain name from our given mail-address:
		domainName.SetTo(mMailAddr.String()+atPos+1);
	} else {
		// fetch domain name from receiving account's server name:
		BmRef<BmRecvAccount> recvAcc = RecvAcc();
		if (recvAcc)
			domainName = recvAcc->GetDomainName();
	}
	return domainName;
}

/*------------------------------------------------------------------------------*\
	HandlesAddrSpec()
		-	determines if the given addrSpec belongs to this identity
\*------------------------------------------------------------------------------*/
bool BmIdentity::HandlesAddrSpec( BmString addrSpec, bool needExactMatch) const {
	BmRef<BmRecvAccount> recvAcc = RecvAcc();
	if (!recvAcc || !addrSpec.Length())
		return false;
	Regexx rx;
	if (addrSpec==GetFromAddress() || addrSpec==mMailAddr)
		return true;
	int32 atPos = addrSpec.FindFirst("@");
	if (atPos != B_ERROR) {
		BmString addrDomain( addrSpec.String()+atPos+1);
		if (addrDomain != GetDomainName())
			return false;						// address is from different domain
		if (addrSpec == recvAcc->Username()+"@"+addrDomain)
			return true;
		addrSpec.Truncate( atPos);
	}
	if (!needExactMatch && mMarkedAsBitBucket)
		return true;
		
	vector<BmString>::const_iterator iter;
	for(iter = mMailAliasesVect.begin(); iter != mMailAliasesVect.end(); ++iter) {
		if  (addrSpec.ICompare(*iter) == 0)
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	_SplitMailAliases()
		-	splits the given comma-/space-separated string into a vector of
			mail aliases
\*------------------------------------------------------------------------------*/
void BmIdentity::_SplitMailAliases()
{
	split("[\\s,]", mMailAliases, mMailAliasesVect);
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmIdentity::SanityCheck( BmString& complaint, BmString& fieldName) const {
	if (!mRecvAccount.Length()) {
		complaint 
			= "Please select a receiving-account to be associated with "
			  "this identity.";
		fieldName = "recvaccount";
		return false;
	}
	if (!mSMTPAccount.Length()) {
		complaint 
			= "Please select a sending-account to be associated with "
			  "this identity.";
		fieldName = "smtpaccount";
		return false;
	}
	return true;
}



/********************************************************************************\
	BmIdentityList
\********************************************************************************/

BmRef< BmIdentityList> BmIdentityList::theInstance( NULL);

const int16 BmIdentityList::nArchiveVersion = 1;

const char* const BmIdentityList::MSG_CURR_IDENTITY = 		"bm:cid";

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmIdentityList* BmIdentityList::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmIdentityList();
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmIdentityList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmIdentityList::BmIdentityList()
	:	inherited( "IdentityList", BM_LogMailTracking) 
	,	mCurrIdentity( NULL)
{
	NeedControllersToContinue( false);
}

/*------------------------------------------------------------------------------*\
	~BmIdentityList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmIdentityList::~BmIdentityList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settins-file for the identity-list
\*------------------------------------------------------------------------------*/
const BmString BmIdentityList::SettingsFileName() {
	return BmString( BeamRoster->SettingsPath()) << "/" << "Identities";
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmIdentityList::ForeignKeyChanged( const BmString& key, 
													 const BmString& oldVal, 
													 const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (key == BmIdentity::MSG_RECV_ACCOUNT) {
			if (ident && ident->RecvAccount() == oldVal)
				ident->RecvAccount( newVal);
		} else if (key == BmIdentity::MSG_SMTP_ACCOUNT) {
			if (ident && ident->SMTPAccount() == oldVal)
				ident->SMTPAccount( newVal);
		} else if (key == BmIdentity::MSG_SIGNATURE_NAME) {
			if (ident && ident->SignatureName() == oldVal)
				ident->SignatureName( newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmIdentity into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmIdentityList::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_CURR_IDENTITY, mCurrIdentity 
																	? mCurrIdentity->Key().String()
																	: "");
	return ret;
}

/*------------------------------------------------------------------------------*\
	InstantiateItem( archive)
		-	instantiates an identity from the given archive
\*------------------------------------------------------------------------------*/
void BmIdentityList::InstantiateItem( BMessage* archive) {
	BmIdentity* newIdent = new BmIdentity( archive, this);
	BM_LOG3( BM_LogMailTracking, 
				BmString("Identity <") << newIdent->Name() << "," 
					<< newIdent->Key() << "> read");
	AddItemToList( newIdent);
	if (!mCurrIdentity)
		mCurrIdentity = newIdent;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	fetches info about last current identity from archive
\*------------------------------------------------------------------------------*/
void BmIdentityList::InstantiateItems( BMessage* archive) {
	mCurrIdentity = NULL;
	inherited::InstantiateItems(archive);
	BmString currIdentName = archive->FindString( MSG_CURR_IDENTITY);
	if (currIdentName.Length()) {
		BmRef<BmListModelItem> identRef = FindItemByKey( currIdentName);
		if (identRef)
			mCurrIdentity = dynamic_cast< BmIdentity*>( identRef.Get());
	}
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets the identites to last saved state
\*------------------------------------------------------------------------------*/
void BmIdentityList::ResetToSaved() {
	BM_LOG2( BM_LogMailTracking, BmString("Start of ResetToSaved() for IdentityList"));
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	Cleanup();
	StartJobInThisThread();
	BM_LOG2( BM_LogMailTracking, BmString("End of ResetToSaved() for IdentityList"));
}

/*------------------------------------------------------------------------------*\
	FindIdentityForRecvAccount( accName)
		-	returns an identity matching the given account
		-	if any identity using the given account is marked as a bit-bucket, this
			identity is returned, otherwise the first identity using the given account
			is returned.
\*------------------------------------------------------------------------------*/
BmRef<BmIdentity> BmIdentityList
::FindIdentityForRecvAccount( const BmString accName) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	// check if we have a bit-bucket identity:
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->MarkedAsBitBucket() && ident->RecvAccount()==accName)
			return ident;
	}
	// return the first address matching the given account:
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->RecvAccount()==accName)
			return ident;
	}
	// nothing found !?!
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindFromAddressForRecvAccount( accName)
		-	returns the from-address corresponding to the given account
\*------------------------------------------------------------------------------*/
BmString BmIdentityList
::FindFromAddressForRecvAccount( const BmString accName) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmRef<BmIdentity> ident = FindIdentityForRecvAccount( accName);
	if (ident)
		return ident->GetFromAddress();
	else
		return "";
}

/*------------------------------------------------------------------------------*\
	FindIdentityForAddress( addrSpec)
		-	determines which identity the given addrSpec belongs to (if any)
\*------------------------------------------------------------------------------*/
BmRef<BmIdentity> BmIdentityList::FindIdentityForAddrSpec( const BmString addrSpec) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	// we first check whether any identity handles the given address (as primary
	// address):
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->HandlesAddrSpec( addrSpec, true))
			return ident;
	}
	// maybe we have a bit-bucket identity/account (catch-all for failed delivery):
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->HandlesAddrSpec( addrSpec, false))
			return ident;
	}
	// nothing found !?!
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindRecvAccountForAddrSpec( addr)
		-	determines to which recv-account the given addrSpec belongs (if any)
\*------------------------------------------------------------------------------*/
BmRef<BmRecvAccount> BmIdentityList
::FindRecvAccountForAddrSpec( const BmString addrSpec) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmRef<BmIdentity> ident = FindIdentityForAddrSpec( addrSpec);
	if (ident)
		return ident->RecvAcc();
	else
		return NULL;
}
