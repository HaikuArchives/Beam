/*
	BmFilter.h

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


#ifndef _BmFilter_h
#define _BmFilter_h

#include <stdexcept>

#include <Archivable.h>

extern "C" {
	#include "sieve_interface.h"
}

#include "BmString.h"
#include "BmDataModel.h"


class BmFilterList;

/*------------------------------------------------------------------------------*\
	BmFilter 
		-	holds information about one filter (a SIEVE-script)
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmFilter : public BmListModelItem {
	typedef BmListModelItem inherited;

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_CONTENT;
	static const char* const MSG_MARK_DEFAULT;
	static const int16 nArchiveVersion;

public:
	BmFilter( const char* name, BmFilterList* model);
	BmFilter( BMessage* archive, BmFilterList* model);
	virtual ~BmFilter();
	
	// native methods:
	bool CompileScript();
	bool Execute( void* msgContext);
	void RegisterCallbacks( sieve_interp_t* interp);
	bool SanityCheck( BmString& complaint, BmString& fieldName);
	BmString ErrorString() const;

	// SIEVE-callbacks:
	static int sieve_parse_error( int lineno, const char *msg, 
											void *interp_context, void *script_context);

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BmString &Content() const	{ return mContent; }
	inline const BmString &Name() const		{ return Key(); }
	inline bool MarkedAsDefault() const		{ return mMarkedAsDefault; }

	inline int LastErrVal() const			{ return mLastErrVal; }
	inline const BmString &LastErr() const	{ return mLastErr; }
	inline const BmString &LastSieveErr() const { return mLastSieveErr; }

	// setters:
	inline void Content( const BmString &s){ mContent = s; TellModelItemUpdated( UPD_ALL); }
	inline void MarkedAsDefault( bool b)	{ mMarkedAsDefault = b;  TellModelItemUpdated( UPD_ALL); }

private:
	BmFilter();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmFilter( const BmFilter&);
	BmFilter operator=( const BmFilter&);

	BmString mContent;
							// the SIEVE-script represented by this filter
	bool mMarkedAsDefault;
							// is this the default filter?
	sieve_script_t* mCompiledScript;
							// the compiled SIEVE-script, ready to be thrown at messages
	int mLastErrVal;
							// last error-value we got
	BmString mLastErr;
							// the last (general) error that occurred
	BmString mLastSieveErr;
							// the last SIEVE-error that occurred
};


/*------------------------------------------------------------------------------*\
	BmFilterList 
		-	holds list of all Filters
\*------------------------------------------------------------------------------*/
class BmFilterList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmFilterList* CreateInboundInstance();
	static BmFilterList* CreateOutboundInstance();
	BmFilterList( const char* name);
	~BmFilterList();
	
	// native methods:
	BmRef<BmFilter> DefaultFilter();
	void SetDefaultFilter( BmString filterName);
	
	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmFilterList> theInboundInstance;
	static BmRef<BmFilterList> theOutboundInstance;

private:
	// Hide copy-constructor and assignment:
	BmFilterList( const BmFilterList&);
	BmFilterList operator=( const BmFilterList&);
	
};

#define TheInboundFilterList BmFilterList::theInboundInstance
#define TheOutboundFilterList BmFilterList::theOutboundInstance

#endif
