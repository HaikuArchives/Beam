/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailMover_h
#define _BmMailMover_h

#include <Message.h>

#include "BmDataModel.h"
#include "BmUtil.h"

enum {
	BM_JOBWIN_MOVEMAILS = 'bmec'
						// sent to JobMetaController in order to move mails
};

class BmMailFolder;

/*------------------------------------------------------------------------------*\
	BmMailMover
		-	implements the moving of mails inside the file-system
		-	in general, each BmMailMover is started as a thread which exits when 
			the moving-operation has ended
\*------------------------------------------------------------------------------*/
class BmMailMover : public BmJobModel {
	typedef BmJobModel inherited;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_MOVER;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;
	static const char* const MSG_REFS;
	static const char* const MSG_REF_COUNT;

	BmMailMover( const BmString& name, entry_ref* refs, int32 refCount,
					 BmMailFolder* destFolder);
							// BmMailMover takes ownership of given refs-array!
	virtual ~BmMailMover();

	inline BmString Name() const			{ return ModelName(); }

	bool StartJob();

private:
	void UpdateStatus( const float delta, const char* filename, 
							 const char* currentCount);
	
	entry_ref* mRefs;
	int32 mRefCount;
	BmMailFolder* mDestFolder;

	// Hide copy-constructor and assignment:
	BmMailMover( const BmMailMover&);
	BmMailMover operator=( const BmMailMover&);
};

#endif
