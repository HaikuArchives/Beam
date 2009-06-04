/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefFilterControl_h
#define _BmMailRefFilterControl_h

#include <layout.h>

#include "BmMenuControl.h"

class BmMailRefView;

enum {
	BM_MAILREF_FILTER_CHANGED = 'bmfC'
						// sent whenever the filter has changed
};

class BmMailRefFilterControl : public BmMenuControl
{
	typedef BmMenuControl inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMailRefFilterControl();
	~BmMailRefFilterControl();
	
	// native methods:
	void TeamUpWith(BmMailRefView* mrv)	{ mPartnerMailRefView = mrv; }

	// overrides:
	void MessageReceived(BMessage* msg);

	//	message component definitions for status-msgs:
	static const char* const MSG_TIME_SPAN_LABEL;
	
	static const char* const TIME_SPAN_NONE;

private:
	BmMailRefView* mPartnerMailRefView;

	// Hide copy-constructor and assignment:
	BmMailRefFilterControl(const BmMailRefFilterControl&);
	BmMailRefFilterControl operator=(const BmMailRefFilterControl&);
};


#endif
