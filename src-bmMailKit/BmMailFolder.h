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

class BmMailRefList;

typedef map<BString, BmMailFolder*> BmFolderMap;

/*------------------------------------------------------------------------------*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolder : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmMailFolder( entry_ref &eref, ino_t node, BmMailFolder* parent, time_t &modified);
	BmMailFolder( BMessage* archive, BmMailFolder* parent);
	virtual ~BmMailFolder();
	
	// native methods:
	void BumpNewMailCount();
	void BumpNewMailCountForSubfolders();
	bool HasNewMail() const					{ return mNewMailCount>0 || mNewMailCountForSubfolders>0; }
	bool CheckIfModifiedSince();
	void CreateMailRefList();
	void RemoveMailRefList();

	// overrides of archivable base:
	static BArchivable* Instantiate( BMessage* archive);
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const entry_ref& EntryRef() const 	{ return mEntryRef; }
	const entry_ref* EntryRefPtr() const{ return &mEntryRef; }
	const ino_t& Inode() const				{ return mInode; }
	const int NewMailCount() const		{ return mNewMailCount; }
	const int NewMailCountForSubfolders() const		{ return mNewMailCountForSubfolders; }
	BmMailFolder* Parent() 					{ return dynamic_cast<BmMailFolder*>( mParent); }
	BmMailRefList* MailRefList();
	const BString& Name() const			{ return mName; }

	// setters:
	void EntryRef( entry_ref &e) 			{ mEntryRef = e; }

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF = 		"bm:eref";
	static const char* const MSG_INODE = 			"bm:inod";
	static const char* const MSG_NUMCHILDREN = 	"bm:nChl";
	static const char* const MSG_CHILDREN = 		"bm:chld";
	static const char* const MSG_LASTMODIFIED = 	"bm:lmod";

	//	message component definitions for status-msgs:
	static const char* const MSG_NAME = 			"bm:fname";

private:
	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	ino_t mInode;
	time_t mLastModified;

	// the following members will be archived into their own files:
	BmMailRefList* mMailRefList;

	// the following members will NOT be archived at all:
	bool mNeedsCacheUpdate;
	int mNewMailCount;
	int mNewMailCountForSubfolders;
	BString mName;

};

#endif
