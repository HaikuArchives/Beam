/*
	BmLogHandler.h
		$Id$
*/

#ifndef _BmLogHandler_h
#define _BmLogHandler_h

#include <map>
#include <stdio.h>

#include <Locker.h>
#include <Looper.h>
#include <StopWatch.h>
#include <String.h>

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmLogfile:
\*------------------------------------------------------------------------------*/
#define BM_LOG_MSG						'bmia'

/*------------------------------------------------------------------------------*\*\
	BmLogHandler
		-	implements the global log-handler that received all logging requests and
			executes them
		-	different logfiles are identified by their name and will be created on demand
\*------------------------------------------------------------------------------*/
class BmLogHandler {
	class BmLogfile;

public:
	// static functions
	static void Log( const BString logname, uint32 flag, const BString& msg, int8 minlevel=1);
	static void Log( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1);
	static void FinishLog( const BString& logname);

	// creator-func, c'tors and d'tor
	static BmLogHandler* CreateInstance( uint32 logLevels);
	BmLogHandler( uint32 logLevels);
	~BmLogHandler();

	// native methods:
	BmLogfile* FindLogfile( const BString &logname);
	void CloseLog( const BString &logname);
	void LogToFile( const BString &logname, uint32 flag, const BString &msg, int8 minlevel=1);
	void LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1);

	// setters:
	void LogLevels( uint32 loglevels)	{ mLoglevels = loglevels; }

	static BmLogHandler* theInstance;

	BStopWatch StopWatch;

private:
	//	message component definitions for status-msgs:
	static const char* const MSG_MESSAGE = 		"bm:msg";

	/*------------------------------------------------------------------------------*\*\
		BmLogfile
			-	implements a single logfile
			-	the actual logging takes place in here
	\*------------------------------------------------------------------------------*/
	class BmLogfile : public BLooper{
		typedef BLooper inherited;
	public:
		BmLogfile( const BString &fn);
		~BmLogfile();
		void Write( const char* const msg);
		void MessageReceived( BMessage* msg);
	
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
extern const uint32 BM_LogPop;
extern const uint32 BM_LogJobWin;
extern const uint32 BM_LogMailParse;
extern const uint32 BM_LogUtil;
extern const uint32 BM_LogMailTracking;
extern const uint32 BM_LogFolderView;
extern const uint32 BM_LogRefView;
extern const uint32 BM_LogMainWindow;
extern const uint32 BM_LogModelController;
extern const uint32 BM_LogMailEditWin;
extern const uint32 BM_LogSmtp;
extern const uint32 BM_LogAll;

// macros to convert the loglevel for a specific flag 
// into it's internal bit-representation:
#define BM_LOGLVL0(flag) (0)
#define BM_LOGLVL1(flag) (flag)
#define BM_LOGLVL2(flag) (flag<<16)
#define BM_LOGLVL3(flag) (flag+(flag<<16))

// the macros used for logging:
#ifdef BM_LOGGING

#define BM_LOG(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg)
#define BM_LOG2(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg, 2)
#define BM_LOG3(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg, 3)
#define BM_LOGERR(msg) \
	BmLogHandler::Log( "Errors", BM_LogAll, msg, 0)
#define BM_LOG_FINISH(name) BmLogHandler::FinishLog( name)
#define BM_LOGNAME "Beam"
#define BM_SHOWERR(msg) \
	{	BmLogHandler::Log( BM_LOGNAME, BM_LogAll, msg, 0); \
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

#define TheLogHandler BmLogHandler::theInstance

#endif
