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
#include <Messenger.h>

#ifdef __POWERPC__
#define BM_BUILDING_BMBASE 1
#endif

#include "BmBasics.h"
#include "BmLogHandler.h"

/*------------------------------------------------------------------------------*\*\
	ShowAlert( text)
		-	pops up an Alert showing the passed (error-)text
\*------------------------------------------------------------------------------*/
void ShowAlert( const BmString &text) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}

/*------------------------------------------------------------------------------*\*\
	ShowAlertWithType( text, type)
		-	pops up an Alert of given type, showing the passed text
\*------------------------------------------------------------------------------*/
void ShowAlertWithType( const BmString &text, alert_type type) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, type);
	alert->Go();
}

/*------------------------------------------------------------------------------*\
	TimeToString( time)
		-	converts the given time into a string
\*------------------------------------------------------------------------------*/
BmString TimeToString( time_t utc, const char* format) {
	BmString s;
	const int32 bufsize=100;
	s.SetTo( '\0', bufsize);
	char* buf=s.LockBuffer( bufsize);
	struct tm ltm;
	localtime_r( &utc, &ltm);
	strftime( buf, bufsize, format, &ltm);
	s.UnlockBuffer( strlen(buf));
	return s;
}

/*------------------------------------------------------------------------------*\
	the different "terrains" we will be logging, each of them
	has its own loglevel.
	N.B.: the maximum number is 16, because we need two bits per terrain!
\*------------------------------------------------------------------------------*/
const uint32 BM_LogPop  				= 1UL<<0;
const uint32 BM_LogJobWin 				= 1UL<<1;
const uint32 BM_LogMailParse 			= 1UL<<2;
const uint32 BM_LogApp		 			= 1UL<<3;
const uint32 BM_LogMailTracking		= 1UL<<4;
const uint32 BM_LogGui					= 1UL<<5;
const uint32 BM_LogModelController	= 1UL<<6;
const uint32 BM_LogSmtp					= 1UL<<7;
const uint32 BM_LogFilter				= 1UL<<8;
const uint32 BM_LogRefCount			= 1UL<<9;
// dummy constant meaning to log everything:
const uint32 BM_LogAll  				= 0xffffffff;


BmLogHandler* TheLogHandler = NULL;

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const BmString logname, const BmString& msg) { 
	if (TheLogHandler)
		TheLogHandler->LogToFile( logname, msg.String());
}

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::Log( const char* const logname, const char* msg) { 
	BmString theLogname(logname);
	if (TheLogHandler)
		TheLogHandler->LogToFile( theLogname, msg);
}

