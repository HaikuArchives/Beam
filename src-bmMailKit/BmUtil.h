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
#include <String.h>

#include <libbenaphore/benaphore.h>

/*------------------------------------------------------------------------------*\*\
	BMruntime_error
		-	exception to indicate a general runtime error
\*------------------------------------------------------------------------------*/
class BM_runtime_error : public runtime_error {
	typedef runtime_error inherited;
public:
	BM_runtime_error (const BString& what_arg): inherited (what_arg.String()) { }
	BM_runtime_error (const char *const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BMinvalid_argument
		-	exception to indicate an invalid-argument error
\*------------------------------------------------------------------------------*/
class BM_invalid_argument : public invalid_argument {
	typedef invalid_argument inherited;
public:
	BM_invalid_argument (const BString& what_arg): inherited (what_arg.String()) { }
	BM_invalid_argument (const char *const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BMnetwork_error
		-	exception to indicate an error during network communication
\*------------------------------------------------------------------------------*/
class BM_network_error : public BM_runtime_error {
	typedef BM_runtime_error inherited;
public:
	BM_network_error (const BString& what_arg): inherited (what_arg.String()) { }
	BM_network_error (const char *const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BM_THROW_...
		-	throws exception of specific type
\*------------------------------------------------------------------------------*/
inline bool BM_THROW_RUNTIME( const BString &s) { throw BM_runtime_error(s); }
inline bool BM_THROW_INVALID( const BString &s) { throw BM_invalid_argument(s); }
inline bool BM_THROW_NETWORK( const BString &s) { throw BM_network_error(s); }

/*------------------------------------------------------------------------------*\*\
	FindMsgXXX( archive, name)
		-	functions that extract the msg-field of a specified name from the given 
			archive and return it.
\*------------------------------------------------------------------------------*/
const char *FindMsgString( BMessage* archive, const char* name);
bool FindMsgBool( BMessage* archive, const char* name);
int64 FindMsgInt64( BMessage* archive, const char* name);
int32 FindMsgInt32( BMessage* archive, const char* name);
int16 FindMsgInt16( BMessage* archive, const char* name);
float FindMsgFloat( BMessage* archive, const char* name);

/*------------------------------------------------------------------------------*\*\
	ShowAlert( text, logtext)
		-	pops up an Alert showing the passed text
		-	logs text unless logtext is specified, in which case that is 
			written to the logfile
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text, const BString logtext="");

/*------------------------------------------------------------------------------*\*\
	BmLogHandler
		-	implements the global log-handler that received all logging requests and
			executes them
		-	different logfiles are identified by their name and will be created on demand
\*------------------------------------------------------------------------------*/
class BmLogHandler {
	/*------------------------------------------------------------------------------*\*\
		BmLogfile
			-	implements a single logfile
			-	the actual logging takes place in here
	\*------------------------------------------------------------------------------*/
	class BmLogfile {
	public:
		BmLogfile( const BString &fn);
		~BmLogfile() 							{ if (logfile) fclose(logfile); }
		void Write( const char* const msg, uint32 flag, int8 minlevel);
	
		static BString LogPath;
	
	private:
		FILE* logfile;
		BString filename;
		uint32 loglevels;
	};

	typedef map<const BString, BmLogfile*> LogfileMap;
	LogfileMap mActiveLogs;					// map of names to logfiles
	Benaphore mBenaph;						// benaphore used to lock write-access to map

public:
	void CloseLog( const BString &logname);

	void LogToFile( const BString &logname, uint32 flag, const BString &msg, int8 minlevel=1);
	void LogToFile( const char* const logname, uint32 flag, const char* const msg, int8 minlevel=1)
							{ LogToFile( BString(logname), flag, BString(msg), minlevel); }

	BmLogHandler() : mBenaph("beam_loghandler") { }
	~BmLogHandler();
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
	Beam::LogHandler->LogToFile( BM_LOGNAME, flag, msg)
#define BM_LOG2(flag,msg) \
	Beam::LogHandler->LogToFile( BM_LOGNAME, flag, msg, 2)
#define BM_LOG3(flag,msg) \
	Beam::LogHandler->LogToFile( BM_LOGNAME, flag, msg, 3)
#define BM_LOGERR(msg) \
	Beam::LogHandler->LogToFile( "Errors", BM_LogAll, msg, 0)
#define BM_LOG_FINISH(name) Beam::LogHandler->CloseLog( name)
#define BM_LOGNAME "Beam"

#else

#define BM_LOG(flag,msg)
#define BM_LOG2(flag,msg)
#define BM_LOG3(flag,msg)
#define BM_LOGERR(msg)
#define BM_LOG_FINISH(name)
#define BM_LOGNAME

#endif

/*------------------------------------------------------------------------------*\*\
	utility function to format a number of bytes into a string
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes);

#endif
