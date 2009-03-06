/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <memory.h>
#include <memory>
#include <stdio.h>

#include <Directory.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailMover.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

static const float GRAIN = 1.0;

const char* const BmMailMover::MSG_MOVER = 		"bm:mover";
const char* const BmMailMover::MSG_DELTA = 		"bm:delta";
const char* const BmMailMover::MSG_TRAILING = 	"bm:trailing";
const char* const BmMailMover::MSG_LEADING = 	"bm:leading";
const char* const BmMailMover::MSG_REFS = 		"refs";
const char* const BmMailMover::MSG_REF_COUNT = 	"refc";

/*------------------------------------------------------------------------------*\
	BmMailMover()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailMover::BmMailMover( const BmString& name, entry_ref* refs, 
								  int32 refCount, BmMailFolder* destFolder)
	:	BmJobModel( name)
	,	mRefs( refs)
	,	mRefCount( refCount)
	,	mDestFolder( destFolder)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailMover()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailMover::~BmMailMover() { 
	delete [] mRefs;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, moves all mails to new home
\*------------------------------------------------------------------------------*/
bool BmMailMover::StartJob() {

	if (!mRefs)
		return false;
	const float delta =  100.0 / (mRefCount / GRAIN);
	status_t err;

	BDirectory destDir( mDestFolder->EntryRefPtr());
	char filename[B_FILE_NAME_LENGTH+1];
	entry_ref* ref;
	BEntry entry;
	node_ref destNodeRef;
	destDir.GetNodeRef( &destNodeRef);
	// move each mailref into the destination folder:
	try {
		int32 i;
		for( i=0; ShouldContinue() && i < mRefCount; ++i) {
			ref = &mRefs[i];
			if (ref->directory == destNodeRef.node 
			&& ref->device == destNodeRef.device)
				continue;						
							// no move neccessary, already at 'new' home
			if ((err = entry.SetTo( ref)) != B_OK)
				BM_THROW_RUNTIME( BmString("couldn't create entry for <")
											<< ref->name << "> \n\nError:" 
											<< strerror(err));
			err = entry.MoveTo( &destDir);
			if ( err == B_FILE_EXISTS) {
				// increment counter until we have found a unique name:
				int32 counter=1;
				while ( (err = entry.MoveTo( 
					&destDir, 
					(BmString(ref->name) << "_" << counter++).String()
				)) == B_FILE_EXISTS)
					;
			}
			if (err != B_OK)
				throw BM_runtime_error(
					BmString("couldn't move <") << ref->name << "> \n\nError:" 
						<< strerror(err)
				);
			if ((i+1)%(int)GRAIN == 0) {
				entry.GetName( filename);
				BmString currentCount = BmString()<<i<<" of "<<mRefCount;
				UpdateStatus( delta, filename, currentCount.String());
			}
			snooze( 20*1000);
							// give node-monitor a chance to keep up... 
							// (it will drop messages if we go too fast)
		}
		entry.GetName( filename);
		BmString currentCount = BmString()<<i<<" of "<<mRefCount;
		UpdateStatus( delta, filename, currentCount.String());
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		BmString text = Name() << "\n\n" << errstr;
		BM_SHOWERR( BmString("BmMailMover: ") << text);
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdateStatus()
		-	informs the interested party about a change in the current state
\*------------------------------------------------------------------------------*/
void BmMailMover::UpdateStatus( const float delta, const char* filename, 
										  const char* currentCount) {
	std::auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MOVER, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, filename);
	if (!ShouldContinue())
		msg->AddString( MSG_TRAILING, 
							 (BmString(currentCount) << ", Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, currentCount);
	TellControllers( msg.get());
}
