/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
/*
 * Beam's test-application is based on the OpenBeOS testing framework
 * (which in turn is based on cppunit). Big thanks to everyone involved!
 *
 */


#ifndef _TestBeam_h
#define _TestBeam_h

#include "BmString.h"

void SlurpFile( const char* filename, BmString& str);

void DumpResult( const BmString& str);

extern BmString AsciiAlphabet[16];
extern bool HaveTestdata;
extern bool LargeDataMode;

struct Activator {
	Activator( bool& f) : flag( f) 		{ flag = true; }
	~Activator()								{ flag = false; }
	bool& flag;
};

#endif
