/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailFolder_h
#define _BmMailFolder_h

#include "BmMailKit.h"

#include <map>

#include <Entry.h>
#include "BmString.h"

#include "BmDataModel.h"
#include "BmMailRefList.h"

class BmMailFolder;
class BmMailFolderList;

/*------------------------------------------------------------------------------*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailFolder : public BmListModelItem {
	typedef BmListModelItem inherited;

	static const int16 nArchiveVersion;

	typedef set<BmString> SpecialMailRefSet;

public:
	BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node,
					  BmMailFolder* parent, time_t &modified);
	BmMailFolder( BMessage* archive, BmMailFolderList* model, 
					  BmMailFolder* parent);
	virtual ~BmMailFolder();
	
	// native methods:
	void AddSpecialFlagForMailRef( const BmString& key);
	void RemoveSpecialFlagForMailRef( const BmString& key);
	void BumpMailCount( int32 offset=1);
	void BumpSpecialMailCountForSubfolders( int32 offset=1);
	bool HasSpecialMail() const			{ return mSpecialMailRefSet.size()>0 
															|| mSpecialMailCountForSubfolders>0; }
	bool HasSpecialMailInSubfolders() const	
													{ return mSpecialMailCountForSubfolders>0; }
	bool CheckIfModifiedSinceLastTime();
	bool CheckIfModifiedSince( time_t when, time_t* storeNewModTime=NULL);
	void RecreateCache();
	void AddMailRef( entry_ref& eref, struct stat& st);
	void RemoveMailRef( const node_ref& nref);
	void UpdateMailRef( const node_ref& nref);
	void UpdateName( const entry_ref &eref);
	void CreateSubFolder( BmString name);
	void Rename( BmString newName);
	void MoveToTrash();
	bool IsOutbound();

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
	inline const int32 SpecialMailCount() const
													{ return mSpecialMailRefSet.size(); }
	inline const int32 SpecialMailCountForSubfolders() const
													{ return mSpecialMailCountForSubfolders; }
	inline const int32 MailCount() const	
													{ return mMailCount; }
	inline const time_t LastModified() const
													{ return mLastModified; }
	BmRef<BmMailRefList> MailRefList();
	inline const BmString& Name() const	{ return mName; }
	inline const BmString& SelectedRefKey() const
													{ return mSelectedRefKey; }
	inline bool RefListStateInfoConnectedToParent() const 
													{ return mRefListStateInfoConnectedToParent; }

	// setters:
	inline void EntryRef( const entry_ref &e)
												 	{ mEntryRef = e; mName = e.name; }
	inline void SelectedRefKey( const BmString& key)
													{ mSelectedRefKey = key; }
	void MailCount( int32 count);
	inline void RefListStateInfoConnectedToParent( bool b)
													{ mRefListStateInfoConnectedToParent = b; }

	static bool IsSystemFolderSubPath( const BmString& subPath);

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF;
	static const char* const MSG_INODE;
	static const char* const MSG_LASTMODIFIED;
	static const char* const MSG_MAILCOUNT;
	static const char* const MSG_SELECTED_KEY;
	static const char* const MSG_STATEINFO_CONNECTED;

	//	message component definitions for status-msgs:
	static const char* const MSG_NAME;

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_SPECIAL_COUNT;
	static const BmUpdFlags UPD_TOTAL_COUNT;
	static const BmUpdFlags UPD_HAVE_SPECIAL_STATUS;

	static const char* DRAFT_FOLDER_NAME;
	static const char* IN_FOLDER_NAME;
	static const char* OUT_FOLDER_NAME;
	static const char* QUARANTINE_FOLDER_NAME;
	static const char* SPAM_FOLDER_NAME;

protected:
	void StartNodeMonitor();
	void StopNodeMonitor();

	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	node_ref mNodeRef;
	time_t mLastModified;
	int32 mMailCount;
	BmString mSelectedRefKey;
	bool mRefListStateInfoConnectedToParent;

	// the following members will be archived into their own files:
	BmRef< BmMailRefList> mMailRefList;

	// the following members will NOT be archived at all:
	SpecialMailRefSet mSpecialMailRefSet;
	int32 mSpecialMailCountForSubfolders;
	BmString mName;

	static BLocker nRefListLocker;

	// Hide copy-constructor and assignment:
	BmMailFolder( const BmMailFolder&);
	BmMailFolder operator=( const BmMailFolder&);
};

#endif
