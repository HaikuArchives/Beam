/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailQuery_h
#define _BmMailQuery_h

#include <vector>

#include "BmMailKit.h"

#include <Entry.h>
#include <Query.h>

#include "BmString.h"

/*------------------------------------------------------------------------------*\
	BmMailQuery
		-	implements the querying for mails
		-	in general, each BmMailQuery is started as a thread which exits when 
			the moving-operation has ended
\*------------------------------------------------------------------------------*/
struct IMPEXPBMMAILKIT BmMailQuery {
	typedef vector< entry_ref > BmRefVect;
	
	BmMailQuery();
	virtual ~BmMailQuery()					{}

	void SetPredicate( const BmString& predicate);
	void Execute();

	BmRefVect mRefVect;
	BQuery mQuery;
	BmString mPredicate;
};

#endif
