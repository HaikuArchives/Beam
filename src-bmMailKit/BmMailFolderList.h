/*
	BmMailFolderList.h
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


#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include <sys/stat.h>

#include <Locker.h>
#include <Looper.h>
#include <Query.h>

#include "BmDataModel.h"
#include "BmMailFolder.h"

/*------------------------------------------------------------------------------*\*\
	BmMailMonitor
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailMonitor : public BLooper {
	typedef BLooper inherited;

public:
	// creator-func and c'tor:
	static BmMailMonitor* CreateInstance();
	BmMailMonitor();

	//	native methods:
	void HandleMailMonitorMsg( BMessage* msg);
	void HandleQueryUpdateMsg( BMessage* msg);

	// overrides of looper base:
	void MessageReceived( BMessage* msg);

	static BmMailMonitor* theInstance;
	
private:
	BmMailFolder* parent;
	ino_t lastParentInode;
	BmRef<BmListModelItem> lastParentRef;
	BmMailFolder* oldParent;
	ino_t lastOldParentInode;
	BmRef<BmListModelItem> lastOldParentRef;
	int32 counter;

	// Hide copy-constructor and assignment:
	BmMailMonitor( const BmMailMonitor&);
	BmMailMonitor operator=( const BmMailMonitor&);
};

/*------------------------------------------------------------------------------*\*\
	BmMailFolderList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolderList : public BmListModel {
	typedef BmListModel inherited;
	typedef map< ino_t, BmMailFolder*> BmNewNodeMap;

	friend class BmMailFolder;
	friend BmMailMonitor;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME = 	"bm:mboxmtime";
	static const int16 nArchiveVersion = 1;

public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderList* CreateInstance();
	BmMailFolderList();
	virtual ~BmMailFolderList();
	
	// native methods:
	BmMailFolder* AddNewFlag( ino_t pnode, ino_t node);
	void RemoveNewFlag( ino_t pnode, ino_t node);
	void SetFolderForNodeFlaggedNew( ino_t node, BmMailFolder* folder);
	BmMailFolder* GetFolderForNodeFlaggedNew( ino_t node);
	bool NodeIsFlaggedNew( ino_t node);
	void QueryForNewMails();
	
	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BmString SettingsFileName();

	// setters:
	void MailboxPathHasChanged( bool b) { mMailboxPathHasChanged = b; }

	static BmRef< BmMailFolderList> theInstance;
	
private:
	// native methods:
	BmMailFolder* AddMailFolder( entry_ref& eref, int64 node, BmMailFolder* parent, 
										  time_t mtime);
	void InitializeItems();
	int doInitializeMailFolders( BmMailFolder* folder, int level);
	void InstantiateItems( BMessage* archive);
	void doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive, int level);

	// overrides of listmodel base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// the following members will be archived as part of BmFolderList:
	BmRef<BmMailFolder> mTopFolder;
	
	// the following members will NOT be archived at all:
	BQuery mNewMailQuery;
	BmNewNodeMap mNewMailNodeMap;
	bool mMailboxPathHasChanged;

	// Hide copy-constructor and assignment:
	BmMailFolderList( const BmMailFolderList&);
	BmMailFolderList operator=( const BmMailFolderList&);
};

#define TheMailFolderList BmMailFolderList::theInstance

#define TheMailMonitor BmMailMonitor::theInstance

#endif
