/*
	BmStorageUtil.h
		$Id$
*/

#ifndef _BmStorageUtil_h
#define _BmStorageUtil_h

#include <vector>

#include <String.h>

class entry_ref;

bool MoveToTrash( const entry_ref* refs, int32 count);

bool CheckMimeType( const entry_ref* eref, const char* type);

/*------------------------------------------------------------------------------*\
	BmTempFileList
		-	
\*------------------------------------------------------------------------------*/
class BmTempFileList {
	typedef vector<BString> BmFileVect;
public:
	BmTempFileList() : mCount(0) 			{}
	~BmTempFileList();
	void AddFile( BString fileWithPath);
	BString NextTempFilename()				{ return BString("bm_") << ++mCount; }
private:
	BmFileVect mFiles;
	int32 mCount;
	// Hide copy-constructor and assignment:
	BmTempFileList( const BmTempFileList&);
	BmTempFileList operator=( const BmTempFileList&);
};

extern BmTempFileList TheTempFileList;

#endif
