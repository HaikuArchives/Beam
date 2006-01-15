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
#include "BmPopAccount.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPopAccount
\********************************************************************************/

const char* const BmPopAccount::AUTH_POP3 = "POP3";
const char* const BmPopAccount::AUTH_APOP = "APOP";

const char* const BmPopAccount::nType = "POP";
/*------------------------------------------------------------------------------*\
	BmPopAccount()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( const char* name, BmRecvAccountList* model) 
	:	inherited( name, model)
{
	mPortNr = 110;
	mPortNrString = "110";
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	BmPopAccount( archive)
		-	c'tor
		-	constructs a BmPopAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( BMessage* archive, BmRecvAccountList* model) 
	:	inherited( archive, model)
{
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	~BmPopAccount()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmPopAccount::~BmPopAccount() {
}

