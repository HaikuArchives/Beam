/*
	BmMailFolder.h
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


#ifndef _BmMailFolder_h
#define _BmMailFolder_h

#include <map>

#include <Entry.h>
#include "BmString.h"

#include "BmDataModel.h"

class BmMailFolder;
class BmMailFolderList;

class BmMailRefList;

typedef map<BmString, BmMailFolder*> BmFolderMap;

/*------------------------------------------------------------------------------*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolder : public BmListModelItem {
	typedef BmListModelItem inherited;

	static const int16 nArchiveVersion = 2;

public:
	BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node,
					  BmMailFolder* parent, time_t &modified);
	BmMailFolder( BMessage* archive, BmMailFolderList* model, 
					  BmMailFolder* parent);
	virtual ~BmMailFolder();
	
	// native methods:
	void BumpNewMailCount( int32 offset=1);
	void BumpNewMailCountForSubfolders( int32 offset=1);
	bool HasNewMail() const					{ return mNewMailCount>0 
															|| mNewMailCountForSubfolders>0; }
	bool CheckIfModifiedSinceLastTime();
	bool CheckIfModifiedSince( time_t when, time_t* storeNewModTime=NULL);
	void CreateMailRefList();
	void RemoveMailRefList();
	void RecreateCache();
	void AddMailRef( entry_ref& eref, struct stat& st);
	BmRef<BmListModelItem> FindMailRefByKey( const BmString& key);
	void RemoveMailRef( const node_ref& nref);
	void CleanupForMailRefList( BmMailRefList* refList);
	void UpdateName( const entry_ref &eref);
	void CreateSubFolder( BmString name);
	void Rename( BmString newName);
	void MoveToTrash();

	// overrides of listmodel-item base:
	const BmString& DisplayKey() const	{ return mName; }

	// overrides of archivable base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const entry_ref& EntryRef() const
											 		{ return mEntryRef; }
	inline const entry_ref* EntryRefPtr() const
													{ return &mEntryRef; }
	inline const node_ref& NodeRef() const
													{ return mNodeRef; }
	inline const int32 NewMailCount() const
													{ return mNewMailCount; }
	inline const int32 NewMailCountForSubfolders() const
													{ return mNewMailCountForSubfolders; }
	inline const int32 MailCount() const	
													{ return mMailCount; }
	inline const time_t LastModified() const
													{ return mLastModified; }
	BmRef<BmMailRefList> MailRefList();
	inline const BmString& Name() const	{ return mName; }

	// setters:
	inline void EntryRef( const entry_ref &e)
												 	{ mEntryRef = e; mName = e.name; }
	inline void MailCount( int32 i)	 	{ mMailCount = i; 
													  TellModelItemUpdated( UPD_ALL); }

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF;
	static const char* const MSG_INODE;
	static const char* const MSG_LASTMODIFIED;
	static const char* const MSG_MAILCOUNT;

	//	message component definitions for status-msgs:
	static const char* const MSG_NAME;

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_NEW_STATUS;

private:
	void StartNodeMonitor();
	void StopNodeMonitor();

	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	node_ref mNodeRef;
	time_t mLastModified;
	int32 mMailCount;

	// the following members will be archived into their own files:
	BmRef< BmMailRefList> mMailRefList;
	static BLocker nRefListLocker;

	// the following members will NOT be archived at all:
	int32 mNewMailCount;
	int32 mNewMailCountForSubfolders;
	BmString mName;

	// Hide copy-constructor and assignment:
	BmMailFolder( const BmMailFolder&);
	BmMailFolder operator=( const BmMailFolder&);
};

#endif
