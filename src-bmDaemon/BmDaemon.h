/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmDaemon_h
#define _BmDaemon_h

#include <BeBuild.h>

/* import-/export-declarations for the bmDaemon shared-lib */
#ifdef BM_BUILDING_BMDAEMON
#define IMPEXPBMDAEMON _EXPORT
#else
#define IMPEXPBMDAEMON _IMPORT
#endif

#endif
