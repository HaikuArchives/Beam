/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>

#include "BmConnectionWin.h"
#include "BmMainWindow.h"
#include "BmMailFolderList.h"

class BeamApp : public BApplication
{
	typedef BApplication inherited;

	Beam beam;
	BmMainWindow *mainWin;
	BmMailFolderList *folderList;
public:
	BeamApp();
	~BeamApp();
	void MessageReceived(BMessage*);
};


#endif
