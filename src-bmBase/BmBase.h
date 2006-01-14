/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmBase_h
#define _BmBase_h

#include <BeBuild.h>

/* import-/export-declarations for the bmBase shared-lib */
#ifdef BM_BUILDING_BMBASE
#define IMPEXPBMBASE _EXPORT
#else
#define IMPEXPBMBASE _IMPORT
#endif

#endif
