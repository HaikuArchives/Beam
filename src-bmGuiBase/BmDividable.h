/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmDividable_h
#define _BmDividable_h

#include "BmGuiBase.h"

class MView;

class IMPEXPBMGUIBASE BmDividable
{
public:
	virtual void SetDivider( float div) = 0;
	virtual float Divider() const = 0;
	static void DivideSame( MView* div1, ...);
};


#endif
