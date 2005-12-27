/*
	BmMailMonitor.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


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
	bool IsIdle();

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
