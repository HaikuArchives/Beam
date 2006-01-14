/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include "BmBasics.h"

bool BeamInTestMode = false;
bool BeamInDevelMode = false;
bool BeamOnDano = false;

BmString BM_DefaultItemLabel("<default>");
BmString BM_NoItemLabel("<none>");

/********************************************************************************\
	BM_error
\********************************************************************************/

BM_error::BM_error( const BmString& what_arg)
	:	mWhat( what_arg)
{
}

BM_error::BM_error( const char* what_arg)
	:	mWhat( what_arg)
{
}

const char* const BM_error::what() const {
	return mWhat.String();
}

/********************************************************************************\
	BM_runtime_error
\********************************************************************************/

BM_runtime_error::BM_runtime_error( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_runtime_error::BM_runtime_error( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_invalid_argument
\********************************************************************************/

BM_invalid_argument::BM_invalid_argument( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_invalid_argument::BM_invalid_argument( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_network_error
\********************************************************************************/

BM_network_error::BM_network_error( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_network_error::BM_network_error( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_text_error
\********************************************************************************/

BM_text_error::BM_text_error( const BmString& what_arg, const char* ctx, 
										int32 pos)
	:	inherited( what_arg.String())
	,	posInText( pos)
	,	context( ctx)
{
}

BM_text_error::BM_text_error( const char* what_arg, const char* ctx, int32 pos)
	:	inherited( what_arg)
	,	posInText( pos)
	,	context( ctx)
{
}

/********************************************************************************\
	throwing-helpers...
\********************************************************************************/

void BM_Throw_Runtime( const BmString &s, int line, const char* file) { 
	throw BM_runtime_error( 
		BmString("*** Exception at ") << file << ":" << line<<" ***\n"<<s
	); 
}

void BM_Throw_Invalid( const BmString &s, int line, const char* file) { 
	throw BM_invalid_argument(
		BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s
	); 
}

void BM_Throw_Network( const BmString &s, int line, const char* file) { 
	throw BM_network_error(
		BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s
	); 
}
