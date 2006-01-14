/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmWindow_h
#define _BmWindow_h

#include "BmString.h"

#ifdef B_BEOS_VERSION_DANO
	class BPopUpMenu;
#endif
#include <MWindow.h>

class BmWindow : public MWindow
{
	typedef MWindow inherited;

	static const char* const MSG_FRAME;

public:
	// creator-func, c'tors and d'tor:
	BmWindow( const char* statfileName, BRect frame, const char* title,
				 window_look look, window_feel feel, uint32 flags);
	~BmWindow();

	// native methods:
	virtual status_t ArchiveState( BMessage* archive) const;
	virtual status_t UnarchiveState( BMessage* archive);
	virtual bool ReadStateInfo();
	virtual bool WriteStateInfo();
	virtual void BeginLife()				{}

	// overrides of BWindow-base:
	void Quit();
	void Show();
	void MessageReceived( BMessage*);
	
protected:
	BmString mStatefileName;
	bool mLifeHasBegun;

	// Hide copy-constructor and assignment:
	BmWindow( const BmWindow&);
	BmWindow operator=( const BmWindow&);
};


#endif
