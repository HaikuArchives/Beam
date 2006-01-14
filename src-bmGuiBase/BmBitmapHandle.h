/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmBitmapHandle_h
#define _BmBitmapHandle_h

#include <Bitmap.h>

struct BmBitmapHandle
{
	BmBitmapHandle()
		:	bitmap(NULL)						{}
	BmBitmapHandle(BBitmap* b)
		:	bitmap(b)							{}
	BBitmap* bitmap;
	BBitmap* operator-> () const			{ return bitmap; }
};


#endif
