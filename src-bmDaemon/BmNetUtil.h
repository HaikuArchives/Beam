/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmNetUtil_h
#define _BmNetUtil_h

#include "BmString.h"

#include "BmDaemon.h"

/*------------------------------------------------------------------------------*\*\
	utility function that finds out this hosts FQHN:
\*------------------------------------------------------------------------------*/
IMPEXPBMDAEMON BmString OwnDomain( BmString fqdn="");

/*------------------------------------------------------------------------------*\*\
	utility function that checks if PPP is up and running:
\*------------------------------------------------------------------------------*/
IMPEXPBMDAEMON bool IsPPPRunning();

#endif
