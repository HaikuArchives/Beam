/*
	BmLogHandler.h
		$Id$
*/

#ifndef _BmLogHandler_h
#define _BmLogHandler_h

#include <map>
#include <stdio.h>

#include <Locker.h>
#include <StopWatch.h>
#include <String.h>

/*------------------------------------------------------------------------------*\*\
	BmLogHandler
		-	implements the global log-handler that received all logging requests and
			executes them
		-	different logfiles are identified by their name and will be created on demand
\*------------------------------------------------------------------------------*/
class BmLogHandler {

public:
	//
	static BmLogHandler* CreateInstance( uint32 logLevels);
	//
	BmLogHandler( uint32 logLevels);
	~BmLogHandler();

	void CloseLog( const BString &logname);

	void LogToFile( const BString &logname, uint32 flag, const BString &msg, int8 minlevel=1);
	void LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1)
							{ LogToFile( BString(logname), flag, BString(msg), minlevel); }

	BStopWatch StopWatch;

private:
	/*------------------------------------------------------------------------------*\*\
		BmLogfile
			-	implements a single logfile
			-	the actual logging takes place in here
	\*------------------------------------------------------------------------------*/
	class BmLogfile {
	public:
		BmLogfile( const BString &fn);
		~BmLogfile() 							{ if (logfile) fclose(logfile); }
		void Write( const char* const msg, uint32 flag, 
						int8 minlevel, uint32 loglevels);
	
		static BString LogPath;
	
	private:
		FILE* logfile;
		BString filename;
	};

	typedef map<const BString, BmLogfile*> LogfileMap;
	LogfileMap mActiveLogs;					// map of names to logfiles
	BLocker mLocker;							// benaphore used to lock write-access to map
	uint32 mLoglevels;
};

/*------------------------------------------------------------------------------*\*\
	macros, constants and defines that facilitate logging-functionality:
\*------------------------------------------------------------------------------*/

// the different "terrains" we will be logging, each of them
// has its own loglevel:
extern const int16 BM_LogPop;
extern const int16 BM_LogConnWin;
extern const int16 BM_LogMailParse;
extern const int16 BM_LogUtil;
extern const int16 BM_LogMailFolders;
extern const int16 BM_LogFolderView;
extern const int16 BM_LogRefView;
extern const int16 BM_LogMainWindow;
extern const int16 BM_LogModelController;
extern const int16 BM_LogAll;

// macros to convert the loglevel for a specific flag 
// into it's internal bit-representation:
#define BM_LOGLVL0(flag) (0)
#define BM_LOGLVL1(flag) (flag)
#define BM_LOGLVL2(flag) (flag<<16)
#define BM_LOGLVL3(flag) (flag+flag<<16)

// the macros used for logging:
#ifdef BM_LOGGING

#define BM_LOG(flag,msg) \
	bmApp->LogHandler->LogToFile( BM_LOGNAME, flag, msg)
#define BM_LOG2(flag,msg) \
	bmApp->LogHandler->LogToFile( BM_LOGNAME, flag, msg, 2)
#define BM_LOG3(flag,msg) \
	bmApp->LogHandler->LogToFile( BM_LOGNAME, flag, msg, 3)
#define BM_LOGERR(msg) \
	bmApp->LogHandler->LogToFile( "Errors", BM_LogAll, msg, 0)
#define BM_LOG_FINISH(name) bmApp->LogHandler->CloseLog( name)
#define BM_LOGNAME "Beam"
#define BM_SHOWERR(msg) \
	{	if (bmApp->LogHandler) { \
			bmApp->LogHandler->LogToFile( BM_LOGNAME, BM_LogAll, msg, 0); \
		}\
		BM_LOGERR(msg); \
		ShowAlert( msg);	}

#else

#define BM_LOG(flag,msg)
#define BM_LOG2(flag,msg)
#define BM_LOG3(flag,msg)
#define BM_LOGERR(msg)
#define BM_LOG_FINISH(name)
#define BM_LOGNAME
#define BM_SHOWERR(msg) 			ShowAlert( msg)

#endif


#endif
