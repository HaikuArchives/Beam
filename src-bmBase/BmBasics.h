/*
	BmBasics.h
		$Id$
*/

#ifndef _BmBasics_h
#define _BmBasics_h

#include <stdexcept>

#include <Autolock.h>
#include <String.h>

#define BM_assert(expr) if (!expr) { throw BM_invalid_argument(BString("BM_assert says no at ")<<__FILE__<<":"<<__LINE__); }

void ShowAlert( const BString &text);	// forward declaration

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
#define BM_THROW_RUNTIME(s) BM_Throw_Runtime(s,__LINE__,__FILE__)
inline bool BM_Throw_Runtime( const BString &s, int line, const char* file) { 
	throw BM_runtime_error(BString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}
#define BM_THROW_INVALID(s) BM_Throw_Invalid(s,__LINE__,__FILE__)
inline bool BM_Throw_Invalid( const BString &s, int line, const char* file) { 
	throw BM_invalid_argument(BString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}
#define BM_THROW_NETWORK(s) BM_Throw_Network(s,__LINE__,__FILE__)
inline bool BM_Throw_Network( const BString &s, int line, const char* file) { 
	throw BM_network_error(BString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}
inline bool BM_DIE( const BString &s) { ShowAlert(s); exit(10); }

/*------------------------------------------------------------------------------*\*\
	utility defines to shorten the use of auto_ptrs
\*------------------------------------------------------------------------------*/
#define BmPtr const auto_ptr
#define BmNcPtr auto_ptr

/*------------------------------------------------------------------------------*\*\
	wrapper around BAutolock that enhances profiling output
\*------------------------------------------------------------------------------*/
// during profiling we use this:
/*
class BmAutolock : public BAutolock {
public:
	BmAutolock( BLooper* l) : BAutolock( l) {};
	BmAutolock( BLocker* l) : BAutolock( l) {};
	BmAutolock( BLocker& l) : BAutolock( l) {};
};
*/
//otherwise, we use this:
#define BmAutolock BAutolock

#endif
