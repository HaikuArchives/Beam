/*
	BmUtil.h
		$Id$
*/

#ifndef _BmUtil_h
#define _BmUtil_h

#include <map>
#include <stdio.h>
#include <stdexcept>

#include <Message.h>
#include <StopWatch.h>
#include <String.h>

#include <libbenaphore/benaphore.h>

//---------------------------------------------------
class network_error : public runtime_error {
public:
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
class BmLogHandler {
	class BmLogfile {
	public:
		BmLogfile( const char* const fn)
			: logfile( NULL)
			, filename(fn)
			, watch("BmLOG", true) 
			{}
		~BmLogfile() { if (logfile) fclose(logfile); }
		void Write( const char* const msg);
	
		static BString LogPath;
	
	private:
		FILE* logfile;
		BString filename;
		BStopWatch watch;
	};

	typedef map<const char* const, BmLogfile*> LogfileMap;
	LogfileMap mActiveLogs;
	Benaphore mBenaph;

public:
	void CloseLog( const char* const logname);
	void CloseLog( const BString &logname) 
							{ CloseLog( logname.String()); }

	void LogToFile( const char* const logname, const char* const msg);
	void LogToFile( const char* const logname, const BString &msg) 
							{ LogToFile( logname, msg.String()); }
	void LogToFile( BString &logname, const char* const msg)
							{ LogToFile( logname.String(), msg); }
	void LogToFile( BString &logname, const BString &msg) 
							{ LogToFile( logname.String(), msg.String()); }

	BmLogHandler() : mBenaph("beam_loghandler") { }
	~BmLogHandler();
};

namespace Beam {
	extern BmLogHandler* LogHandler;
};

#ifdef LOGGING
#define BmLOG(msg) Beam::LogHandler->LogToFile( LOGNAME, msg)
#define BmLOG_FINISH(name) Beam::LogHandler->CloseLog( name)
#else
#define BmLOG(msg)
#define BmLOG_FINISH(name)
#endif
#define LOGNAME "beam"

//---------------------------------------------------
BString BytesToString( int32 bytes);

#endif
