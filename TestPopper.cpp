/*
	TestPopper.cpp
		$Id$
*/

#include <stdio.h>
#include <Application.h>
#include <MButton.h>
#include <MWindow.h>

#include "BmApp.h"
#include "BmConnectionWin.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPopper.h"

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

class GenericApp : public BmApplication
{
	BmConnectionWin *win;
	GenericWin *gw;
	int32 count;
public:
	GenericApp();
	~GenericApp();
	void MessageReceived(BMessage*);
};

int main()
{
	GenericApp *testApp = new GenericApp;
	try {
		testApp->Run();
	} 
	catch( exception &e) {
		BM_LOGERR( BString("Oops: %s") << e.what());
	}
	delete testApp;
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
	bmApp->PostMessage( B_QUIT_REQUESTED);
	return true;
}

GenericApp::GenericApp()
: BmApplication("application/x-vnd.OT-Generic"), win(0), count(0)
{
	win = new BmConnectionWin( "ConnectionWin", this);
	win->Hide();
	win->Show();
	gw = new GenericWin();
	gw->Show();
}

GenericApp::~GenericApp()
{
}

void GenericApp::MessageReceived(BMessage* msg) {
	BMessage* archive;
	BmPopAccount acc;
	switch( msg->what) {
		case BM_MSG_NOCH_EINER: 

			count++;
			archive = new BMessage(BM_CONNWIN_FETCHPOP);
			if (count % 3 == 1) {
				sprintf(buf, "mailtest@kiwi:110");
				acc.Name( buf);
				acc.Username( "mailtest");
				acc.Password( "mailtest");
				acc.POPServer( "kiwi");
				acc.PortNr( 110);
				acc.SMTPPortNr( 25);
				acc.Archive( archive, false);
			} else if (count % 3 == 2) {
				sprintf(buf, "mailtest2@kiwi:110");
				acc.Name( buf);
				acc.Username( "mailtest2");
				acc.Password( "mailtest2");
				acc.POPServer( "kiwi");
				acc.PortNr( 110);
				acc.SMTPPortNr( 25);
				acc.Archive( archive, false);
			} else if (count % 3 == 0) {
				sprintf(buf, "negativland@t-online.de");
				acc.Name( buf);
				acc.Username( "negativland");
				acc.Password( "xxx");
				acc.POPServer( "pop.t-online.de");
				acc.PortNr( 110);
				acc.SMTPPortNr( 25);
				acc.Archive( archive, false);
			}
			archive->AddString( BmConnectionWin::MSG_CONN_NAME, acc.Name());
			win->PostMessage( archive);
			delete archive;

/*
			archive = new BMessage(BM_POPWIN_FETCHMSGS);
			sprintf(buf, "mailtest@kiwi:112");
			acc.Name( buf);
			acc.Username( "mailtest");
			acc.Password( "mailtest");
			acc.POPServer( "kiwi");
			acc.PortNr( 112);
			acc.SMTPPortNr( 25);
			acc.Archive( archive, false);
			win->PostMessage( archive);
			delete archive;
*/

			break;
//		case BM_CONNWIN_DONE: 
//			PostMessage( B_QUIT_REQUESTED);
			break;
		default:
			BmApplication::MessageReceived( msg);
	}
}
