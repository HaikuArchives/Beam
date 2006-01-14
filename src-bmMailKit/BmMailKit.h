/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmMailKit_h
#define _BmMailKit_h

#include <BeBuild.h>

/* import-/export-declarations for the bmMailKit shared-lib */
#ifdef BM_BUILDING_BMMAILKIT
#define IMPEXPBMMAILKIT _EXPORT
#else
#define IMPEXPBMMAILKIT _IMPORT
#endif

#endif
