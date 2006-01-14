/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmLogHandler_h
#define _BmLogHandler_h

#include <stdio.h>

#include <Alert.h>
#include <Directory.h>
#include <List.h>
#include <Locker.h>
#include <Looper.h>
#include <StopWatch.h>

#include "BmBase.h"
#include "BmString.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmLogfile:
\*------------------------------------------------------------------------------*/
#define BM_LOG_MSG						'bmia'

class BDirectory;
class BFile;
struct node_ref;
/*------------------------------------------------------------------------------*\
	BmLogHandler
		-	implements the global log-handler that received all logging requests 
			and executes them
		-	different logfiles are identified by their name and will be created 
			on demand
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmLogHandler {

	class BmLogfile;

	struct BmWatcherInfo {
		BmString logname;
		BList watchingHandlers;
		BmWatcherInfo( const BmString& ln, BHandler* h)
			:	logname( ln) 					{ watchingHandlers.AddItem( h); }
	};

public:
	// static functions
	static void Log( const BmString logname, const BmString& msg);
	static void Log( const char* const logname, const char* msg);
	static void Shutdown( bool sync=true);
	static void FinishLog( const BmString& logname);

	// creator-func, c'tors and d'tor
	static BmLogHandler* CreateInstance( uint32 logLevels, node_ref* appFolder);
	BmLogHandler( uint32 logLevels, node_ref* appFolder);
	~BmLogHandler();

	// native methods:
	BmLogfile* FindLogfile( const BmString &logname);
	void CloseAllLogs();
	void CloseLog( const BmString &logname);
	void LogToFile( const BmString& logname, const BmString &msg);
	void LogToFile( const BmString& logname, const char* msg);
	//
	bool CheckLogLevel( uint32 terrain, int8 minlevel) const;

	void StartWatchingLogfile( BHandler* looper, const char* logfileName);
	void StopWatchingLogfile( BHandler* looper, const char* logfileName);

	// getters:
	bool ShowErrorsOnScreen()				{ return mShowErrorsOnScreen; }

	// setters:
	void LogLevels( uint32 loglevels, int32 minFileSize, int32 maxFileSize);
	void ShowErrorsOnScreen( bool b)		{ mShowErrorsOnScreen = b; }

	BStopWatch StopWatch;

	bool mWaitingForShutdown;

	BLocker mLocker;
							// benaphore used to lock write-access to list

	//	message component definitions for status-msgs:
	static const char* const MSG_MESSAGE;
	static const char* const MSG_THREAD_ID;

private:
	BmLogfile* LogfileFor( const BmString &logname);
	BmWatcherInfo* WatcherInfoFor( const BmString &logname);

	// Hide copy-constructor and assignment:
	BmLogHandler( const BmLogHandler&);
	BmLogHandler operator=( const BmLogHandler&);
	
	/*---------------------------------------------------------------------------*\
		BmLogfile
			-	implements a single logfile
			-	the actual logging takes place in here
	\*---------------------------------------------------------------------------*/
	class IMPEXPBMBASE BmLogfile : public BLooper{
		typedef BLooper inherited;
		friend BmLogHandler;
	public:
		BmLogfile( BFile* file, const char* fn, const char* ln);
		~BmLogfile();
		void Write( const char* const msg, const int32 threadId);
		void MessageReceived( BMessage* msg);
	
		BList mWatchingHandlers;
		BmString logname;

	private:
		BFile* mLogFile;
		BmString filename;

		// Hide copy-constructor and assignment:
		BmLogfile( const BmLogfile&);
		BmLogfile operator=( const BmLogfile&);
	};

	BList mActiveLogs;					
							// list of logfiles
	BList mWatcherInfo;

	uint32 mLoglevels;
	BDirectory mLogFolder;
	int32 mMinFileSize;
	int32 mMaxFileSize;
	bool mShowErrorsOnScreen;
};

/*------------------------------------------------------------------------------*\
	macros, constants and defines that facilitate logging-functionality:
\*------------------------------------------------------------------------------*/

