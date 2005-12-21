/*
	BmStorageUtil.cpp
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


#include <set>

#include <errno.h> 

#include <Directory.h> 
#include <Messenger.h> 
#include <Message.h> 
#include <NodeMonitor.h> 
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h> 
#include <fs_index.h> 
#include <NodeInfo.h> 
#include <Path.h> 

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"

// -----------------------------------------------------------------------------
BmString BM_REFKEY( const node_ref& nref) {
	return BmString() << nref.node;
}

/*------------------------------------------------------------------------------*\
	WatchNode()
		-	tries to add a node-watcher, if limit is reached, we bump it
		-	Ripped from OpenTracker
\*------------------------------------------------------------------------------*/
extern "C" int _kset_mon_limit_(int num);

status_t WatchNode( const node_ref *node, uint32 flags, BHandler *handler) {
	static int32 gNodeMonitorCount = 4096;
	static const int32 kNodeMonitorBumpValue = 1024;

	status_t result = watch_node( node, flags, BMessenger(handler));

	if (result == B_OK || result != ENOMEM)
		// need to make sure this uses the same error value as
		// the node monitor code
		return result;

	gNodeMonitorCount += kNodeMonitorBumpValue;
	BM_LOG2( BM_LogMailTracking, 
				BmString("Failed to add monitor, trying to bump limit to ")
					<< gNodeMonitorCount << " nodes.");
	result = _kset_mon_limit_(gNodeMonitorCount);
	if (result != B_OK) {
		BM_LOGERR( BmString("Failed to allocate more node monitors, error: ")
						<< strerror(result));
		return result;
	}

	// try again, this time with more node monitors
	return watch_node(node, flags, BMessenger(handler));
}

/*------------------------------------------------------------------------------*\
	MoveToTrash( refs, count)
		-	moves the files specified by the given refs-array into the trash
		-	param count indicates number of files contained in array refs
\*------------------------------------------------------------------------------*/
bool MoveToTrash( const entry_ref* refs, int32 count) {
	// this is based on code I got from Tim Vernum's Website. thx!
	static BmString desktopWin;
	if (!desktopWin.Length()) {
		// initialize (find) desktop-path:
		status_t err;
		BPath desktopPath;
		if ((err=find_directory( B_DESKTOP_DIRECTORY, 
										 &desktopPath, true)) != B_OK) {
			BM_SHOWERR( BmString("Could not find desktop-folder!\n\nError: ")
								<< strerror( err));
			desktopWin = "/boot/home/Desktop";
		} else
			desktopWin = desktopPath.Path();
	}
	BMessenger tracker( "application/x-vnd.Be-TRAK" );
	if (refs && tracker.IsValid()) {
		BMessage msg( B_DELETE_PROPERTY);
		BMessage specifier( 'sref' );
		char buf[1024];
		for( int i=0; i<count; ++i) {
			// add refs through AddData in order to set the array's size 
			// in advance:
			int32 nmLen = strlen( refs[i].name);
			memcpy( buf, &refs[i], sizeof( entry_ref));
			strcpy( buf+sizeof( entry_ref)-sizeof(char*), refs[i].name);
			specifier.AddData( "refs", B_REF_TYPE, buf, 
									 sizeof( const entry_ref)-sizeof(char*)+nmLen+1, 
									 false, count);
		}
		specifier.AddString( "property", "Entry" );
		msg.AddSpecifier( &specifier );

		msg.AddSpecifier( "Poses" );
		msg.AddSpecifier( "Window", desktopWin.String());

		BMessage reply;
		tracker.SendMessage( &msg, &reply);
		if (reply.what == B_MESSAGE_NOT_UNDERSTOOD) {
			// maybe we have to deal with a Tracker that does not have a 
			// window named '/boot/home/Desktop', but uses 'Desktop' instead,
			// we try that:
			if (desktopWin != "Desktop") {
				// try with 'Desktop':
				desktopWin = "Desktop";
				return MoveToTrash( refs, count);
			}
			// neither '/boot/home/Desktop' nor 'Desktop' seems to work,
			// we give up:
			return false;
		}
		return true;
	} else
		return false;
} 

