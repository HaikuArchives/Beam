/*
	TestPopper.cpp
		$Id$
*/

#include <stdio.h>
#include <Application.h>
#include <MButton.h>
#include <MWindow.h>

#include "BmConnectionWin.h"

static char buf[30];

// for testing:
#define BM_MSG_NOCH_EINER 'bmt1'

class GenericWin : public MWindow
{
public:
	GenericWin();
	~GenericWin();
	bool QuitRequested();
};

class GenericApp : public BApplication
{
	BmConnectionWin *win;
	GenericWin *gw;
public:
	GenericApp();
	~GenericApp();
	void MessageReceived(BMessage*);
	bool QuitRequested();
};

int main()
{
	BmPrefs::InitPrefs();
	new GenericApp;
	try {
		be_app->Run();
	} 
	catch( exception &e) {
		BmLOG( string("Oops: %s") + e.what());
	}
	delete be_app;
}

GenericWin::GenericWin()
: MWindow( BRect(5,20,0,0), "hit me!",
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					B_ASYNCHRONOUS_CONTROLS)
{
	MView *mOuterGroup = 
		new VGroup(
				new MButton("Noch einer...",new BMessage(BM_MSG_NOCH_EINER), be_app),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

GenericWin::~GenericWin()
{ }

bool GenericWin::QuitRequested() {
	be_app->PostMessage( B_QUIT_REQUESTED);
	return true;
}

GenericApp::GenericApp()
: BApplication("application/x-vnd.OT-Generic"), win(0)
{
	win = new BmConnectionWin( "ConnectionWin", this);
	win->Hide();
	win->Show();
	gw = new GenericWin();
	gw->Show();
}

GenericApp::~GenericApp()
{ }

bool GenericApp::QuitRequested() {
	return true;
}

void GenericApp::MessageReceived(BMessage* msg) {
	BMessage* archive;
	BmPopAccount acc;
	switch( msg->what) {
		case BM_MSG_NOCH_EINER: 
			archive = new BMessage(BM_POPWIN_FETCHMSGS);
			sprintf(buf, "mailtest@kiwi");
			acc.Name( buf);
			acc.Username( "mailtest");
			acc.Password( "mailtest");
			acc.POPServer( "kiwi");
			acc.PortNr( 110);
			acc.SMTPPortNr( 25);
			acc.Archive( archive, false);
			win->PostMessage( archive);
			delete archive;

			archive = new BMessage(BM_POPWIN_FETCHMSGS);
			sprintf(buf, "zooey@kiwi");
			acc.Name( buf);
			acc.Username( "zooey");
			acc.Password( "leeds#42");
			acc.POPServer( "kiwi");
			acc.PortNr( 110);
			acc.SMTPPortNr( 25);
			acc.Archive( archive, false);
			win->PostMessage( archive);
			delete archive;

			break;
		case BM_POPWIN_DONE: 
			PostMessage( B_QUIT_REQUESTED);
			break;
		default:
			BApplication::MessageReceived( msg);
	}
}
