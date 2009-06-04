/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefViewFilterControl_h
#define _BmMailRefViewFilterControl_h

#include <MessageFilter.h>

#include <layout.h>
#include <HGroup.h>

#include "BmController.h"
#include "BmMailRefViewFilterJob.h"

class BMessageRunner;

class BmMailRefView;
class BmMenuControl;
class BmTextControl;

enum {
	BM_MAILREF_VIEW_FILTER_CHANGED = 'bmfD'
						// sent whenever the filter has changed
};

class BmMailRefViewFilterControl : public HGroup, public BmJobController
{
	typedef HGroup inheritedView;
	typedef BmJobController inheritedController;

public:
	// creator-func, c'tors and d'tor:
	BmMailRefViewFilterControl();
	~BmMailRefViewFilterControl();
	
	// native methods:
	void TeamUpWith(BmMailRefView* mrv)	{ mPartnerMailRefView = mrv; }
	void ClearFilter();

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

	// overrides of view base:
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);
	void KeyDown(const char *bytes, int32 numBytes);
	void MakeFocus(bool focus);

	//	message component definitions for status-msgs:
	static const char* const MSG_FILTER_KIND;
	
	static filter_result MessageFilterHook(BMessage* msg, BHandler** handler,
														BMessageFilter* messageFilter);
	
protected:
	// overrides of controller base:
	void JobIsDone(bool completed);

private:
	BmMenuControl* mMenuControl;
	BmTextControl* mTextControl;
	
	BMessageRunner* mMsgRunner;
	BmMailRefView* mPartnerMailRefView;
	
	BmString mLastKind;
	BmString mLastContent;
	
	// Hide copy-constructor and assignment:
	BmMailRefViewFilterControl( const BmMailRefViewFilterControl&);
	BmMailRefViewFilterControl operator=( const BmMailRefViewFilterControl&);
};


#endif
