/*
	BmSpamFilter.h

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


#ifndef _BmSpamFilter_h
#define _BmSpamFilter_h

#include <Archivable.h>
#include <Autolock.h>

#include "BmFilterAddon.h"
#include "BmFilterAddonPrefs.h"

const int BM_MAX_MATCH_COUNT = 20;

/*------------------------------------------------------------------------------*\
	BmSpamFilter 
		-	implements filtering through SIEVE
\*------------------------------------------------------------------------------*/
class BmSpamFilter : public BmFilterAddon {
	typedef BmFilterAddon inherited;
	
	friend class SieveTest;

public:
	BmSpamFilter( const BmString& name, const BMessage* archive);
	virtual ~BmSpamFilter();
	
	// native methods:
	BLocker* Lock();

	// implementations for abstract BmFilterAddon-methods:
	bool Execute( void* msgContext);
	bool SanityCheck( BmString& complaint, BmString& fieldName);
	status_t Archive( BMessage* archive, bool deep = true) const;
	BmString ErrorString() const;

	// getters:
	inline const BmString &Name() const	{ return mName; }

	// setters:

	// archivable components:
	static const char* const MSG_VERSION;
	static const int16 nArchiveVersion;

protected:
	BmString mName;
							// the name of this filter-implementation
	BmString mLastErr;
	static BLocker* nLock;

private:
	BmSpamFilter();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmSpamFilter( const BmSpamFilter&);
	BmSpamFilter operator=( const BmSpamFilter&);

};



/*------------------------------------------------------------------------------*\
	BmSpamFilterPrefs
		-	
\*------------------------------------------------------------------------------*/

class MButton;

enum {
	BM_SHOW_STATISTICS		= 'bmTa',
};


class BmSpamFilterPrefs : public BmFilterAddonPrefsView {
	typedef BmFilterAddonPrefsView inherited;

public:
	BmSpamFilterPrefs( minimax minmax);
	virtual ~BmSpamFilterPrefs();
	
	// native methods:

	// implementations for abstract base-class methods:
	const char *Kind() const;
	void ShowFilter( BmFilterAddon* addon);
	void Initialize();
	void Activate();

	// BView overrides:
	void MessageReceived( BMessage* msg);

private:

	MButton* mStatisticsButton;

	BmSpamFilter* mCurrFilterAddon;

	// Hide copy-constructor and assignment:
	BmSpamFilterPrefs( const BmSpamFilterPrefs&);
	BmSpamFilterPrefs operator=( const BmSpamFilterPrefs&);
};



#endif
