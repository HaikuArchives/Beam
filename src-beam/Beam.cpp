/*
	Beam.cpp
		$Id$
*/

#include <Application.h>

#include "BmApp.h"

int main()
{
	BeamApp *beamApp = NULL;
	try {
		beamApp = new BeamApp;
		beamApp->Run();
	}
	catch( exception &e) {
		BM_LOGERR( BString("Beam: ") << e.what());
	}
	delete beamApp;
}
