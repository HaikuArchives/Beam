/*
	Beam.cpp
		$Id$
*/

#include "BmApp.h"

int main()
{
	BmApplication* app = new BmApplication("application/x-vnd.zooey-Beam");
	app->Run();
	delete app;
}

