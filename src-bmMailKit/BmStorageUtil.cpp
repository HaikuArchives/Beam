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


//#include <stdio.h> 
#include <errno.h> 

#include <Directory.h> 
#include <Messenger.h> 
#include <Message.h> 
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

BmTempFileList TheTempFileList;

// -----------------------------------------------------------------------------
BmString BM_REFKEY( const node_ref& nref) {
	return BmString() << nref.node;
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
void EnsureIndexExists( const char* attrName) {
	struct index_info idxInfo;
	if (fs_stat_index( ThePrefs->MailboxVolume.Device(), attrName, 
							 &idxInfo) != 0) {
		status_t res = fs_create_index( ThePrefs->MailboxVolume.Device(), 
												  attrName,
												  B_STRING_TYPE, 0);
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
		char* buf = contents.LockBuffer( size);
		ssize_t read = file.Read( buf, size);
		read = MAX( 0, read);
		buf[read] = '\0';
		contents.UnlockBuffer( read);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	~BmTempFileList()
		-	d'tor, removes all temporary files used during this session
\*------------------------------------------------------------------------------*/
BmTempFileList::~BmTempFileList() {
	BmFileSet::const_iterator iter;
	while( (iter=mFiles.begin()) != mFiles.end()) {
		RemoveFile( *iter);
	}
}

/*------------------------------------------------------------------------------*\
	AddFile( fileWithPath)
		-	adds given filename to the list of temporary files
\*------------------------------------------------------------------------------*/
void BmTempFileList::AddFile( BmString fileWithPath) {
	mFiles.insert( fileWithPath);
}

/*------------------------------------------------------------------------------*\
	RemoveFile( fileWithPath)
		-	removes file specified by fileWithPath from list of
			temporary files and from disk
\*------------------------------------------------------------------------------*/
void BmTempFileList::RemoveFile( BmString fileWithPath) {
	BEntry tmpFile( fileWithPath.String());
	tmpFile.Remove();
	mFiles.erase( fileWithPath);
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
void BmReadStringAttr( const BNode* node, const char* attrName, 
							  BmString& outStr) {
	attr_info attrInfo;
	if (node->GetAttrInfo( attrName, &attrInfo) != B_OK) {
		outStr.Truncate( 0);
		return;
	}
	long long size = max( (long long)0, attrInfo.size-1);
	char* buf = outStr.LockBuffer( size);
	node->ReadAttr( attrName, B_STRING_TYPE, 0, buf, size);
	outStr.UnlockBuffer( size);
}
