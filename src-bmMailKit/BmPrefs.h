/*
	BmPrefs.h

		$Id$
*/

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <Archivable.h>
#include <String.h>

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPrefs : public BArchivable {
	typedef BArchivable inherited;
	// archivable components:
	static const char* const MSG_DYNAMIC_CONN_WIN = "bm:dynconnwin";
	static const char* const MSG_RECEIVE_TIMEOUT = 	"bm:recvtimeout";
	static const char* const MSG_LOGLEVELS = 			"bm:loglevels";
	static const char* const MSG_MAILBOXPATH = 		"bm:mailboxpath";
	static const char* const MSG_REF_CACHING = 		"bm:refcaching";
	static const char* const MSG_DEFAULT_ENCODING = "bm:defencoding";

	static const char* const PREFS_FILENAME = 		"General Settings";

public:
	//
	static BmPrefs* CreateInstance();
	//
	BmPrefs( void);
	BmPrefs( BMessage* archive);
	virtual ~BmPrefs() 						{}

	bool Store();
	
	// stuff needed for BArchivable:
	static BArchivable* Instantiate( BMessage* archive)
			;
	virtual status_t Archive( BMessage* archive, bool deep = true) const
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
	uint32 Loglevels() const 				{ return mLoglevels; }
	const BString& MailboxPath() const 	{ return mMailboxPath; }
	bool RefCaching() const 				{ return mRefCaching; }
	int32 DefaultEncoding() const 		{ return mDefaultEncoding; }

	// setters:
	void CheckMail( TConnWinMode m) 		{ mDynamicConnectionWin = m; }
	void ReceiveTimeout( int16 i) 		{ mReceiveTimeout = i; }
	void Loglevels( int32 i) 				{ mLoglevels = i; }
	void MailboxPath( BString& s) 		{ mMailboxPath = s; }
	void RefCaching( bool b) 				{ mRefCaching = b; }
	void DefaultEncoding( int32 i) 		{ mDefaultEncoding = i; }


private:
	// TODO: make these configurable by user (i.e. write a GUI):
	TConnWinMode mDynamicConnectionWin;	
							// show connections in connection-window only when
							// they are alive, 
							// show the ones with new mail
							// or display them all the time?

	int16 mReceiveTimeout;
							// network-timeout for answer of servers

	int32 mLoglevels;
							// default loglevels, for each logflag
							// 2 bits are used (allowing levels from 
							// 0 [off] to 3 [full])

	BString mMailboxPath;
							// Path of mailbox-dir (usually '/boot/home/mail')

	bool mRefCaching;
							// switches mailref-caching on or off

	int32 mDefaultEncoding;
							// The default encoding for messages.
							// This is used in two circumstances:
							//	-	when a received mail does not indicate any encoding
							//	-	when a new mail is created in order to be sent
};

#endif
