/*
	BmMailMover.cpp
		- Implements the main POP3-client-class: BmMailMover

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

/*------------------------------------------------------------------------------*\
	BmMailMover()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailMover::BmMailMover( const BmString& name, BList* refList, BmMailFolder* destFolder)
	:	BmJobModel( name)
	,	mRefList( refList)
	,	mDestFolder( destFolder)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailMover()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailMover::~BmMailMover() { 
	if (mRefList) {
		entry_ref* ref;
		while( (ref = static_cast<entry_ref*>( mRefList->RemoveItem( (int32)0)))!=NULL)
			delete ref;
		delete mRefList;
	}
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, moves all mails to new home
\*------------------------------------------------------------------------------*/
bool BmMailMover::StartJob() {

	int32 refCount = mRefList->CountItems();
	const float delta =  100.0 / (refCount / GRAIN);
	status_t err;

	BDirectory destDir( mDestFolder->EntryRefPtr());
	char filename[B_FILE_NAME_LENGTH+1];
	entry_ref* ref;
	BEntry entry;
	node_ref destNodeRef;
	destDir.GetNodeRef( &destNodeRef);
	// move each mailref into the destination folder:
	try {
		for( int32 i=0; ShouldContinue() && i<refCount; ++i) {
			ref = static_cast<entry_ref*>( mRefList->ItemAt(i));
			if (ref->directory == destNodeRef.node && ref->device == destNodeRef.device)
				continue;						// no move neccessary, already at 'new' home
			(err = entry.SetTo( ref)) == B_OK
													||	BM_THROW_RUNTIME(BmString("couldn't create entry for <")<<ref->name<<"> \n\nError:" << strerror(err));
			err = entry.MoveTo( &destDir);
			if ( err == B_FILE_EXISTS) {
				// increment counter until we have found a unique name:
				int32 counter=1;
				while ( (err = entry.MoveTo( &destDir, (BmString(ref->name)<<"_"<<counter++).String())) == B_FILE_EXISTS)
					;
			}
			if (err != B_OK)
				throw BM_runtime_error(BmString("couldn't move <")<<ref->name<<"> \n\nError:" << strerror(err));
			if ((i+1)%(int)GRAIN == 0) {
				entry.GetName( filename);
				BmString currentCount = BmString()<<i<<" of "<<refCount;
				UpdateStatus( delta, filename, currentCount.String());
			}
			snooze( 20*1000);					// give node-monitor a chance to keep up... 
													// (it will drop messages if we go too fast)
		}
		entry.GetName( filename);
		BmString currentCount = BmString()<<refCount<<" of "<<refCount;
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
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MOVER, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, filename);
	msg->AddString( MSG_TRAILING, currentCount);
	TellControllers( msg.get());
}
