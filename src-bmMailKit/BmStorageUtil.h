/*
	BmStorageUtil.h
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


#ifndef _BmStorageUtil_h
#define _BmStorageUtil_h

#include "BmMailKit.h"

#include <Entry.h>
#include <File.h>

#include "BmString.h"


struct entry_ref;

IMPEXPBMMAILKIT 
status_t WatchNode( const node_ref *node, uint32 flags, BHandler *handler);

IMPEXPBMMAILKIT 
bool MoveToTrash( const entry_ref* refs, int32 count);

IMPEXPBMMAILKIT 
bool CheckMimeType( const entry_ref* eref, const char* type);

IMPEXPBMMAILKIT 
BmString DetermineMimeType( const entry_ref* eref, bool doublecheck=false);

IMPEXPBMMAILKIT 
void EnsureIndexExists( const char* attrName);

IMPEXPBMMAILKIT 
bool FetchFile( BmString fileName, BmString& contents);

IMPEXPBMMAILKIT 
bool BmReadStringAttr( const BNode* node, const char* attrName, BmString& out);

IMPEXPBMMAILKIT 
BmString BM_REFKEY( const node_ref& nref);

IMPEXPBMMAILKIT 
status_t SetupFolder( const BmString& name, BDirectory* dir);

/*------------------------------------------------------------------------------*\
	BmTempFileList
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmTempFileList {
public:
	BmTempFileList();
	~BmTempFileList();
	void AddFile( BmString fileWithPath);
	void RemoveFile( BmString fileWithPath);
	BmString NextTempFilename()			{ return BmString("bm_") << ++mCount; }
	BmString NextTempFilenameWithPath();
private:
	int32 mCount;
	// Hide copy-constructor and assignment:
	BmTempFileList( const BmTempFileList&);
	BmTempFileList operator=( const BmTempFileList&);
};

extern BmTempFileList TheTempFileList;

/*------------------------------------------------------------------------------*\
	BmBackedFile
		-	implements save storing into a file (by keeping a backup around).
\*------------------------------------------------------------------------------*/
class BmBackedFile {
public:
	BmBackedFile()								{}
	BmBackedFile( const char* filename, const char *mimetype = NULL,
					  const BEntry* = NULL);
	BmBackedFile( const BEntry& entry, const char *mimetype = NULL,
					  const BEntry* = NULL);
	~BmBackedFile();
	status_t SetTo( const char* filename, const char *mimetype = NULL,
						 const BEntry* = NULL);
	status_t SetTo( const BEntry& entry, const char *mimetype = NULL,
						 const BEntry* = NULL);
	ssize_t Write(const void *buffer, size_t size);
	BFile& File()								{ return mFile; }
	BEntry& BackupEntry()					{ return mBackupEntry; }
private:
	status_t Init();
	BmString mFileName;
	BmString mMimeType;
	BFile mFile;
	BmString mBackupName;
	BEntry mBackupEntry;
};

#endif
