/*
	BmMessage.cpp
		$Id$
*/

#include "BmMessage.h"

//-------------------------------------------------
BmMessage::BmMessage( ) {
}

//-------------------------------------------------
BmMessage::BmMessage( char *msgText, const char *msgUID=NULL) {
	Set( msgText, msgUID);
}
	
//-------------------------------------------------
BmMessage::~BmMessage() {
}

//-------------------------------------------------
void BmMessage::Set( char *msgText, const char *msgUID=NULL) {
	assert( msgText);

	char *border = strstr( msgText, "\r\n\r\n")
	if (!border) {
		throw runtime_error("Could not determine borderline between header and text of message");
	}
	*border = 0;
	border += 4;
	mBody = border;

	Regexx rxx;
	rxx.expr( "\r\n\W");
	rxx.str( msgText);
	mHeaders.reserve( rxx.exec());
	vector<RegexxMatch>::const_iterator i;
	string::size_type lastpos = 0;
	for( i = rxx.match.begin(); i != rxx.match.end(); ++i) {
		
//		mHeadersv.push_back(_str.substr(lastpos,i->start()-lastpos));
		lastpos = i->start()+i->length();
	}
	v.push_back(_str.substr(lastpos,i->start()));
}

//-------------------------------------------------
void BmMessage::SplitOffHeaders() {
}
