/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _libregexx_h
#define _libregexx_h

#include <BeBuild.h>

/* import-/export-declarations for the libregexx shared-lib */
#ifdef BM_BUILDING_BMREGEXX
#define IMPEXPBMREGEXX _EXPORT
#else
#define IMPEXPBMREGEXX _IMPORT
#endif

#endif
