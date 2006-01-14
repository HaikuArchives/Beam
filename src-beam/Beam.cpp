/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "BeamApp.h"

int main()
{
	BeamApplication* app = new BeamApplication( BM_APP_SIG);
	app->Run();
	delete app;
}

