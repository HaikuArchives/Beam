/*
	BmUtil.cpp
		$Id$
*/

#include <iomanip>
#include <strstream>

#include <regexx/regexx.hh>

#include "BmUtil.h"

using namespace regexx;

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
			BmLOG( string("locking result: %ld") + iToStr(res));
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
string BmLogHandler::BmLogfile::LogPath = "/boot/home/Sources/beam/logs/";

//---------------------------------------------------
void BmLogHandler::BmLogfile::Write( const char* const msg) {
	if (logfile == NULL) {
		string fn = string(LogPath) + filename;
		if (fn.find(".log") == string::npos) {
			fn += ".log";
		}
		if (!(logfile = fopen( fn.c_str(), "a"))) {
			string s = string("Unable to open logfile ") + fn;
			throw runtime_error( s.c_str());
		}
		fprintf( logfile, "------------------------------\nSession Started\n------------------------------\n");
	}
	string s(msg);
	Regexx rx;
	rx.replace( s, "\013", "<CR>", Regexx::global);
	rx.replace( s, "\n", "\n                ", Regexx::global|Regexx::newline);
	fprintf( logfile, "<%012Ld>: %s\n", watch.ElapsedTime(), s.c_str());
}

//---------------------------------------------------
string BytesToString( int32 bytes) {
	ostrstream ostr; 
	ostr << setiosflags( std::ios::fixed );
	if (bytes >= 1024*1000) {
		ostr 	<< setprecision(2) 
				<< bytes/(1024*1000.0) 
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
	ostr << '\0';								// terminate string!
	return ostr.str();
}

//---------------------------------------------------
string iToStr( int32 i) {
	char buf[40];
	sprintf( buf, "%ld", i);
	return string( buf);
}

//---------------------------------------------------
string fToStr( double f) {
	char buf[40];
	sprintf( buf, "%.2f", f);
	return string( buf);
}
