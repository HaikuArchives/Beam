/*
	BmMailFolder.h
		$Id$
*/

#ifndef _BmMailFolder_h
#define _BmMailFolder_h

#include <StorageKit.h>

#include "BmPrefs.h"

class BmMailFolder;

class BmMailRef;

typedef map<ino_t, BmMailFolder*> BmFolderMap;

/*------------------------------------------------------------------------------*\*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolder : public BArchivable {
	typedef BArchivable inherited;

public:
	// archival-fieldnames:
	static const char* const MSG_ENTRYREF = 		"bm:eref";
	static const char* const MSG_INODE = 			"bm:inod";
	static const char* const MSG_NUMCHILDREN = 	"bm:nChl";
	static const char* const MSG_CHILDREN = 		"bm:chld";
	static const char* const MSG_LASTMODIFIED = 	"bm:lmod";

private:
	typedef map<node_ref, BmMailRef*> MailRefMap;
	
public:
	BmMailFolder( entry_ref &eref, ino_t node, BmMailFolder *parent, time_t &modified);
	BmMailFolder( BMessage *archive, BmMailFolder *parent);
	virtual ~BmMailFolder();
	
	// archival stuff:
	static BArchivable *Instantiate( BMessage *archive);
	virtual status_t Archive( BMessage *archive, bool deep = true) const;

	// getters:
	const entry_ref& EntryRef() const 		{ return mEntryRef; }
	const entry_ref* EntryRefPtr() const	{ return &mEntryRef; }
	const BString Name() const					{ return mEntryRef.name; }
	const ino_t& ID() const						{ return mInode; }
//	const time_t& LastModified() const		{ return mLastModified; }
	const int NewMailCount() const			{ return mNewMailCount; }
	const int NewMailCountForSubfolders() const		{ return mNewMailCountForSubfolders; }

	// setters:
	void EntryRef( entry_ref &e) 				{ mEntryRef = e; }

	// 
	void AddSubFolder( BmMailFolder *child);
	void BumpNewMailCount();
	void BumpNewMailCountForSubfolders();
	bool CheckIfModifiedSince();

private:
	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	BmFolderMap mSubFolderMap;
	ino_t mInode;
	time_t mLastModified;

	// the following members will be archived into their own files:
	MailRefMap mMailRefMap;

	// the following members will NOT be archived at all:
	BmMailFolder *mParent;
	bool mNeedsCacheUpdate;
	int mNewMailCount;
	int mNewMailCountForSubfolders;
};

#endif
