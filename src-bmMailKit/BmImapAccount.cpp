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
#include "BmMailFolder.h"
#include "BmImapAccount.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmImapAccount
\********************************************************************************/

const char* const BmImapAccount::AUTH_LOGIN = "LOGIN";

const char* const BmImapAccount::nType = "IMAP";
/*------------------------------------------------------------------------------*\
	BmImapAccount()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmImapAccount::BmImapAccount( const char* name, BmRecvAccountList* model) 
	:	inherited( name, model)
{
	mPortNr = 143;
	mPortNrString = "143";
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	BmImapAccount( archive)
		-	c'tor
		-	constructs a BmImapAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmImapAccount::BmImapAccount( BMessage* archive, BmRecvAccountList* model) 
	:	inherited( archive, model)
{
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	~BmImapAccount()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmImapAccount::~BmImapAccount() {
}

/*------------------------------------------------------------------------------*\
	GetSupportedAuthTypes()
		-
\*------------------------------------------------------------------------------*/
void BmImapAccount::GetSupportedAuthTypes(vector<BmString>& outList) const
{
	outList.push_back(AUTH_AUTO);
	outList.push_back(AUTH_CRAM_MD5);
	outList.push_back(AUTH_DIGEST_MD5);
	outList.push_back(AUTH_LOGIN);
	outList.push_back(AUTH_NONE);
}