/*------------------------------------------------------------------------------*\
	LivesInMailbox( eref)
		-	returns whether or not the given entry_ref lives inside the Mailbox
\*------------------------------------------------------------------------------*/
bool LivesInMailbox( const entry_ref& eref) {
	node_ref nref;
	nref.node = eref.directory;
	nref.device = eref.device;
	BmString key = BM_REFKEY(nref);
	BmRef<BmListModelItem> itemRef = TheMailFolderList->FindItemByKey(key);
	return itemRef != NULL;
}

/*------------------------------------------------------------------------------*\
	CheckMimeType( eref, type)
		-	checks if the file specified by eref is of the given mimetype
\*------------------------------------------------------------------------------*/
bool CheckMimeType( const entry_ref* eref, const char* type) {
	BmString realMT = DetermineMimeType( eref);
	return realMT.ICompare( type) == 0;
}

/*------------------------------------------------------------------------------*\
	DetermineMimeType( eref, doublecheck)
		-	determines the mimetype of the given file (according to BeOS)
\*------------------------------------------------------------------------------*/
BmString DetermineMimeType( const entry_ref* inref, bool doublecheck) {
	BEntry entry( inref, true);			// traverse links
	entry_ref eref;
	entry.GetRef( &eref);
	BNode node( &eref);
	if (node.InitCheck() == B_OK) {
		if (doublecheck)
			node.RemoveAttr( "BEOS:TYPE");
		BNodeInfo nodeInfo( &node);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimetype[B_MIME_TYPE_LENGTH+1];
			*mimetype = 0;
			if (nodeInfo.GetType( mimetype)!=B_OK) {
				// no mimetype info yet, we ask BeOS to determine mimetype 
				// and then try again:
				BPath path;
				entry.GetPath( &path);
				status_t res=entry.InitCheck();
				if (res==B_OK && path.InitCheck()==B_OK && path.Path())
					update_mime_info( path.Path(), false, true, true);
			}
			nodeInfo.SetTo( &node);
			if (nodeInfo.GetType( mimetype) == B_OK)
				return mimetype;
		}
	}
	return "application/octet-stream";	// basically means "unknown"
}

/*------------------------------------------------------------------------------*\
	EnsureIndexExists( attrName)
		-	create an index for the given attribute-name 
			(the index is created on the mailbox-volume).
\*------------------------------------------------------------------------------*/
void EnsureIndexExists( const char* attrName, int32 type) {
	struct index_info idxInfo;
	if (fs_stat_index( ThePrefs->MailboxVolume.Device(), attrName, 
							 &idxInfo) != 0) {
		status_t res = fs_create_index( ThePrefs->MailboxVolume.Device(), 
												  attrName, type, 0);
		if (res == -1)
			BM_SHOWERR( BmString("Could not create index for attribute ")
								<< attrName << ".\n\nError: " << strerror( errno));
	}
}

/*------------------------------------------------------------------------------*\
	FetchFile( filename, contents)
		-	reads the file specified by filename (complete path)
		-	stores the file's data into given string contents
\*------------------------------------------------------------------------------*/
bool FetchFile( BmString fileName, BmString& contents) {
	BFile file( fileName.String(), B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		off_t size;
		file.GetSize( &size);
		if (size>0) {
			char* buf = contents.LockBuffer( size);
			ssize_t read = file.Read( buf, size);
			read = MAX( 0, read);
			buf[read] = '\0';
			contents.UnlockBuffer( read);
		} else
			contents.Truncate( 0);
		return true;
	}
	return false;
}

typedef set<BmString> BmFileSet;
static BmFileSet nTempFiles;
BmTempFileList TheTempFileList;

