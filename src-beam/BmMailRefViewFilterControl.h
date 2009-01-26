/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefViewFilterControl_h
#define _BmMailRefViewFilterControl_h

#include <layout.h>
#include <HGroup.h>

class BMessageRunner;
class BmMailRefView;
class BmMenuControl;
class BmTextControl;

enum {
	BM_MAILREF_VIEW_FILTER_CHANGED = 'bmfD'
						// sent whenever the filter has changed
};

class BmMailRefViewFilterControl : public HGroup
{
	typedef HGroup inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMailRefViewFilterControl();
	~BmMailRefViewFilterControl();
	
	// native methods:
	void TeamUpWith(BmMailRefView* mrv)	{ mPartnerMailRefView = mrv; }

	// overrides:
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

	//	message component definitions for status-msgs:
	static const char* const MSG_FILTER_KIND;
	static const char* const MSG_FILTER_CONTENT;
	
	static const char* const FILTER_SUBJECT_OR_ADDRESS;
	static const char* const FILTER_MAILTEXT;
private:
	BmMenuControl* mMenuControl;
	BmTextControl* mTextControl;
	
	BMessageRunner* mMsgRunner;
	BmMailRefView* mPartnerMailRefView;

	// Hide copy-constructor and assignment:
	BmMailRefViewFilterControl( const BmMailRefViewFilterControl&);
	BmMailRefViewFilterControl operator=( const BmMailRefViewFilterControl&);
};


#endif