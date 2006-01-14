/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailFilter_h
#define _BmMailFilter_h

#include "BmMailKit.h"

#include <vector>

#include <Message.h>

#include "BmMailRef.h"
#include "BmUtil.h"

class BmFilter;
class BmMsgContext;

/*------------------------------------------------------------------------------*\
	BmMailFilter
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailFilter : public BmJobModel {
	typedef BmJobModel inherited;

	typedef vector< BmRef< BmMail> > BmMailVect;
	typedef vector< const char**> BmHeaderVect;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_FILTER;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;
	static const char* const MSG_REFS;

	BmMailFilter( const BmString& name, BmFilter* filter, 
					  bool executeInMem = false, bool needControllers = true);
	virtual ~BmMailFilter();

	// native methods:
	void SetMailRefVect( BmMailRefVect* refVect);
	void AddMail( BmMail* mail);
	void ManageHeaderVect( const char**header);

	// overrides of BmJobModel base:
	bool StartJob();
	bool ShouldContinue();

	// getters:
	inline BmString Name() const			{ return ModelName(); }

private:
	void Execute( BmMail* mail);
	bool ExecuteFilter( BmMail* mail, BmFilter* filter,
							  BmMsgContext* msgContext);
	void UpdateStatus( const float delta, const char* filename, 
							 const char* currentCount);

	BmRef<BmFilter> mFilter;
							// the actual SIEVE-filter
	BmMailRefVect* mMailRefs;
							// the mail-refs we shall be filtering
	BmMailVect mMails;
							// the mails we shall be filtering
	bool mExecuteInMem;
							// indicates whether the mail shall be stored
							// after the filtering process (false) or not (true).

	// Hide copy-constructor and assignment:
	BmMailFilter( const BmMailFilter&);
	BmMailFilter operator=( const BmMailFilter&);
};

#endif
