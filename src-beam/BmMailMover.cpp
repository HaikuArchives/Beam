/*
	BmMailMover.cpp
		- Implements the main POP3-client-class: BmMailMover

		$Id$
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

/*------------------------------------------------------------------------------*\
	BmMailMover()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailMover::BmMailMover( const BString& name, BList* refList, BmMailFolder* destFolder)
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
		while( (ref = static_cast<entry_ref*>( mRefList->RemoveItem( (int32)0))))
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
	// move each mailref into the destination folder:
	try {
		for( int32 i=0; i<refCount; ++i) {
			ref = static_cast<entry_ref*>( mRefList->ItemAt(i));
			(err = entry.SetTo( ref)) == B_OK
													||	BM_THROW_RUNTIME(BString("couldn't create entry for <")<<ref->name<<"> \n\nError:" << strerror(err));
			err = entry.MoveTo( &destDir);
			if ( err == B_FILE_EXISTS) {
				// increment counter until we have found a unique name:
				int32 counter=1;
				while ( (err = entry.MoveTo( &destDir, (BString(ref->name)<<"_"<<counter++).String())) == B_FILE_EXISTS)
					;
			}
			if (err != B_OK)
				throw BM_runtime_error(BString("couldn't move <")<<ref->name<<"> \n\nError:" << strerror(err));
			if ((i+1)%(int)GRAIN == 0) {
				entry.GetName( filename);
				BString currentCount = BString()<<i<<" of "<<refCount;
				UpdateStatus( delta, filename, currentCount.String());
			}
			snooze( 20*1000);
		}
		entry.GetName( filename);
		BString currentCount = BString()<<refCount<<" of "<<refCount;
		UpdateStatus( delta, filename, currentCount.String());
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BString errstr = err.what();
		BString text = Name() << "\n\n" << errstr;
		BM_SHOWERR( BString("BmMailMover: ") << text);
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
