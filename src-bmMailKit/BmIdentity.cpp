/*
	BmIdentity.cpp

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <Application.h>
#include <ByteOrder.h>
#include <File.h>
#include <Message.h>
#include <MessageRunner.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmMsgTypes.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmIdentity
\********************************************************************************/

const char* const BmIdentity::MSG_NAME = 				"bm:name";
const char* const BmIdentity::MSG_POP_ACCOUNT = 	"bm:popacc";
const char* const BmIdentity::MSG_SMTP_ACCOUNT = 	"bm:smtpacc";
const char* const BmIdentity::MSG_REAL_NAME = 		"bm:realname";
const char* const BmIdentity::MSG_MAIL_ADDR = 		"bm:mailaddr";
const char* const BmIdentity::MSG_SIGNATURE_NAME = "bm:signaturename";
const char* const BmIdentity::MSG_MARK_BUCKET = 	"bm:markbucket";
const char* const BmIdentity::MSG_MAIL_ALIASES = "bm:mailaliases";
const int16 BmIdentity::nArchiveVersion = 1;

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
	mPOPAccount = FindMsgString( archive, MSG_POP_ACCOUNT);
	mSMTPAccount = FindMsgString( archive, MSG_SMTP_ACCOUNT);
	mRealName = FindMsgString( archive, MSG_REAL_NAME);
	mMailAddr = FindMsgString( archive, MSG_MAIL_ADDR);
	mSignatureName = FindMsgString( archive, MSG_SIGNATURE_NAME);
	mMarkedAsBitBucket = FindMsgBool( archive, MSG_MARK_BUCKET);
	mMailAliases = FindMsgString( archive, MSG_MAIL_ALIASES);
}

/*------------------------------------------------------------------------------*\
	~BmIdentity()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmIdentity::~BmIdentity() {
}

/*------------------------------------------------------------------------------*\
	UpdatePopAccount()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmIdentity::PopAcc() const {
	BmRef<BmListModelItem> accRef = ThePopAccountList->FindItemByKey( mPOPAccount);
	return dynamic_cast< BmPopAccount*>( accRef.Get());
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmIdentity into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmIdentity::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_POP_ACCOUNT, mPOPAccount.String())
		||	archive->AddString( MSG_SMTP_ACCOUNT, mSMTPAccount.String())
		||	archive->AddString( MSG_REAL_NAME, mRealName.String())
		||	archive->AddString( MSG_MAIL_ADDR, mMailAddr.String())
		||	archive->AddString( MSG_SIGNATURE_NAME, mSignatureName.String())
		||	archive->AddBool( MSG_MARK_BUCKET, mMarkedAsBitBucket)
		||	archive->AddString( MSG_MAIL_ALIASES, mMailAliases.String());
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetFromAddress()
		-	returns the constructed from - address for this identity
\*------------------------------------------------------------------------------*/
BmString BmIdentity::GetFromAddress() const {
	BmRef<BmPopAccount> popAcc = PopAcc();
	if (!popAcc)
		return "";
	BmString addr( mRealName);
	BmString domainPart = popAcc->GetDomainName();
	if (domainPart.Length())
		domainPart.Prepend( "@");
	if (addr.Length()) {
		if (mMailAddr.Length())
			addr << " <" << mMailAddr << ">";
		else
			addr << " <" << popAcc->Username() << domainPart << ">";
	} else {
		if (mMailAddr.Length())
			addr << mMailAddr;
		else
			addr << popAcc->Username() << domainPart;
	}
	return addr;
}

