/*
	BmPrefs.h

		$Id$
*/

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <stdexcept>

#include <Archivable.h>
#include <String.h>

#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPrefs : public BArchivable {
	// archivable components:
	static char* const MSG_DYNAMIC_CONN_WIN = "bm:dynconnwin";
	static char* const MSG_RECEIVE_TIMEOUT = 	"bm:recvtimeout";
public:
	BmPrefs( void)
			;
	BmPrefs( BMessage *archive)
			;
	virtual ~BmPrefs() 
			{}

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

	// setters:
	void CheckMail( TConnWinMode m) 		{ mDynamicConnectionWin = m; }
	void ReceiveTimeout( int16 i) 		{ mReceiveTimeout = i; }

	bool static InitPrefs();

private:
	// TODO: make these configurable by user (i.e. write a GUI):
	TConnWinMode mDynamicConnectionWin;	// show connections in connection-window only when
													// they are alive, 
													// show the ones with new mail
													// or display them all the time?
	int16 mReceiveTimeout;					// network-timeout for answer of servers
};

// global pointer to preferences:
namespace Beam {
	extern BmPrefs* Prefs;
}

#endif