/*------------------------------------------------------------------------------*\
	static logging-function
		-	logs only if a loghandler is actually present
\*------------------------------------------------------------------------------*/
void BmLogHandler::FinishLog( const BmString& logname) { 
	if (TheLogHandler)
		TheLogHandler->CloseLog( logname);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmLogHandler::StartWatchingLogfile( BHandler* handler, const char* logfileName) {
	mLocker.Lock();
	BmLogfile* log = LogfileFor( logfileName);
	if (log && !log->mWatchingHandlers.HasItem( handler))
		log->mWatchingHandlers.AddItem( handler);
	mLocker.Unlock();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmLogHandler::StopWatchingLogfile( BHandler* handler, const char* logfileName) {
	mLocker.Lock();
	BmLogfile* log = LogfileFor( logfileName);
	if (log)
		log->mWatchingHandlers.RemoveItem( handler);
	mLocker.Unlock();
}

/*------------------------------------------------------------------------------*\
	static creator-function
		-	singleton creator
\*------------------------------------------------------------------------------*/
BmLogHandler* BmLogHandler::CreateInstance( uint32 logLevels, node_ref* appFolder) {
	if (TheLogHandler)
		return TheLogHandler;
	else
		return TheLogHandler = new BmLogHandler( logLevels, appFolder);
}

/*------------------------------------------------------------------------------*\
	constructor
		-	initializes StopWatch()
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogHandler( uint32 logLevels, node_ref* appFolder)
	:	StopWatch( "Beam_watch", true)
	,	mLocker("beam_loghandler")
	,	mLoglevels( logLevels)
	,	mAppFolder( new BDirectory( appFolder))
{
}

/*------------------------------------------------------------------------------*\
	destructor
		-	frees each and every log-file
\*------------------------------------------------------------------------------*/
BmLogHandler::~BmLogHandler() {
	int32 count = mActiveLogs.CountItems();
	for( int i=0; i<count; ++i) {
		BLooper* looper = static_cast< BLooper*>( mActiveLogs.ItemAt(i));
		looper->LockLooper();
		looper->Quit();
	}
	mActiveLogs.MakeEmpty();
	delete mAppFolder;
	TheLogHandler = NULL;
}

/*------------------------------------------------------------------------------*\
	LogLevels
		-	
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogLevels( uint32 loglevels, int32 minFileSize, int32 maxFileSize) {
	mLoglevels = loglevels;
	mMinFileSize = minFileSize;
	mMaxFileSize = maxFileSize;
}

/*------------------------------------------------------------------------------*\
	LogfileFor( logname)
		-	tries to find the logfile of the given name in the logfile-map
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile* BmLogHandler::LogfileFor( const BmString &logname) {
	BmLogfile* log = NULL;
	int32 count = mActiveLogs.CountItems();
	for( int i=0; i<count; ++i) {
		log = static_cast< BmLogfile*>( mActiveLogs.ItemAt(i));
		if (log && log->logname == logname)
			return log;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindLogfile( logname)
		-	tries to find the logfile of the given name in the logfile-map
		-	if logfile does not exist yet, it is created and added to map
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile* BmLogHandler::FindLogfile( const BmString &ln) {
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		throw BM_runtime_error("LogToFile(): Unable to get lock on loghandler");
	BmString logname = ln.Length() ? ln : BmString("Beam");
	BmString name = BmString("logs/") + logname + ".log";
	BmLogfile* log = LogfileFor( logname);
	if (!log) {
		// logfile doesn't exists, so we create it:
		mAppFolder->CreateDirectory( "logs", NULL);
						// ensure that the logs-folder exists
		BFile* logfile = new BFile( mAppFolder, name.String(),
											 B_READ_WRITE|B_CREATE_FILE|B_OPEN_AT_END);
		if (logfile->InitCheck() != B_OK)
			throw BM_runtime_error( BmString("Unable to open logfile ") << name);
		// shrink logfile if it has become too large:
		if (logfile->Position() > mMaxFileSize) {
			off_t newSize = mMinFileSize;
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
		log = new BmLogfile( logfile, name.String(), logname.String());
		mActiveLogs.AddItem( log);
	}
	return log;
}

/*------------------------------------------------------------------------------*\
	CheckLogLevel( flag)
		-	returns whether or not the loglevel for the given flag is at least 
		   minlevel
\*------------------------------------------------------------------------------*/
bool BmLogHandler::CheckLogLevel( uint32 flag, int8 minlevel) const {
	int8 loglevel = BM_LOGLVL_FOR(mLoglevels, flag);
	return loglevel >= minlevel;
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	dispatches msg to corrsponding logfile
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const BmString& logname, const BmString& msg) {
	LogToFile( logname, msg.String());
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	dispatches msg to corrsponding logfile
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const BmString& logname, const char* msg) { 
	BmLogfile* log = FindLogfile( logname);
	if (log) {
		BMessage mess( BM_LOG_MSG);
		mess.AddString( MSG_MESSAGE, msg);
		mess.AddInt32( MSG_THREAD_ID, find_thread(NULL));
		log->PostMessage( &mess);
	}
}

/*------------------------------------------------------------------------------*\
	CloseLog( logname)
		-	closes the logfile with the specified logname
\*------------------------------------------------------------------------------*/
void BmLogHandler::CloseLog( const BmString &logname) {
	BAutolock lock( mLocker);
	if (lock.IsLocked()) {
		BmLogfile* log = LogfileFor( logname);
		if (log) {
			mActiveLogs.RemoveItem( log);
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

const char* const BmLogHandler::MSG_MESSAGE = 	"bm:msg";
const char* const BmLogHandler::MSG_THREAD_ID =	"bm:tid";

/*------------------------------------------------------------------------------*\
	BmLogfile()
		-	c'tor
		-	starts Looper's message-loop
\*------------------------------------------------------------------------------*/
BmLogHandler::BmLogfile::BmLogfile( BFile* file, const char* fn, const char* ln)
	:	BLooper( (BmString("log_")<<ln).String(), B_DISPLAY_PRIORITY, 500)
	,	logname( ln)
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
	s.Prepend( buf);
	ssize_t result;
	if ((result = mLogFile->Write( s.String(), s.Length())) < 0)
		throw BM_runtime_error( BmString("Unable to write to logfile ") << filename);
	int32 watcherCount = mWatchingHandlers.CountItems();
	if (watcherCount>0) {
		BMessage msg( BM_LOG_MSG);
		msg.AddString( MSG_MESSAGE, s.String());
		for( int32 i=0; i<watcherCount; ++i) {
			BMessenger watcher( static_cast< BHandler*>( mWatchingHandlers.ItemAt(i)));
			watcher.SendMessage( &msg);
		}
	}
	mLogFile->Sync();
}
