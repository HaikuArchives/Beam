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
#include <MessageQueue.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
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
const uint32 BM_LogFilter				= 1UL<<12;
// dummy constant meaning to log everything:
const uint32 BM_LogAll  				= 0xffffffff;


BmLogHandler* BmLogHandler::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const BmString logname, uint32 flag, const BmString& msg, int8 minlevel) { 
	if (theInstance)
		theInstance->LogToFile( logname, flag, msg, minlevel);
}

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const char* const logname, uint32 flag, const char* const msg, int8 minlevel) { 
	BmString theLogname(logname);
	BmString theMsg(msg);
	if (theInstance)
		theInstance->LogToFile( theLogname, flag, theMsg, minlevel);
}

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::FinishLog( const BmString& logname) { 
	if (theInstance)
		theInstance->CloseLog( logname);
}

/*------------------------------------------------------------------------------*\
	static creator-function
		-	singleton creator
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
	FindLogfile( logname)
		-	tries to find the logfile of the given name in the logfile-map
		-	if logfile does not exist yet, it is created and added to map
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile* BmLogHandler::FindLogfile( const BmString &logname) {
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		throw BM_runtime_error("LogToFile(): Unable to get lock on loghandler");
	BmString name = logname.Length() ? logname : BmString("Beam");
	name = BmString("logs/") + name + ".log";
	LogfileMap::iterator logIter = mActiveLogs.find( name);
	BmLogfile* log;
	if (logIter == mActiveLogs.end()) {
		// logfile doesn't exists, so we create it:
		mAppFolder->CreateDirectory( "logs", NULL);
						// ensure that the logs-folder exists
		BFile* logfile = new BFile( mAppFolder, name.String(),
											 B_READ_WRITE|B_CREATE_FILE|B_OPEN_AT_END);
		if (logfile->InitCheck() != B_OK)
			throw BM_runtime_error( BmString("Unable to open logfile ") << name);
		// shrink logfile if it has become too large:
		off_t maxSize = ThePrefs->GetInt( "MaxLogfileSize", 200*1024);
		if (logfile->Position() > maxSize) {
			off_t newSize = ThePrefs->GetInt( "MinLogfileSize", 50*1024);
			char* buf = new char [newSize+1];
			newSize = logfile->ReadAt( logfile->Position()-newSize, buf, newSize);
			buf[newSize] = '\0';
			int32 offs = 0;
			char* pos = strchr( buf, '\n');
			if (pos != NULL)
				offs = 1+pos-buf;
			logfile->SetSize( 0);
			logfile->Seek( SEEK_SET, 0);
			logfile->WriteAt( 0, buf+offs, newSize-offs);
			delete [] buf;
		}
		log = new BmLogfile( logfile, name.String());
		mActiveLogs[name] = log;
	} else {
		log = (*logIter).second;
	}
	return log;
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	writes msg into the logfile specified by logname
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const BmString& logname, uint32 flag,
										const BmString& msg, int8 minlevel) {
	BmLogfile* log = FindLogfile( logname);
	if (log) {
		int8 loglevel = BM_LOGLVL_FOR(mLoglevels, flag);
		if (loglevel < minlevel)
			return;								// loglevel indicates to ignore this message
		BMessage mess( BM_LOG_MSG);
		mess.AddString( BmLogfile::MSG_MESSAGE, msg.String());
		mess.AddInt32( BmLogfile::MSG_THREAD_ID, find_thread(NULL));
		log->PostMessage( &mess);
	}
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	writes msg into the logfile that is named logname
		-	if no logfile of given name exists, it is created
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel) { 
	LogToFile( BmString(logname), flag, BmString(msg), minlevel);
}

/*------------------------------------------------------------------------------*\
	CloseLog( logname)
		-	closes the logfile with the specified logname
\*------------------------------------------------------------------------------*/
void BmLogHandler::CloseLog( const BmString &logname) {
	BAutolock lock( mLocker);
	if (lock.IsLocked()) {
		BmString name = logname.Length() ? logname : BmString("Beam");
		name = BmString("logs/") + name + ".log";
		LogfileMap::iterator logIter = mActiveLogs.find( name);
		BmLogfile* log;
		if (logIter != mActiveLogs.end()) {
			log = (*logIter).second;
			mActiveLogs.erase( logIter);
			// wait until the logfile has no more entries to process...
			BMessageQueue* msgQueue = log->MessageQueue();
			bool done = false;
			while( msgQueue && !done) {
				msgQueue->Lock();
				done = msgQueue->IsEmpty();
				msgQueue->Unlock();
				if (!done)
					snooze( 100*1000);
			}
			// ...ok, we can quit the logfile:
			log->Lock();
			log->Quit();
		}
	}
}

const char* const BmLogHandler::BmLogfile::MSG_MESSAGE = 	"bm:msg";
const char* const BmLogHandler::BmLogfile::MSG_THREAD_ID =	"bm:tid";

/*------------------------------------------------------------------------------*\
	BmLogfile()
		-	c'tor
		-	starts Looper's message-loop
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::BmLogfile( BFile* file, const char* fn)
	:	BLooper( (BmString("log_")<<fn).String(), B_DISPLAY_PRIORITY, 500)
	,	mLogFile( file)
	,	filename( fn)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	~BmLogfile()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::~BmLogfile() {
	delete mLogFile;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles log-requests to this logfile
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
	BmString s(msg);
	s.ReplaceAll( "\r", "<CR>");
	s.ReplaceAll( "\n\n", "\n");
	s.ReplaceAll( "\n", "\n                                  ");
	s << "\n";
	bigtime_t rtNow = real_time_clock_usecs();
	time_t now = rtNow/1000000;
	int32 nowMSecs = (rtNow/1000)%1000;
	char buf[40];
	sprintf( buf, "<%6ld|%s.%03ld>: ", 
					  threadId, 
					  TimeToString( now, "%Y-%m-%d|%H:%M:%S").String(),
					  nowMSecs);
	ssize_t result;
	if ((result = mLogFile->Write( buf, strlen( buf))) < 0)
		throw BM_runtime_error( BmString("Unable to write to logfile ") << filename);
	if ((result = mLogFile->Write( s.String(), s.Length())) < 0)
		throw BM_runtime_error( BmString("Unable to write to logfile ") << filename);
	mLogFile->Sync();
}
