/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailMonitor_h
#define _BmMailMonitor_h

#include "BmMailKit.h"

#include <Locker.h>
#include <Looper.h>

class BmMailFolder;
/*------------------------------------------------------------------------------*\
	BmMailMonitor
		-	class 
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailMonitor : public BLooper {
	typedef BLooper inherited;

public:
	// creator-func and c'tor:
	static BmMailMonitor* CreateInstance();
	BmMailMonitor();
	~BmMailMonitor();

	void CacheRefToFolder( node_ref& nref, const BmString& fKey);
	bool IsIdle(uint32 msecs = 1000);

	// overrides of looper base:
	void MessageReceived( BMessage* msg);
	void Quit();

	static BmMailMonitor* theInstance;

private:

	class BmMailMonitorWorker* mWorker;

	// Hide copy-constructor and assignment:
	BmMailMonitor( const BmMailMonitor&);
	BmMailMonitor operator=( const BmMailMonitor&);
};

#define TheMailMonitor BmMailMonitor::theInstance

#endif
