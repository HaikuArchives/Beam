/*
	BmUtil.cpp
		$Id$
*/

#include <Autolock.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	the different "terrains" we will be logging, each of them
	has its own loglevel:
\*------------------------------------------------------------------------------*/
const uint32 BM_LogPop  				= 1UL<<0;
const uint32 BM_LogJobWin 				= 1UL<<1;
const uint32 BM_LogMailParse 			= 1UL<<2;
const uint32 BM_LogUtil		 			= 1UL<<3;
const uint32 BM_LogMailTracking		= 1UL<<4;
const uint32 BM_LogFolderView			= 1UL<<5;
const uint32 BM_LogRefView				= 1UL<<6;
const uint32 BM_LogMainWindow			= 1UL<<7;
const uint32 BM_LogModelController	= 1UL<<8;
const uint32 BM_LogMailEditWin		= 1UL<<9;
// dummy constant meaning to log everything:
const uint32 BM_LogAll  				= 0xffffffff;


BmLogHandler* BmLogHandler::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	static logging-function
		- logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const BString logname, uint32 flag, const BString& msg, int8 minlevel=1) { 
	if (theInstance)
		theInstance->LogToFile( logname, flag, msg, minlevel);
}

/*------------------------------------------------------------------------------*\
	static logging-function
		- logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1) { 
	if (theInstance)
		theInstance->LogToFile( BString(logname), flag, BString(msg), minlevel);
}

/*------------------------------------------------------------------------------*\
	static logging-function
		- logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::FinishLog( const BString& logname) { 
	if (theInstance)
		theInstance->CloseLog( logname);
}

/*------------------------------------------------------------------------------*\
	static creator-function
		-
\*------------------------------------------------------------------------------*/
BmLogHandler* BmLogHandler::CreateInstance( uint32 logLevels) {
	if (theInstance)
		return theInstance;
	else
		return theInstance = new BmLogHandler( logLevels);
}

/*------------------------------------------------------------------------------*\
	constructor
		-	initializes StopWatch()
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogHandler( uint32 logLevels)
	:	mLocker("beam_loghandler")
	,	StopWatch( "Beam_watch", true)
	,	mLoglevels( logLevels)
{
}

/*------------------------------------------------------------------------------*\
	destructor
		-	frees each and every log-file
\*------------------------------------------------------------------------------*/
BmLogHandler::~BmLogHandler() {
	for(  LogfileMap::iterator logIter = mActiveLogs.begin();
			logIter != mActiveLogs.end();
			++logIter) {
		BLooper* looper = (*logIter).second;
		looper->LockLooper();
		looper->Quit();
	}
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	FindLogfile()
		-	
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile* BmLogHandler::FindLogfile( const BString &logname) {
	BmAutolock lock( mLocker);
	BString name = logname.Length() ? logname : "Beam";
	if (lock.IsLocked()) {
		LogfileMap::iterator logIter = mActiveLogs.find( name);
		BmLogfile* log;
		if (logIter == mActiveLogs.end()) {
			// logfile doesn't exists, so we create it:
			log = new BmLogfile( name);
			mActiveLogs[name] = log;
		} else {
			log = (*logIter).second;
		}
		return log;
	} else
		throw BM_runtime_error("LogToFile(): Unable to get lock on loghandler");
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	writes msg into the logfile that is named logname
		-	if no logfile of given name exists, it is created
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const BString& logname, uint32 flag,
										const BString& msg, int8 minlevel) {
	BmLogfile* log = FindLogfile( logname);
	if (log) {
		int8 loglevel = ((mLoglevels & flag) ? 1 : 0)
						  + ((mLoglevels & flag<<16) ? 2 : 0);
		if (loglevel < minlevel)
			return;								// loglevel indicates to ignore this message
		BMessage mess( BM_LOG_MSG);
		mess.AddString( MSG_MESSAGE, msg);
		log->PostMessage( &mess);
	}
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	writes msg into the logfile that is named logname
		-	if no logfile of given name exists, it is created
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1) { 
	LogToFile( BString(logname), flag, BString(msg), minlevel);
}

/*------------------------------------------------------------------------------*\
	CloseLog( logname)
		-	closes the logfile with the specified by name
\*------------------------------------------------------------------------------*/
void BmLogHandler::CloseLog( const BString &logname) {
	BmAutolock lock( mLocker);
	if (lock.IsLocked()) {
		LogfileMap::iterator logIter = mActiveLogs.find( logname);
		BmLogfile* log;
		if (logIter != mActiveLogs.end()) {
			log = (*logIter).second;
			mActiveLogs.erase( logname);
			log->Lock();
			log->Quit();
		}
	}
}

/*------------------------------------------------------------------------------*\
	LogPath
		-	standard-path to logfiles
		-	TODO: make this part of BmPrefs
\*------------------------------------------------------------------------------*/
BString BmLogHandler::BmLogfile::LogPath = "/boot/home/Sources/beam/logs/";

/*------------------------------------------------------------------------------*\
	constructor
		- standard
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::BmLogfile( const BString &fn)
	:	BLooper( (BString("log_")<<fn).String(), B_DISPLAY_PRIORITY, 500)
	,	logfile( NULL)
	,	filename(fn)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	destructor
		- standard
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::~BmLogfile() {
	if (logfile) fclose(logfile);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmLogHandler::BmLogfile::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_LOG_MSG: {
			Write( msg->FindString( MSG_MESSAGE));
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	Write( msg)
		-	writes given msg into log, including current timestamp
		-	log is flushed after each write
\*------------------------------------------------------------------------------*/
void BmLogHandler::BmLogfile::Write( const char* const msg) {
	if (logfile == NULL) {
		BString fn = BString(LogPath) << filename;
		if (fn.FindFirst(".log") == B_ERROR) {
			fn << ".log";
		}
		(logfile = fopen( fn.String(), "a"))
													|| BM_THROW_RUNTIME( BString("Unable to open logfile ") << fn);
	}
	BString s(msg);
	s.ReplaceAll( "\r", "<CR>");
	s.ReplaceAll( "\n\n", "\n");
	s.ReplaceAll( "\n", "\n                       ");
	fprintf( logfile, "<%6ld|%012Ld>: %s\n", find_thread(NULL), TheLogHandler->StopWatch.ElapsedTime(), s.String())>=0
													|| BM_THROW_RUNTIME( BString("Unable to write to logfile ") << filename);
	fflush( logfile);
}
