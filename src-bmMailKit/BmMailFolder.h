/*
	BmMailFolder.h
		$Id$
*/

#ifndef _BmMailFolder_h
#define _BmMailFolder_h

#include <map>

#include <Archivable.h>
#include <Entry.h>
#include <Node.h>
#include <String.h>

#include "BmDataModel.h"

class BmMailFolder;
class BmMailFolderList;

class BmMailRefList;

typedef map<BString, BmMailFolder*> BmFolderMap;

/*------------------------------------------------------------------------------*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolder : public BmListModelItem {
	typedef BmListModelItem inherited;

	static const int16 nArchiveVersion = 1;

public:
	BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node,
					  BmMailFolder* parent, time_t &modified);
	BmMailFolder( BMessage* archive, BmMailFolderList* model, BmMailFolder* parent);
	virtual ~BmMailFolder();
	
	// native methods:
	void BumpNewMailCount( int32 offset=1);
	void BumpNewMailCountForSubfolders( int32 offset=1);
	bool HasNewMail() const					{ return mNewMailCount>0 || mNewMailCountForSubfolders>0; }
	bool CheckIfModifiedSinceLastTime();
	bool CheckIfModifiedSince( time_t when, time_t* storeNewModTime=NULL);
	void CreateMailRefList();
	void RemoveMailRefList();
	void RecreateCache();
	void AddMailRef( entry_ref& eref, struct stat& st);
	bool HasMailRef( BString key);
	void RemoveMailRef( ino_t node);
	void CreateSubFolder( BString name);
	void Rename( BString newName);
	void MoveToTrash();

	// overrides of archivable base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const entry_ref& EntryRef() const 		{ return mEntryRef; }
	inline const entry_ref* EntryRefPtr() const	{ return &mEntryRef; }
	inline const ino_t& Inode() const				{ return mInode; }
	inline const int NewMailCount() const			{ return mNewMailCount; }
	inline const int NewMailCountForSubfolders() const		{ return mNewMailCountForSubfolders; }
	inline const time_t LastModified() const		{ return mLastModified; }
	BmRef<BmMailRefList> MailRefList();
	inline const BString& Name() const				{ return mName; }

	// setters:
	inline void EntryRef( entry_ref &e) 			{ mEntryRef = e; mName = e.name; }

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF = 		"bm:eref";
	static const char* const MSG_INODE = 			"bm:inod";
	static const char* const MSG_LASTMODIFIED = 	"bm:lmod";

	//	message component definitions for status-msgs:
	static const char* const MSG_NAME = 			"bm:fname";

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_NAME	= 1<<1;

private:
	void StartNodeMonitor();
	void StopNodeMonitor();

	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	ino_t mInode;
	time_t mLastModified;

	// the following members will be archived into their own files:
	BmRef< BmMailRefList> mMailRefList;

	// the following members will NOT be archived at all:
	int mNewMailCount;
	int mNewMailCountForSubfolders;
	BString mName;

	// Hide copy-constructor and assignment:
	BmMailFolder( const BmMailFolder&);
	BmMailFolder operator=( const BmMailFolder&);
};

#endif
