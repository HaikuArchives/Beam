/*
	BmPrefs.h

		$Id$
*/

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <Archivable.h>
#include <Font.h>
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
	static const char* const MSG_DYNAMIC_CONN_WIN = 	"bm:dyncw";
	static const char* const MSG_RECEIVE_TIMEOUT = 		"bm:rcvtmo";
	static const char* const MSG_LOGLEVELS = 				"bm:lglev";
	static const char* const MSG_MAILBOXPATH = 			"bm:mbpath";
	static const char* const MSG_REF_CACHE_MEM = 		"bm:rfcachem";
	static const char* const MSG_REF_CACHE_DISK = 		"bm:rfcached";
	static const char* const MSG_DEFAULT_ENCODING = 	"bm:defenc";
	static const char* const MSG_STRIPED_LISTVIEW = 	"bm:strpdlv";
	static const char* const MSG_MAILREF_LAYOUT = 		"bm:mreflayout";
	static const char* const MSG_RESTORE_FOLDERS = 		"bm:rstfldrs";
	static const char* const MSG_HEADERVIEW_MODE = 		"bm:hdrvmode";

	static const char* const PREFS_FILENAME = 			"General Settings";

public:
	// creator-func, c'tors and d'tor:
	static BmPrefs* CreateInstance();
	BmPrefs( void);
	BmPrefs( BMessage* archive);
	virtual ~BmPrefs();

	// native methods:
	bool Store();

	// overrides of BArchivable base:
	static BArchivable* Instantiate( BMessage* archive);
	status_t Archive( BMessage* archive, bool deep = true) const;

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
	bool RefCacheInMem() const 			{ return mRefCacheInMem; }
	bool RefCacheOnDisk() const			{ return mRefCacheOnDisk; }
	int32 DefaultEncoding() const 		{ return mDefaultEncoding; }
	bool StripedListView() const 			{ return mStripedListView; }
	BMessage* MailRefLayout() const		{ return mMailRefLayout; }
	bool RestoreFolderStates() const 	{ return mRestoreFolderStates; }
	// stubs for later use:
	bool ShowDecodedLength() const 		{ return true; }

	// setters:
	void CheckMail( TConnWinMode m) 		{ mDynamicConnectionWin = m; }
	void ReceiveTimeout( int16 i) 		{ mReceiveTimeout = i; }
	void Loglevels( int32 i) 				{ mLoglevels = i; }
	void MailboxPath( BString& s) 		{ mMailboxPath = s; }
	void RefCacheInMem( bool b) 			{ mRefCacheInMem = b; }
	void RefCacheOnDisk( bool b) 			{ mRefCacheOnDisk = b; }
	void DefaultEncoding( int32 i) 		{ mDefaultEncoding = i; }
	void StripedListView( bool b) 		{ mStripedListView = b; }
	void MailRefLayout( BMessage* m) 	{ mMailRefLayout = m; }
	void RestoreFolderStates( bool b) 	{ mRestoreFolderStates = b; }

	static BmPrefs* theInstance;

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

	bool mRefCacheInMem;
							// toggles caching of mailref-lists in memory.
							// true ->	current mailreflist stays in memory when another
							//				folder ist selected and is reused when current folder
							//				gets reselected later (this is faster).
							// false -> current mailreflist is deleted when another folder is
							//				selected and is reconstructed from disk when current folder
							//				gets reselected later (this uses less memory).

	bool mRefCacheOnDisk;
							// toggles caching of mailref-lists on disk.
							// true ->	for each folder a list of mails contained inside it is
							//				created on disk (a mailref-cache). Whenever a folder is
							//				selected, the mailref-cache is read in order to display
							//				the mails that the folder contains.
							//				N.B.: The cache is automatically updated whenever a change
							//						in the mail-folder is detected (modification-time of
							//						folder is newer than modification-time of cache-file).
							// false -> No caches are maintained, i.e. the mails are collected from
							//				disk every time a folder is selected (unless mRefCacheInMem
							//				is set)
	// A general note:
	// 	at least on my system (2xPII,233MHz, older IDE disk), the disk cache has a much
	//		greater impact on the performance than the memory cache. This should generally be
	//		the case, since seeking on the disk (while searching for mails) is really slow.
	//		Therefore, my suggestion is: enable disk-cache, disable memory-cache.

	int32 mDefaultEncoding;
							// The default encoding for messages.
							// This is used in two circumstances:
							//	-	when a received mail does not indicate any encoding
							//	-	when a new mail is created in order to be sent

	bool mStripedListView;
							// toggles striping of listview, striped mode is faster,
							// but I'm sure some people will prefer dotted-tail-mode 
							// (which is slower).
	BMessage* mMailRefLayout;
							// archive of default state-info for MailRef-listview
	bool mRestoreFolderStates;
							// determines whether or not the states of each mail-folder
							// (e.g. expanded or not) will be restored as they were at the
							// end of the last sesson.
};

#define ThePrefs BmPrefs::theInstance

#endif
