/*
	BmUtil.cpp
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <Autolock.h>
#include <Directory.h>
#include <File.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
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
const uint32 BM_LogSmtp					= 1UL<<10;
const uint32 BM_LogPrefsWin			= 1UL<<11;
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
BmLogHandler* BmLogHandler::CreateInstance( uint32 logLevels, node_ref* appFolder) {
	if (theInstance)
		return theInstance;
	else
		return theInstance = new BmLogHandler( logLevels, appFolder);
}

/*------------------------------------------------------------------------------*\
	constructor
		-	initializes StopWatch()
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogHandler( uint32 logLevels, node_ref* appFolder)
	:	mLocker("beam_loghandler")
	,	StopWatch( "Beam_watch", true)
	,	mLoglevels( logLevels)
	,	mAppFolder( new BDirectory( appFolder))
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
	delete mAppFolder;
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	FindLogfile()
		-	
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile* BmLogHandler::FindLogfile( const BString &logname) {
	BmAutolock lock( mLocker);
	if (lock.IsLocked()) {
		BString name = logname.Length() ? logname : "Beam";
		name = BString("logs/") + name + ".log";
		LogfileMap::iterator logIter = mActiveLogs.find( name);
		BmLogfile* log;
		if (logIter == mActiveLogs.end()) {
			// logfile doesn't exists, so we create it:
			mAppFolder->CreateDirectory( "logs", NULL);
							// ensure that the logs-folder exists
			BFile* logfile = new BFile( mAppFolder, name.String(),
												 B_WRITE_ONLY|B_CREATE_FILE|B_OPEN_AT_END);
			if (logfile->InitCheck() != B_OK)
				throw BM_runtime_error( BString("Unable to open logfile ") << name);
			log = new BmLogfile( logfile, name.String());
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
		int8 loglevel = BM_LOGLVL_FOR(mLoglevels, flag);
		if (loglevel < minlevel)
			return;								// loglevel indicates to ignore this message
		BMessage mess( BM_LOG_MSG);
		mess.AddString( MSG_MESSAGE, msg);
		mess.AddInt32( MSG_THREAD_ID, find_thread(NULL));
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
	constructor
		- standard
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::BmLogfile( BFile* file, const char* fn)
	:	BLooper( (BString("log_")<<fn).String(), B_DISPLAY_PRIORITY, 500)
	,	mLogFile( file)
	,	filename( fn)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	destructor
		- standard
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::~BmLogfile() {
	delete mLogFile;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmLogHandler::BmLogfile::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_LOG_MSG: {
			Write( msg->FindString( MSG_MESSAGE), msg->FindInt32( MSG_THREAD_ID));
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
void BmLogHandler::BmLogfile::Write( const char* const msg, int32 threadId) {
	BString s(msg);
	s.ReplaceAll( "\r", "<CR>");
	s.ReplaceAll( "\n\n", "\n");
	s.ReplaceAll( "\n", "\n                       ");
	s << "\n";
	static char buf[40];
	sprintf( buf, "<%6ld|%012Ld>: ", threadId, TheLogHandler->StopWatch.ElapsedTime());
	ssize_t result;
	if ((result = mLogFile->Write( buf, strlen( buf))) < 0)
		throw BM_runtime_error( BString("Unable to write to logfile ") << filename);
	if ((result = mLogFile->Write( s.String(), s.Length())) < 0)
		throw BM_runtime_error( BString("Unable to write to logfile ") << filename);
	mLogFile->Sync();
}