// the different "terrains" we will be logging, each of them
// has its own loglevel:
extern IMPEXPBMBASE const uint32 BM_LogPop;
extern IMPEXPBMBASE const uint32 BM_LogJobWin;
extern IMPEXPBMBASE const uint32 BM_LogMailParse;
extern IMPEXPBMBASE const uint32 BM_LogApp;
extern IMPEXPBMBASE const uint32 BM_LogMailTracking;
extern IMPEXPBMBASE const uint32 BM_LogGui;
extern IMPEXPBMBASE const uint32 BM_LogModelController;
extern IMPEXPBMBASE const uint32 BM_LogSmtp;
extern IMPEXPBMBASE const uint32 BM_LogFilter;
extern IMPEXPBMBASE const uint32 BM_LogRefCount;
extern IMPEXPBMBASE const uint32 BM_LogAll;

// macros to convert the loglevel for a specific terrain 
// into it's internal bit-representation:
#define BM_LOGLVL0(terrain) (0)
#define BM_LOGLVL1(terrain) (terrain)
#define BM_LOGLVL2(terrain) (terrain<<16)
#define BM_LOGLVL3(terrain) (terrain+(terrain<<16))

// macro to obtain the loglevel for a specific terrain 
// from it's internal bit-representation:
#define BM_LOGLVL_FOR(loglevels,terrain) \
(((loglevels & terrain) ? 1 : 0) + ((loglevels & terrain<<16) ? 2 : 0))

// macro to bit-encode a single loglevel for the
// given terrain:
#define BM_LOGLVL_VAL(loglevel,terrain) \
(((loglevel & 1) ? terrain : 0) + ((loglevel & 2) ? terrain<<16 : 0))

/*------------------------------------------------------------------------------*\
	time-related utility functions
\*------------------------------------------------------------------------------*/
IMPEXPBMBASE BmString TimeToString( time_t t, 
												const char* format="%Y-%m-%d %H:%M:%S");

/*------------------------------------------------------------------------------*\
	ShowAlert( text)
		-	pops up an Alert showing the passed text
\*------------------------------------------------------------------------------*/
IMPEXPBMBASE void ShowAlert( const BmString &text);

/*------------------------------------------------------------------------------*\
	ShowAlertWithType( text, type)
		-	pops up an Alert of given type, showing the passed text
\*------------------------------------------------------------------------------*/
IMPEXPBMBASE void ShowAlertWithType( const BmString &text, alert_type type);

// the macros used for logging:
#define BM_LOG(terrain,msg) \
	do {	\
		if (TheLogHandler && TheLogHandler->CheckLogLevel( terrain, 1)) \
			BmLogHandler::Log( BM_LOGNAME, msg); \
	} while(0)
#define BM_LOG2(terrain,msg) \
	do {	\
		if (TheLogHandler && TheLogHandler->CheckLogLevel( terrain, 2)) \
			BmLogHandler::Log( BM_LOGNAME, msg); \
	} while(0)
#define BM_LOG3(terrain,msg) \
	do {	\
		if (TheLogHandler && TheLogHandler->CheckLogLevel( terrain, 3)) \
			BmLogHandler::Log( BM_LOGNAME, msg); \
	} while(0)
#define BM_LOGERR(msg) \
	do {	\
		if (TheLogHandler) { \
		  BmLogHandler::Log( BM_LOGNAME, msg); \
		  BmLogHandler::Log( "Errors", msg); \
		} \
	} while(0)
#define BM_LOG_FINISH(name) BmLogHandler::FinishLog( name)
#define BM_LOGNAME "Beam"
#define BM_SHOWERR(msg) \
	do {	\
		if (TheLogHandler) { \
			BmLogHandler::Log( BM_LOGNAME, msg); \
		 	BmLogHandler::Log( "Errors", msg); \
		 	if (TheLogHandler->ShowErrorsOnScreen()) \
				ShowAlert( msg);	\
		} \
	} while(0)

extern "C" IMPEXPBMBASE BmLogHandler* TheLogHandler;

#endif
