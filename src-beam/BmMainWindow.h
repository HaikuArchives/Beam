/*
	BmMainWindow.h
		$Id$
*/

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <MWindow.h>

class BmMailFolderView;
class BmMailRefView;
class CLVContainerView;

class BmMainWindow : public MWindow
{
	typedef MWindow inherited;

public:
	BmMainWindow();
	~BmMainWindow();

	// BeOS-stuff:
	void MessageReceived(BMessage*);
	bool QuitRequested();
	void Quit();

	//
	static bool IsAlive();

	//
	void BeginLife();

	//
	CLVContainerView* CreateMailFolderView( minimax, int32, int32);
	CLVContainerView* CreateMailRefView( minimax, int32, int32);

	// getters:
	BmMailFolderView* MailFolderView()	{ return mMailFolderView; }
	BmMailRefView* MailRefView()			{ return mMailRefView; }
	BmMailFolderList* MailFolderList()	{ return mMailFolderList; }
	
private:
	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	BmMailFolderList* mMailFolderList;

	static bool nIsAlive;
};


#endif
