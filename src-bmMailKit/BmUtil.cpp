/*
	BmUtil.cpp
		$Id$
*/

#include <stdio.h>

#include "BmUtil.h"

//---------------------------------------------------
namespace Beam {
	BmLogHandler LogHandler;
};

//---------------------------------------------------
const char *FindMsgString( BMessage* archive, char* name) {
	const char *str;
	assert(archive && name);
	if (archive->FindString( name, &str) == B_OK) {
		return str;
	} else {
		string s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s);
	}
}

//---------------------------------------------------
bool FindMsgBool( BMessage* archive, char* name) {
	bool b;
	assert(archive && name);
	if (archive->FindBool( name, &b) == B_OK) {
		return b;
	} else {
		string s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s);
	}
}

//---------------------------------------------------
int32 FindMsgInt32( BMessage* archive, char* name) {
	int32 i;
	assert(archive && name);
	if (archive->FindInt32( name, &i) == B_OK) {
		return i;
	} else {
		string s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s);
	}
}

//---------------------------------------------------
int16 FindMsgInt16( BMessage* archive, char* name) {
	int16 i;
	assert(archive && name);
	if (archive->FindInt16( name, &i) == B_OK) {
		return i;
	} else {
		string s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s);
	}
}

//---------------------------------------------------
float FindMsgFloat( BMessage* archive, char* name) {
	float f;
	assert(archive && name);
	if (archive->FindFloat( name, &f) == B_OK) {
		return f;
	} else {
		string s( "unknown message-field: ");
		s += name;
		throw invalid_argument( s);
	}
}

//---------------------------------------------------
BmLogHandler::~BmLogHandler() {
	for(  LogfileMap::iterator logIter = mActiveLogs.begin();
			logIter != mActiveLogs.end();
			++logIter) {
		delete (*logIter).second;
	}
}

//---------------------------------------------------
void BmLogHandler::LogToFile( const char* const logname, const char* const msg) {
	LogfileMap::iterator logIter = mActiveLogs.find( logname);
	BmLogfile *log;
	if (logIter == mActiveLogs.end()) {
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

//---------------------------------------------------
BString BmLogHandler::BmLogfile::LogPath = "/boot/home/Sources/beam/logs/";

//---------------------------------------------------
void BmLogHandler::BmLogfile::Write( const char* const msg) {
	if (logfile == NULL) {
		BString fn = BString(LogPath) << filename;
		if (fn.FindFirst(".log") == B_ERROR) {
			fn += ".log";
		}
		if (!(logfile = fopen( fn.String(), "a"))) {
			BString s = BString("Unable to open logfile ") << fn;
			throw runtime_error( s.String());
		}
		fprintf( logfile, "------------------------------\nSession Started\n------------------------------\n");
	}
	BString s(msg);
	s.ReplaceAll("\r","<CR>");
	s.ReplaceAll("\n","\n                ");
	fprintf( logfile, "<%012Ld>: %s\n", watch.ElapsedTime(), s.String());
}

