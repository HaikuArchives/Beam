/*
	BmUtil.h
		$Id$
*/

#ifndef _BmUtil_h
#define _BmUtil_h

#include <ctime>
#include <stdexcept>

#include <Message.h>
#include <String.h>

#define BM_assert(expr) if (!expr) { throw BM_invalid_argument(BString("BM_assert says no at ")<<__FILE__<<":"<<__LINE__); }

/*------------------------------------------------------------------------------*\*\
	ShowAlert( text, logtext)
		-	pops up an Alert showing the passed text
		-	logs text unless logtext is specified, in which case that is 
			written to the logfile
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text);

/*------------------------------------------------------------------------------*\*\
	BMruntime_error
		-	exception to indicate a general runtime error
\*------------------------------------------------------------------------------*/
class BM_runtime_error : public runtime_error {
	typedef runtime_error inherited;
public:
	BM_runtime_error (const BString& what_arg): inherited (what_arg.String()) { }
	BM_runtime_error (const char* const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BMinvalid_argument
		-	exception to indicate an invalid-argument error
\*------------------------------------------------------------------------------*/
class BM_invalid_argument : public invalid_argument {
	typedef invalid_argument inherited;
public:
	BM_invalid_argument (const BString& what_arg): inherited (what_arg.String()) { }
	BM_invalid_argument (const char* const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BMnetwork_error
		-	exception to indicate an error during network communication
\*------------------------------------------------------------------------------*/
class BM_network_error : public BM_runtime_error {
	typedef BM_runtime_error inherited;
public:
	BM_network_error (const BString& what_arg): inherited (what_arg.String()) { }
	BM_network_error (const char* const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BM_THROW_...
		-	throws exception of specific type
\*------------------------------------------------------------------------------*/
inline bool BM_THROW_RUNTIME( const BString &s) { throw BM_runtime_error(s); }
inline bool BM_THROW_INVALID( const BString &s) { throw BM_invalid_argument(s); }
inline bool BM_THROW_NETWORK( const BString &s) { throw BM_network_error(s); }
inline bool BM_DIE( const BString &s) { ShowAlert(s); exit(10); }

/*------------------------------------------------------------------------------*\*\
	FindMsgXXX( archive, name)
		-	functions that extract the msg-field of a specified name from the given 
			archive and return it.
\*------------------------------------------------------------------------------*/
const char* FindMsgString( BMessage* archive, const char* name, int32 index=0);
bool FindMsgBool( BMessage* archive, const char* name, int32 index=0);
int64 FindMsgInt64( BMessage* archive, const char* name, int32 index=0);
int32 FindMsgInt32( BMessage* archive, const char* name, int32 index=0);
int16 FindMsgInt16( BMessage* archive, const char* name, int32 index=0);
float FindMsgFloat( BMessage* archive, const char* name, int32 index=0);
BMessage* FindMsgMsg( BMessage* archive, const char* name, BMessage* msg=NULL, int32 index=0);
void* FindMsgPointer( BMessage* archive, const char* name, int32 index=0);

/*------------------------------------------------------------------------------*\*\
	utility function to format a number of bytes into a string
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes, bool mini=false);

/*------------------------------------------------------------------------------*\*\
	utility function to format a time into a string
\*------------------------------------------------------------------------------*/
BString TimeToString( time_t t);

/*------------------------------------------------------------------------------*\*\
	utility defines to shorten the use of auto_ptrs
\*------------------------------------------------------------------------------*/
#define BmPtr const auto_ptr
#define BmNcPtr auto_ptr

/*------------------------------------------------------------------------------*\*\
	utility operator to easy concatenation of BStrings
\*------------------------------------------------------------------------------*/
BString operator+(const BString& s1, const BString& s2);
BString operator+(const char* s1, const BString& s2);
BString operator+(const BString& s1, const char* s2);

#endif
