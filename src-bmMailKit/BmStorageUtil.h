/*
	BmStorageUtil.h
		$Id$
*/

#ifndef _BmStorageUtil_h
#define _BmStorageUtil_h

class entry_ref;

bool MoveToTrash( entry_ref* refs, int32 count);

bool CheckMimeType( entry_ref* eref, const char* type);

#endif
