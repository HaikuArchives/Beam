/*
	BmLogHandler.h
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


#ifndef _BmLogHandler_h
#define _BmLogHandler_h

#include <map>
#include <stdio.h>

#include <Locker.h>
#include <Looper.h>
#include <StopWatch.h>

#include "BmBasics.h"
#include "BmString.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmLogfile:
\*------------------------------------------------------------------------------*/
#define BM_LOG_MSG						'bmia'

class BDirectory;
class BFile;
struct node_ref;
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
	static void Log( const BmString logname, uint32 flag, const BmString& msg, int8 minlevel=1);
	static void Log( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1);
	static void FinishLog( const BmString& logname);

	// creator-func, c'tors and d'tor
	static BmLogHandler* CreateInstance( uint32 logLevels, node_ref* appFolder);
	BmLogHandler( uint32 logLevels, node_ref* appFolder);
	~BmLogHandler();

	// native methods:
	BmLogfile* FindLogfile( const BmString &logname);
	void CloseLog( const BmString &logname);
	void LogToFile( const BmString &logname, uint32 flag, const BmString &msg, int8 minlevel=1);
	void LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1);

	// setters:
	inline void LogLevels( uint32 loglevels)	{ mLoglevels = loglevels; }

	static BmLogHandler* theInstance;

	BStopWatch StopWatch;

	bool mWaitingForShutdown;

private:
	// Hide copy-constructor and assignment:
	BmLogHandler( const BmLogHandler&);
	BmLogHandler operator=( const BmLogHandler&);
	
	/*------------------------------------------------------------------------------*\*\
		BmLogfile
			-	implements a single logfile
			-	the actual logging takes place in here
	\*------------------------------------------------------------------------------*/
	class BmLogfile : public BLooper{
		typedef BLooper inherited;
	public:
		BmLogfile( BFile* file, const char* fn);
		~BmLogfile();
		void Write( const char* const msg, const int32 threadId);
		void MessageReceived( BMessage* msg);
	
		//	message component definitions for status-msgs:
		static const char* const MSG_MESSAGE;
		static const char* const MSG_THREAD_ID;

	private:
		BFile* mLogFile;
		BmString filename;

		// Hide copy-constructor and assignment:
		BmLogfile( const BmLogfile&);
		BmLogfile operator=( const BmLogfile&);
	};

	typedef map<const BmString, BmLogfile*> LogfileMap;
	LogfileMap mActiveLogs;					// map of names to logfiles
	BLocker mLocker;							// benaphore used to lock write-access to map
	uint32 mLoglevels;
	BDirectory* mAppFolder;
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
extern const uint32 BM_LogPrefsWin;
extern const uint32 BM_LogFilter;
extern const uint32 BM_LogAll;

// macros to convert the loglevel for a specific flag 
// into it's internal bit-representation:
#define BM_LOGLVL0(flag) (0)
#define BM_LOGLVL1(flag) (flag)
#define BM_LOGLVL2(flag) (flag<<16)
#define BM_LOGLVL3(flag) (flag+(flag<<16))

// macro to obtain the loglevel for a specific flag 
// from it's internal bit-representation:
#define BM_LOGLVL_FOR(loglevels,flag) \
(((loglevels & flag) ? 1 : 0) + ((loglevels & flag<<16) ? 2 : 0))

// macro to bit-encode a single loglevel for the
// given flag:
#define BM_LOGLVL_VAL(loglevel,flag) \
(((loglevel & 1) ? flag : 0) + ((loglevel & 2) ? flag<<16 : 0))

// the macros used for logging:
#ifdef BM_LOGGING

#define BM_LOG(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg)
#define BM_LOG2(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg, 2)
#define BM_LOG3(flag,msg) \
	BmLogHandler::Log( BM_LOGNAME, flag, msg, 3)
#define BM_LOGERR(msg) \
	{ BmLogHandler::Log( BM_LOGNAME, BM_LogAll, msg, 0); \
	  BmLogHandler::Log( "Errors", BM_LogAll, msg, 0);  }
#define BM_LOG_FINISH(name) BmLogHandler::FinishLog( name)
#define BM_LOGNAME "Beam"
#define BM_SHOWERR(msg) \
	{	BM_LOGERR(msg); \
		ShowAlert( msg);	}

#else
	// error-only logging:
#define BM_LOG(flag,msg)
#define BM_LOG2(flag,msg)
#define BM_LOG3(flag,msg)
#define BM_LOG_FINISH(name) BmLogHandler::FinishLog( name)
#define BM_LOGNAME "Beam"
#define BM_LOGERR(msg) \
	{ BmLogHandler::Log( BM_LOGNAME, BM_LogAll, msg, 0); \
	  BmLogHandler::Log( "Errors", BM_LogAll, msg, 0);  }
#define BM_SHOWERR(msg) \
	{	BM_LOGERR(msg); \
		ShowAlert( msg);	}

#endif

#define TheLogHandler BmLogHandler::theInstance

#endif
