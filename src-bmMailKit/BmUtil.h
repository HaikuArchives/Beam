/*
	BmUtil.h
		$Id$
*/

#ifndef _BmUtil_h
#define _BmUtil_h

#include <map>
#include <stdio.h>
#include <stdexcept>
#include <string>

#include <Message.h>
#include <StopWatch.h>
#include <String.h>

#include <libbenaphore/benaphore.h>

//---------------------------------------------------
class network_error : public runtime_error {
public:
  network_error (const string& what_arg): runtime_error (what_arg.c_str()) { }
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
	
		static string LogPath;
	
	private:
		FILE* logfile;
		string filename;
		BStopWatch watch;
	};

	typedef map<const char* const, BmLogfile*> LogfileMap;
	LogfileMap mActiveLogs;
	Benaphore mBenaph;

public:
	void LogToFile( const char* const logname, const char* const msg);
	void LogToFile( const char* const logname, const string &msg) 
							{ LogToFile( logname, msg.c_str()); }
	void LogToFile( string &logname, const char* const msg)
							{ LogToFile( logname.c_str(), msg); }
	void LogToFile( string &logname, const string &msg) 
							{ LogToFile( logname.c_str(), msg.c_str()); }

	BmLogHandler() : mBenaph("beam_loghandler") { }
	~BmLogHandler();
};

namespace Beam {
	extern BmLogHandler LogHandler;
};

#ifdef LOGGING
#define BmLOG(msg) Beam::LogHandler.LogToFile( LOGNAME, msg)
#else
#define BmLOG(msg)
#endif
#define LOGNAME "beam"

//---------------------------------------------------
string BytesToString( int32 bytes);

//---------------------------------------------------
string iToStr( int32 i);
string fToStr( double f);

#endif
