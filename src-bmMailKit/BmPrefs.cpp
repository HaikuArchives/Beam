/*
	BmPopAccount.cpp

		$Id$
*/

#include <Message.h>
#include <ByteOrder.h>

#include "BmPrefs.h"

// global pointer to preferences:
namespace Beam {
	BmPrefs* Prefs = 0;
}

/*------------------------------------------------------------------------------*\
	InitPrefs()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, default prefs are used
\*------------------------------------------------------------------------------*/
bool BmPrefs::InitPrefs() 
{
	// ToDo: we should really look for the prefs file first...
	Beam::Prefs = new BmPrefs;
	return true;
}

/*------------------------------------------------------------------------------*\
	BmPrefs()
		-	default constructor
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( void)
	: BArchivable() 
	, mDynamicConnectionWin( CONN_WIN_DYNAMIC)
	, mReceiveTimeout( 60 )
	, mLoglevels( BM_LOGLVL2(BM_LogPop) 
					| BM_LOGLVL3(BM_LogConnWin) 
					| BM_LOGLVL3(BM_LogMailParse) 
					| BM_LOGLVL3(BM_LogUtil) 
					)
	, StopWatch( "Beam", true)
{
}

/*------------------------------------------------------------------------------*\
	BmPrefs( archive)
		-	constructs a BmPrefs from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( BMessage *archive) 
	: BArchivable( archive)
	, StopWatch( "Beam", true)
{
	mDynamicConnectionWin = static_cast<TConnWinMode>(FindMsgInt16( archive, MSG_DYNAMIC_CONN_WIN));
	mReceiveTimeout = ntohs(FindMsgInt16( archive, MSG_RECEIVE_TIMEOUT));
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPrefs into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPrefs::Archive( BMessage *archive, bool deep) const {
	status_t ret = (BArchivable::Archive( archive, deep)
		||	archive->AddString("class", "BmPrefs")
		||	archive->AddInt16( MSG_DYNAMIC_CONN_WIN, mDynamicConnectionWin)
		||	archive->AddInt16( MSG_RECEIVE_TIMEOUT, htons(mReceiveTimeout)));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Instantiate( archive)
		-	(re-)creates a PopAccount from a given BMessage
\*------------------------------------------------------------------------------*/
BArchivable* BmPrefs::Instantiate( BMessage *archive) {
	if (!validate_instantiation( archive, "BmPrefs"))
		return NULL;
	return new BmPrefs( archive);
}
