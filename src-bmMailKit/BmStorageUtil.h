/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
bool LivesInMailbox( const entry_ref& eref);

IMPEXPBMMAILKIT 
bool CheckMimeType( const entry_ref* eref, const char* type);

IMPEXPBMMAILKIT 
BmString DetermineMimeType( const entry_ref* eref, bool doublecheck=false);

IMPEXPBMMAILKIT 
void EnsureIndexExists( const char* attrName, int32 type);

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

extern IMPEXPBMMAILKIT BmTempFileList TheTempFileList;

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
	void Finish();
	BEntry mEntry;
	BmString mFileName;
	BmString mMimeType;
	BFile mFile;
	BmString mBackupName;
	BEntry mBackupEntry;
};

#endif
