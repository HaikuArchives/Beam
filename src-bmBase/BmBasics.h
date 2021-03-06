/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmBasics_h
#define _BmBasics_h

#include <stdexcept>

#include <Debug.h>

#include "BmBase.h"
#include "BmString.h"

#define BM_ASSERT(E)		(!(E) ? _debuggerAssert(__FILE__,__LINE__, (char*)#E) \
										: (int)0)

extern IMPEXPBMBASE bool BeamInTestMode;		
							// indicates if Beam is running in test-mode

extern IMPEXPBMBASE bool BeamInDevelMode;		
							// indicates if this is a devel-version of Beam

extern IMPEXPBMBASE bool BeamOnDano;
							// indicates if Beam is running on Dano or later

extern IMPEXPBMBASE BmString BM_DefaultItemLabel;
extern IMPEXPBMBASE BmString BM_NoItemLabel;

/*------------------------------------------------------------------------------*\
	BM_error
		-	base-class for any Beam-exception
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BM_error : public std::runtime_error {
	typedef std::runtime_error inherited;
public:
	BM_error( const BmString& what_arg);
	BM_error( const char* what_arg);
	virtual ~BM_error () throw();
private:
};

/*------------------------------------------------------------------------------*\
	BM_runtime_error
		-	exception to indicate a general runtime error
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BM_runtime_error : public BM_error {
	typedef BM_error inherited;
public:
	BM_runtime_error (const BmString& what_arg);
	BM_runtime_error (const char* const what_arg);
	virtual ~BM_runtime_error () throw();
};

/*------------------------------------------------------------------------------*\
	BM_invalid_argument
		-	exception to indicate an invalid-argument error
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BM_invalid_argument : public BM_error {
	typedef BM_error inherited;
public:
	BM_invalid_argument (const BmString& what_arg);
	BM_invalid_argument (const char* const what_arg);
	virtual ~BM_invalid_argument () throw();
};

/*------------------------------------------------------------------------------*\
	BM_network_error
		-	exception to indicate an error during network communication
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BM_network_error : public BM_runtime_error {
	typedef BM_runtime_error inherited;
public:
	BM_network_error (const BmString& what_arg);
	BM_network_error (const char* const what_arg);
	virtual ~BM_network_error () throw();
};

/*------------------------------------------------------------------------------*\
	BM_text_error
		-	exception to indicate a problem with a given text
		-	additionally, a character position may be given that indicates
			the position of the problem within the text
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BM_text_error : public BM_runtime_error {
	typedef BM_runtime_error inherited;
public:
	BM_text_error (const BmString& what_arg, const char* ctx="", int32 pos=-1);
	BM_text_error (const char* const what_arg, const char* ctx="", int32 pos=-1);
	virtual ~BM_text_error () throw();
	int32 posInText;
	BmString context;
};

/*------------------------------------------------------------------------------*\
	BM_THROW_...
		-	throws exception of specific type
\*------------------------------------------------------------------------------*/
#define BM_THROW_RUNTIME(s) BM_Throw_Runtime(s,__LINE__,__FILE__)

#define BM_THROW_INVALID(s) BM_Throw_Invalid(s,__LINE__,__FILE__)

#define BM_THROW_NETWORK(s) BM_Throw_Network(s,__LINE__,__FILE__)

IMPEXPBMBASE void BM_Throw_Runtime( const BmString &s, int line, 
												const char* file);

IMPEXPBMBASE void BM_Throw_Invalid( const BmString &s, int line, 
												const char* file);

IMPEXPBMBASE void BM_Throw_Network( const BmString &s, int line, 
												const char* file);

/*------------------------------------------------------------------------------*\
	utility defines to shorten the use of auto_ptrs
\*------------------------------------------------------------------------------*/
#define BmPtr const auto_ptr
#define BmNcPtr auto_ptr

#endif
