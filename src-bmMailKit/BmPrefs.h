/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include "BmMailKit.h"

#include <Archivable.h>
#include <Locker.h>
#include <Message.h>
#include <Node.h>
#include <Volume.h>
#include "BmString.h"

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmPrefs : public BArchivable {
	typedef BArchivable inherited;

	static const char* const PREFS_FILENAME;

	static const char* const MSG_VERSION;

	static const int16 nPrefsVersion;
	
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

	const char* GetLogLevelFor( uint32 terrain);
	void SetLogLevelForTo( uint32 terrain, BmString level);

	// getters:
	BMessage* ShortcutsMsg()				{ return &mShortcutsMsg; }
	BLocker& Locker()							{ return mLocker; }

	static BmPrefs* theInstance;

	void SetupMailboxVolume();
	BVolume MailboxVolume;
	BmString TrashPath;

	// log-levels as string (for prefs):
	static const char* const LOG_LVL_0;
	static const char* const LOG_LVL_1;
	static const char* const LOG_LVL_2;
	static const char* const LOG_LVL_3;

	static const BmString nListSeparator;
	static const BmString nDefaultIconset;

private:

	void SetLoglevels();
	static void InitDefaults(BMessage& defaultsMsg);
	static BMessage* GetShortcutDefaults( BMessage* msg=NULL);
	static void SetShortcutIfNew( BMessage* msg, const char* name, const BmString val);

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
