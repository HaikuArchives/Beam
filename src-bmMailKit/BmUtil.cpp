/*
	BmUtil.cpp
		$Id$
*/

#include <iomanip>
#include <strstream>

#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	LogHandler
		-	the global object that handles all logging requests
\*------------------------------------------------------------------------------*/
namespace Beam {
	BmLogHandler* LogHandler = NULL;
};

/*------------------------------------------------------------------------------*\
	FindMsgString( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
const char *FindMsgString( BMessage* archive, char* name) {
	const char *str;
	assert(archive && name);
	if (archive->FindString( name, &str) == B_OK) {
		return str;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgBool( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
bool FindMsgBool( BMessage* archive, char* name) {
	bool b;
	assert(archive && name);
	if (archive->FindBool( name, &b) == B_OK) {
		return b;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgInt32( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int32 FindMsgInt32( BMessage* archive, char* name) {
	int32 i;
	assert(archive && name);
	if (archive->FindInt32( name, &i) == B_OK) {
		return i;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgInt16( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int16 FindMsgInt16( BMessage* archive, char* name) {
	int16 i;
	assert(archive && name);
	if (archive->FindInt16( name, &i) == B_OK) {
		return i;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgFloat( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
float FindMsgFloat( BMessage* archive, char* name) {
	float f;
	assert(archive && name);
	if (archive->FindFloat( name, &f) == B_OK) {
		return f;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	destructor
		-	frees each and every log-file
\*------------------------------------------------------------------------------*/
BmLogHandler::~BmLogHandler() {
	for(  LogfileMap::iterator logIter = mActiveLogs.begin();
			logIter != mActiveLogs.end();
			++logIter) {
		delete (*logIter).second;
	}
}

/*------------------------------------------------------------------------------*\
	LogToFile( logname, msg)
		-	writes msg into the logfile that is named logname
		-	if no logfile of given name exists, it is created
\*------------------------------------------------------------------------------*/
void BmLogHandler::LogToFile( const char* const logname, const char* const msg) {
	LogfileMap::iterator logIter = mActiveLogs.find( logname);
	BmLogfile *log;
	if (logIter == mActiveLogs.end()) {
		// logfile doesn't exists, so we create it:
		status_t res;
		while( (res = mBenaph.Lock()) != B_NO_ERROR) {
			BmLOG( BString("locking result: %ld") << res);
		}
		log = new BmLogfile( logname);
		mActiveLogs[logname] = log;
		mBenaph.Unlock();
	} else {
		log = (*logIter).second;
	}
	log->Write( msg);
}

/*------------------------------------------------------------------------------*\
	CloseLog( logname)
		-	closes the logfile with the specified by name
\*------------------------------------------------------------------------------*/
void BmLogHandler::CloseLog( const char* const logname) {
	LogfileMap::iterator logIter = mActiveLogs.find( logname);
	BmLogfile *log;
	if (logIter != mActiveLogs.end()) {
		status_t res;
		while( (res = mBenaph.Lock()) != B_NO_ERROR) {
			BmLOG( BString("locking result: %ld") << res);
		}
		log = (*logIter).second;
		mActiveLogs.erase( logname);
		delete log;
		mBenaph.Unlock();
	}
}

/*------------------------------------------------------------------------------*\
	LogPath
		-	standard-path to logfiles
		-	TODO: make this part of BmPrefs
\*------------------------------------------------------------------------------*/
BString BmLogHandler::BmLogfile::LogPath = "/boot/home/Sources/beam/logs/";

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
		if (!(logfile = fopen( fn.String(), "a"))) {
			BString s = BString("Unable to open logfile ") << fn;
			throw runtime_error( s.String());
		}
		fprintf( logfile, "<%012Ld>: %s\n", watch.ElapsedTime(), "Session Started");
	}
	BString s(msg);
	s.ReplaceAll( "\r", "<CR>");
	s.ReplaceAll( "\n", "\n                ");
	fprintf( logfile, "<%012Ld>: %s\n", watch.ElapsedTime(), s.String());
	fflush( logfile);
}

/*------------------------------------------------------------------------------*\
	BytesToString( bytes)
		-	returns the given number of bytes as a short, descriptive string:
			* bytes < 1024 				-> "X bytes"
			* 1024 < bytes < 1024000	-> "X.xx KB"
			* 1024000 < bytes 			-> "X.xx MB"
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes) {
	ostrstream ostr; 
	ostr << setiosflags( std::ios::fixed );
	if (bytes >= 1024000) {
		ostr 	<< setprecision(2) 
				<< bytes/(1024000.0) 
				<< " MB";
	} else if (bytes >= 1024) {
		ostr	<< setiosflags( std::ios::fixed )
				<< setprecision(2)
				<< bytes/1024.0
				<< " KB";
	} else {
		ostr	<< setiosflags( std::ios::fixed )
				<< setprecision(0) 
				<< bytes 
				<< " bytes";
	}
	ostr << '\0';								// terminate BString!
	return ostr.str();
}
