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
	CLVContainerView* CreateMailFolderView( minimax minmax, int32 width, int32 height);
	CLVContainerView* CreateMailRefView( minimax minmax, int32 width, int32 height);

private:
	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;

	static bool nIsAlive;
};


#endif
