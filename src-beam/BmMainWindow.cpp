/*
	BmMainWindow.cpp
		$Id$
*/

#include <InterfaceKit.h>
#include <Message.h>

#include <layout-all.h>

#include "BmMainWindow.h"

BmMainWindow::BmMainWindow()
: MWindow( BRect(5,20,0,0), "Beam (BEware, Another Mailer)",
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					B_ASYNCHRONOUS_CONTROLS)
{
	MView *mOuterGroup = 
		new VGroup(
				new MButton("Lass gut sein...",
								new BMessage(B_QUIT_REQUESTED), 
								be_app),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

BmMainWindow::~BmMainWindow() {
}

void BmMainWindow::MessageReceived( BMessage *msg) {
	switch( msg->what) {
/*		case BM_MAINWIN_DONE: 
			break;
*/
		default:
			inherited::MessageReceived( msg);
	}
}
