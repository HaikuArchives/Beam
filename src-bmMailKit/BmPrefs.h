/*
	BmPrefs.h

		$Id$
*/

#ifndef _BmPrefs_h
#define _BmPrefs_h

#include <map>

#include <Archivable.h>
#include <Font.h>
#include <Message.h>
#include <String.h>

/*------------------------------------------------------------------------------*\
	BmPrefs 
		-	holds preference information for Beam
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPrefs : public BArchivable {
	typedef BArchivable inherited;

	static const char* const PREFS_FILENAME = 			"General Settings";

public:
	// creator-func, c'tors and d'tor:
	static BmPrefs* CreateInstance();
	BmPrefs( void);
	BmPrefs( BMessage* archive);
	virtual ~BmPrefs();

	// native methods:
	void InitDefaults();
	void SetLoglevels();
	bool Store();
	bool GetBool( const char* name);
	bool GetBool( const char* name, const bool defaultVal);
	int32 GetInt( const char* name);
	int32 GetInt( const char* name, const int32 defaultVal);
	const BMessage* GetMsg( const char* name);
	const BMessage* GetMsg( const char* name, const BMessage* defaultVal);
	BString GetString( const char* name);
	BString GetString( const char* name, const BString defaultVal);

	static BmPrefs* theInstance;

private:

	BMessage mPrefsMsg;
	BMessage mDefaultsMsg;

	map<BString, BMessage*> mMsgCache;

	// TODO: make these configurable by user (i.e. write a GUI):
/*
	bool DynamicStatusWin;	
							// show jobs in job-window only when
							// they are alive, 
							// or display them all the time?

	int ReceiveTimeout;
							// network-timeout for answer of servers

	int Loglevels;
							// default loglevels, for each logflag
							// 2 bits are used (allowing levels from 
							// 0 [off] to 3 [full])

	BString MailboxPath;
							// Path of mailbox-dir (usually '/boot/home/mail')

	bool RefCacheInMem;
							// toggles caching of mailref-lists in memory.
							// true ->	current mailreflist stays in memory when another
							//				folder ist selected and is reused when current folder
							//				gets reselected later (this is faster).
							// false -> current mailreflist is deleted when another folder is
							//				selected and is reconstructed from disk when current folder
							//				gets reselected later (this uses less memory).

	bool RefCacheOnDisk;
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

	int mDefaultEncoding;
							// The default encoding for messages.
							// This is used in two circumstances:
							//	-	when a received mail does not indicate any encoding
							//	-	when a new mail is created in order to be sent

	bool StripedListView;
							// toggles striping of listview, striped mode is faster,
							// but I'm sure some people will prefer dotted-tail-mode 
							// (which is slower).
	msg MailRefLayout;
							// archive of default state-info for MailRef-listview
	bool RestoreFolderStates;
							// determines whether or not the states of each mail-folder
							// (e.g. expanded or not) will be restored as they were at the
							// end of the last sesson.

	bool ShowDecodedLength;
							// determines if the length of attachments should be computed
							// for the decoded state (slower) or as is (less useful...)
*/


};

#define ThePrefs BmPrefs::theInstance

#endif
