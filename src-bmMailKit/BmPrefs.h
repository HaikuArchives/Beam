/*
	BmPrefs.h

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


#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <Archivable.h>
#include <Locker.h>
#include <Message.h>
#include "BmString.h"

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPrefs : public BArchivable {
	typedef BArchivable inherited;

	static const char* const PREFS_FILENAME;// = 			"General Settings";

	static const char* const MSG_VERSION;// = 	"bm:version";

	static const int16 nPrefsVersion = 4;
	
public:
	// creator-func, c'tors and d'tor:
	static BmPrefs* CreateInstance();
	BmPrefs( void);
	BmPrefs( BMessage* archive);
	virtual ~BmPrefs();

	// native methods:
	bool Store();
	bool GetBool( const char* name);
	bool GetBool( const char* name, const bool defaultVal);
	int32 GetInt( const char* name);
	int32 GetInt( const char* name, const int32 defaultVal);
	BMessage* GetMsg( const char* name);
	BmString GetString( const char* name);
	BmString GetString( const char* name, const BmString defaultVal);
	void SetBool( const char* name, const bool val);
	void SetInt( const char* name, const int32 val);
	void SetMsg( const char* name, const BMessage* val);
	void SetString( const char* name, const BmString val);
	//
	void ResetToSaved();
	void ResetToDefault();

	BmString GetShortcutFor( const char* shortcutID);
	void SetShortcutFor( const char* name, const BmString val);

	// getters:
	BMessage* ShortcutsMsg()				{ return &mShortcutsMsg; }
	BLocker& Locker()							{ return mLocker; }

	static BmPrefs* theInstance;

private:

	void InitDefaults();
	void SetLoglevels();
	BMessage* GetShortcutDefaults( BMessage* msg=NULL);
	void SetShortcutIfNew( BMessage* msg, const char* name, const BmString val);

	BMessage mPrefsMsg;
	BMessage mDefaultsMsg;
	BMessage mShortcutsMsg;
	BMessage mSavedPrefsMsg;

	BLocker mLocker;

	// Hide copy-constructor and assignment:
	BmPrefs( const BmPrefs&);
	BmPrefs operator=( const BmPrefs&);

};

#define ThePrefs BmPrefs::theInstance

#endif