/*------------------------------------------------------------------------------*\
	HandlesAddrSpec()
		-	determines if the given addrSpec belongs to this identity
\*------------------------------------------------------------------------------*/
bool BmIdentity::HandlesAddrSpec( BmString addrSpec, bool needExactMatch) const {
	BmRef<BmPopAccount> popAcc = PopAcc();
	if (!popAcc)
		return "";
	Regexx rx;
	if (addrSpec==GetFromAddress() || addrSpec==mMailAddr || addrSpec==popAcc->Username())
		return true;
	int32 atPos = addrSpec.FindFirst("@");
	if (atPos != B_ERROR) {
		BmString addrDomain( addrSpec.String()+atPos+1);
		if (addrDomain != popAcc->GetDomainName())
			return false;						// address is from different domain
		if (addrSpec == popAcc->Username()+"@"+addrDomain)
			return true;
		addrSpec.Truncate( atPos);
	}
	if (!needExactMatch && mMarkedAsBitBucket)
		return true;
	BmString regex = BmString("\\b") + addrSpec + "\\b";
	return rx.exec( mMailAliases, regex) > 0;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmIdentity::SanityCheck( BmString& complaint, BmString& fieldName) const {
	if (!mPOPAccount.Length()) {
		complaint = "Please select a receiving-account to be associated with this identity.";
		fieldName = "popaccount";
		return false;
	}
	if (!mSMTPAccount.Length()) {
		complaint = "Please select a sending-account to be associated with this identity.";
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
	:	inherited( "IdentityList") 
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
		-	returns the name of the settins-file for the POP3-accounts-list
\*------------------------------------------------------------------------------*/
const BmString BmIdentityList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/" << "Identities";
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmIdentityList::ForeignKeyChanged( const BmString& key, 
													 const BmString& oldVal, 
													 const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (key == BmIdentity::MSG_POP_ACCOUNT) {
			if (ident && ident->POPAccount() == oldVal)
				ident->POPAccount( newVal);
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
	InstantiateItems( archive)
		-	initializes the identities info from the given message
\*------------------------------------------------------------------------------*/
void BmIdentityList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BmString("Start of InstantiateItems() for IdentityList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find identity nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmIdentity* newIdent = new BmIdentity( &msg, this);
		BM_LOG3( BM_LogMailTracking, BmString("Identity <") << newIdent->Name() << "," << newIdent->Key() << "> read");
		AddItemToList( newIdent);
		if (i==0)
			mCurrIdentity = newIdent;
	}
	BmString currIdentName = archive->FindString( MSG_CURR_IDENTITY);
	if (currIdentName.Length()) {
		BmRef<BmListModelItem> identRef = FindItemByKey( currIdentName);
		if (identRef)
			mCurrIdentity = dynamic_cast< BmIdentity*>( identRef.Get());
	}
	BM_LOG2( BM_LogMailTracking, BmString("End of InstantiateItems() for IdentityList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets the identites to last saved state
\*------------------------------------------------------------------------------*/
void BmIdentityList::ResetToSaved() {
	BM_LOG2( BM_LogMailTracking, BmString("Start of ResetToSaved() for IdentityList"));
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	Cleanup();
	StartJobInThisThread();
	BM_LOG2( BM_LogMailTracking, BmString("End of ResetToSaved() for IdentityList"));
}

/*------------------------------------------------------------------------------*\
	FindIdentityForPopAccount( accName)
		-	returns an identity matching the given account
		-	if any identity using the given account is marked as a bit-bucket, this
			identity is returned, otherwise the first identity using the given account
			is returned.
\*------------------------------------------------------------------------------*/
BmRef<BmIdentity> BmIdentityList::FindIdentityForPopAccount( const BmString accName) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	// check if we have a bit-bucket identity:
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->MarkedAsBitBucket() && ident->POPAccount()==accName)
			return ident;
	}
	// return the first address matching the given account:
	for( iter = begin(); iter != end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		if (ident->POPAccount()==accName)
			return ident;
	}
	// nothing found !?!
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindFromAddressForPopAccount( accName)
		-	returns the from-address corresponding to the given account
\*------------------------------------------------------------------------------*/
BmString BmIdentityList::FindFromAddressForPopAccount( const BmString accName) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmRef<BmIdentity> ident = FindIdentityForPopAccount( accName);
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
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
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
	FindPopAccountForAddrSpec( addr)
		-	determines to which pop-account the given addrSpec belongs (if any)
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmIdentityList::FindPopAccountForAddrSpec( const BmString addrSpec) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmRef<BmIdentity> ident = FindIdentityForAddrSpec( addrSpec);
	if (ident)
		return ident->PopAcc();
	else
		return NULL;
}
