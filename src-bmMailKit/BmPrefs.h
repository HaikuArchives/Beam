/*
	BmPrefs.h

		$Id$
*/

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <stdexcept>

#include <Archivable.h>
#include <StopWatch.h>
#include <String.h>

#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPrefs : public BArchivable {
	typedef BArchivable inherited;
	// archivable components:
	static char* const MSG_DYNAMIC_CONN_WIN = "bm:dynconnwin";
	static char* const MSG_RECEIVE_TIMEOUT = 	"bm:recvtimeout";
	static char* const MSG_LOGLEVELS = 			"bm:loglevels";

	static char* const PREFS_FILENAME = 		"General Settings";

	static BString PrefsFilePath;

public:
	BmPrefs( void)
			;
	BmPrefs( BMessage *archive)
			;
	virtual ~BmPrefs() 
			{}

	bool BmPrefs::Store()
			;

	// stuff needed for BArchivable:
	static BArchivable *Instantiate( BMessage *archive)
			;
	virtual status_t Archive( BMessage *archive, bool deep = true) const
			;

	// possible modes of BmConnectionWin:
	enum TConnWinMode {
		CONN_WIN_STATIC = 0,
		CONN_WIN_DYNAMIC_EMPTY,
		CONN_WIN_DYNAMIC
	};

	// getters:
	TConnWinMode DynamicConnectionWin() const	{ return mDynamicConnectionWin; }
	int16 ReceiveTimeout() const 			{ return mReceiveTimeout; }
	int32 Loglevels() const 				{ return mLoglevels; }

	// setters:
	void CheckMail( TConnWinMode m) 		{ mDynamicConnectionWin = m; }
	void ReceiveTimeout( int16 i) 		{ mReceiveTimeout = i; }
	void Loglevels( int32 i) 				{ mLoglevels = i; }

	bool static InitPrefs();

	BStopWatch StopWatch;
	
private:
	// TODO: make these configurable by user (i.e. write a GUI):
	TConnWinMode mDynamicConnectionWin;	// show connections in connection-window only when
													// they are alive, 
													// show the ones with new mail
													// or display them all the time?
	int16 mReceiveTimeout;					// network-timeout for answer of servers
	int32 mLoglevels;							// default loglevels, for each logflag
													// 2 bits are used (allowing levels from 
													// 0 [off] to 3 [full])
};

// global pointer to preferences:
namespace Beam {
	extern BmPrefs* Prefs;
}

#endif
