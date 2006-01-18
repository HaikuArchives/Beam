/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPopAccount_h
#define _BmPopAccount_h

#include "BmMailKit.h"

#include "BmRecvAccount.h"

enum {
	BM_JOBWIN_POP	= 'bmea'
		// sent to JobMetaController (or app) in order to 
		// start pop-connection
};

/*------------------------------------------------------------------------------*\
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	extends BmRecvAccount with POP-specific functionality
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmPopAccount : public BmRecvAccount {
	typedef BmRecvAccount inherited;

public:
	BmPopAccount( const char* name, BmRecvAccountList* model);
	BmPopAccount( BMessage* archive, BmRecvAccountList* model);
	virtual ~BmPopAccount();
	
	// overrides of BmRecvAccount base:
	virtual const char* Type() const		{ return nType; }
	virtual int32 JobType() const			{ return BM_JOBWIN_POP; }

	virtual const char* DefaultPort(bool encrypted) const {
		return encrypted ? "995" : "110";
	}

	static const char* const AUTH_POP3;
	static const char* const AUTH_APOP;

	static const char* const nType;
private:
	BmPopAccount();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmPopAccount( const BmPopAccount&);
	BmPopAccount operator=( const BmPopAccount&);
};

#endif
