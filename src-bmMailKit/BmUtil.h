/*
	BmUtil.h		-	$Id$
*/

#ifndef _BmUtil_h
#define _BmUtil_h

#include <stdio.h>
#include <stdexcept>
#include <string>

#include <Message.h>
#include <StopWatch.h>
#include <String.h>

//---------------------------------------------------
class network_error : public runtime_error {
public:
  network_error (const string& what_arg): runtime_error (what_arg) { }
  network_error (const BString& what_arg): runtime_error (what_arg.String()) { }
  network_error (char *const what_arg): runtime_error (what_arg) { }
};

//---------------------------------------------------
const char *FindMsgString( BMessage* archive, char* name);
bool FindMsgBool( BMessage* archive, char* name);
int32 FindMsgInt32( BMessage* archive, char* name);
int16 FindMsgInt16( BMessage* archive, char* name);
float FindMsgFloat( BMessage* archive, char* name);

//---------------------------------------------------
class BmLogfile {
public:
	BmLogfile( )
		: filename()
		, watch("BmLOG", true) 
		{}
	BmLogfile( const char* const fn)
		: filename(fn)
		, watch("BmLOG", true) 
		{}
	BmLogfile( const BString &fn)
		: filename(fn.String())
		, watch("BmLOG", true) 
		{}
	BmLogfile( const string &fn)
		: filename(fn.c_str())
		, watch("BmLOG", true) 
		{}
	~BmLogfile() { if (logfile) fclose(logfile); }
	void SetFilename( const char*  const fn) { filename = fn; }
	void SetFilename( const BString &fn) { filename = fn; }
	void SetFilename( const string &fn) { filename = fn.c_str(); }
	void Log( const char* const msg) { Write(msg); }
	void Log( const BString &msg) { Write(msg.String()); }
	void Log( const string &msg) { Write(msg.c_str()); }

	static BString LogPath;

private:
	void Write( const char* const msg);
	FILE* logfile;
	BString filename;
	BStopWatch watch;
};

#ifdef LOGGING
#define BmLOG(msg) log.Log(msg)
#else
#define BmLOG(msg)
#endif


#endif
