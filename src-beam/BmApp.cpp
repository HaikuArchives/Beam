/*
	BmApp.cpp
		$Id$
*/

#include "BmApp.h"

BeamApp::BeamApp()
:	BApplication("application/x-vnd.zooey-Beam")
,	mainWin( NULL)
{
	mainWin = new BmMainWindow();
	mainWin->Show();
}

BeamApp::~BeamApp()
{
}

void BeamApp::MessageReceived(BMessage* msg) {
	switch( msg->what) {
/*		case BM_MAINWIN_DONE: 
//			PostMessage( B_QUIT_REQUESTED);
			break;
*/
		default:
			inherited::MessageReceived( msg);
	}
}
