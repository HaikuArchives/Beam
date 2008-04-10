/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmImapAccount_h
#define _BmImapAccount_h

#include "BmMailKit.h"

#include "BmRecvAccount.h"

enum {
	BM_JOBWIN_IMAP	= 'bmei'
		// sent to JobMetaController (or app) in order to 
		// start pop-connection
};

/*------------------------------------------------------------------------------*\
	BmImapAccount 
		-	holds information about one specific IMAP-account
		- 	extends BmRecvAccount with IMAP-specific functionality
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmImapAccount : public BmRecvAccount {
	typedef BmRecvAccount inherited;

public:
	BmImapAccount( const char* name, BmRecvAccountList* model);
	BmImapAccount( BMessage* archive, BmRecvAccountList* model);
	virtual ~BmImapAccount();
	
	// overrides of BmRecvAccount base:
	virtual const char* Type() const		{ return nType; }
	virtual int32 JobType() const			{ return BM_JOBWIN_IMAP; }

	virtual const char* DefaultPort(bool encrypted) const {
		return encrypted ? "993" : "143";
	}

	virtual void GetSupportedAuthTypes(vector<BmString>& outList) const;

	static const char* const AUTH_LOGIN;

	static const char* const nType;
private:
	BmImapAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmImapAccount( const BmImapAccount&);
	BmImapAccount operator=( const BmImapAccount&);
};

#endif