/*------------------------------------------------------------------------------*\
	BmTempFileList()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmTempFileList::BmTempFileList()
	: mCount(0)
{
}

/*------------------------------------------------------------------------------*\
	~BmTempFileList()
		-	d'tor, removes all temporary files used during this session
\*------------------------------------------------------------------------------*/
BmTempFileList::~BmTempFileList() {
	BmFileSet::const_iterator iter;
	while( (iter=nTempFiles.begin()) != nTempFiles.end()) {
		RemoveFile( *iter);
	}
}

/*------------------------------------------------------------------------------*\
	AddFile( fileWithPath)
		-	adds given filename to the list of temporary files
\*------------------------------------------------------------------------------*/
void BmTempFileList::AddFile( BmString fileWithPath) {
	nTempFiles.insert( fileWithPath);
}

/*------------------------------------------------------------------------------*\
	RemoveFile( fileWithPath)
		-	removes file specified by fileWithPath from list of
			temporary files and from disk
\*------------------------------------------------------------------------------*/
void BmTempFileList::RemoveFile( BmString fileWithPath) {
	BEntry tmpFile( fileWithPath.String());
	tmpFile.Remove();
	nTempFiles.erase( fileWithPath);
}

/*------------------------------------------------------------------------------*\
	NextTempFileNameWithPath()
		-	returns the name to use for a new temporary file
\*------------------------------------------------------------------------------*/
BmString BmTempFileList::NextTempFilenameWithPath() {
	BPath tempPath;
	status_t err;
	if ((err=find_directory( B_COMMON_TEMP_DIRECTORY, &tempPath, true)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not find tmp-folder!\n\nError: ")
									<< strerror( err));
	return BmString(tempPath.Path()) << "/bm_" << ++mCount;
}

/*------------------------------------------------------------------------------*\
	BmReadStringAttr( node, attrName, outStr)
		-	
\*------------------------------------------------------------------------------*/
bool BmReadStringAttr( const BNode* node, const char* attrName, 
							  BmString& outStr) {
	attr_info attrInfo;
	BmString tmpStr;
	if (node->GetAttrInfo( attrName, &attrInfo) == B_OK) {
		long long size = max( (long long)0, attrInfo.size-1);
		char* buf = tmpStr.LockBuffer( size);
		node->ReadAttr( attrName, B_STRING_TYPE, 0, buf, size);
		tmpStr.UnlockBuffer( size);
	}
	if (tmpStr != outStr) {
		outStr.Adopt( tmpStr);
		return true;	// attribute has changed
	}
	return false;		// nothing has changed
}



/*------------------------------------------------------------------------------*\
	BmBackedFile()
		-	construct backed-file from given path
\*------------------------------------------------------------------------------*/
BmBackedFile::BmBackedFile( const char* filename, const char *mimetype,
									 const BEntry* backupEntry)
{
	SetTo( filename, mimetype, backupEntry);
}

/*------------------------------------------------------------------------------*\
	BmBackedFile()
		-	construct backed-file from given entry
\*------------------------------------------------------------------------------*/
BmBackedFile::BmBackedFile( const BEntry& entry, const char *mimetype,
									 const BEntry* backupEntry)
{
	SetTo( entry, mimetype, backupEntry);
}

/*------------------------------------------------------------------------------*\
	~BmBackedFile()
		-	sync's new file and then removes backup.
\*------------------------------------------------------------------------------*/
BmBackedFile::~BmBackedFile() {
	Finish();
}

/*------------------------------------------------------------------------------*\
	BmBackedFile()
		-	construct backed-file from given path
\*------------------------------------------------------------------------------*/
status_t BmBackedFile::SetTo( const char* filename, const char *mimetype,
										const BEntry* backupEntry)
{
	BEntry entry(filename, true);
	return SetTo(entry, mimetype, backupEntry);
}

/*------------------------------------------------------------------------------*\
	BmBackedFile()
		-	construct backed-file from given entry
\*------------------------------------------------------------------------------*/
status_t BmBackedFile::SetTo( const BEntry& entry, const char *mimetype,
										const BEntry* backupEntry)
{
	if (entry.InitCheck() != B_OK)
		return B_BAD_VALUE;
	if (backupEntry)
		mBackupEntry = *backupEntry;
	mEntry = entry;
	mMimeType = mimetype;
	return Init();
}

/*------------------------------------------------------------------------------*\
	Init()
		-	makes backup, if file already exists and then creates new file
\*------------------------------------------------------------------------------*/
status_t BmBackedFile::Init() {
	status_t err;
	char* filenameBuf = mFileName.LockBuffer(B_FILE_NAME_LENGTH);
	if (filenameBuf) {
		*filenameBuf = '\0';
		mEntry.GetName(filenameBuf);
		mFileName.UnlockBuffer();
	}

	if (mBackupEntry.InitCheck() != B_OK)
		mBackupEntry = mEntry;
	if (mBackupEntry.Exists()) {
		// file exists, we rename it to a unique backup-name:
		static int counter = 1;
		BmString backupExt("-backup");
		mBackupName.SetTo( mFileName, B_FILE_NAME_LENGTH-10-backupExt.Length());
		mBackupName << backupExt << "-" << counter++;
		if (mMimeType.Length()) {
			// change mimetype of backup in order to trigger the node-monitor
			// to invalidate it (remove the item from the listview):
			BNode node(&mBackupEntry);
			BNodeInfo nodeInfo;
			if ((err = nodeInfo.SetTo( &node)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not set node-info for file\n\t<") 
						<< mBackupName << ">\n\n Result: " << strerror(err)
				);
			nodeInfo.SetType( (mMimeType+"-backup").String());
		}
		// now rename the backup:
		if ((err = mBackupEntry.Rename( mBackupName.String(), true)) != B_OK) {
			BM_LOGERR( 
				BmString("Could not rename file <") << mFileName << "> to <"
					<< mBackupName << ">\n\n Result: " << strerror(err)
			);
			return err;
		}
	} else
		mBackupEntry.Unset();
	err = mFile.SetTo( &mEntry, B_WRITE_ONLY | B_CREATE_FILE);
	if (err != B_OK) {
		BM_LOGERR( 
			BmString("Could not create file\n\t<") 
				<< mFileName << ">\n\n Result: " << strerror(err)
		);
		return err;
	}
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	Finish()
		-	syncs the new file and removes the backup (if any)
\*------------------------------------------------------------------------------*/
void BmBackedFile::Finish() {
	if (mFile.InitCheck() == B_OK)
		mFile.Sync();
	if (mBackupEntry.InitCheck() == B_OK && mBackupEntry.Exists())
		mBackupEntry.Remove();
	if (mMimeType.Length()) {
		status_t err;
		BNodeInfo nodeInfo;
		if ((err = nodeInfo.SetTo( &mFile)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not set node-info for file\n\t<") 
					<< mFileName << ">\n\n Result: " << strerror(err)
			);
		nodeInfo.SetType( mMimeType.String());
	}
}

/*------------------------------------------------------------------------------*\
	Write()
		-	delegates writing to real file
\*------------------------------------------------------------------------------*/
ssize_t BmBackedFile::Write(const void *buffer, size_t size) {
	return mFile.Write( buffer, size);
}

/*------------------------------------------------------------------------------*\
	SetupFolder( name, dir)
		-	initializes the given BDirectory dir to the given path name
		-	if the directory does not yet exist, it is created
		-	a pointer to the initialized directory is returned, so you probably
			don't want to delete that
\*------------------------------------------------------------------------------*/
status_t SetupFolder( const BmString& name, BDirectory* dir) {
	status_t res = B_BAD_VALUE;
	if (dir) {
		res = dir->SetTo( name.String());
		if (res != B_OK) {
			if ((res = create_directory( name.String(), 0755) 
			|| (res = dir->SetTo( name.String()))) != B_OK) {
				BM_SHOWERR( BmString("Sorry, could not create folder ") << name
									<< ".\n\t Error:" << strerror( res));
			}
		}
	}
	return res;
}
