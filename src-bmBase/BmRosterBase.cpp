/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include "BmRosterBase.h"

BmRosterBase* BeamRoster = 0;

BmGuiRosterBase* BeamGuiRoster = 0;

BLooper* TheJobMetaController = 0;

BLooper* BmGuiRosterBase::JobMetaController()
{
	return TheJobMetaController;
}
