/*
	BmMessage.h
		$Id$
*/

#ifndef _BmMessage_h
#define _BmMessage_h

#include <map>
#include <string>

#include <regexx.hh>
#include <split.hh>

// -----------------------------------------------
class BmMessage {

public:
	BmMessage( );
	BmMessage( char *msgText, const char *msgUID=NULL);
	virtual ~BmMessage();

	void Set( char *msgText, const char *msgUID=NULL);	

private:
	typedef map<string, string> HeaderMap;
	HeaderMap mHeaders;
	string mBody;
	string mUID;

	void SplitOffHeaders();
};

#endif

