/*
	BmMainWindow.h
		$Id$
*/

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <Window.h>

class BmMailFolderView;
class BmMailRefView;
class CLVContainerView;

class BmMainWindow : public BWindow
{
	typedef BWindow inherited;

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
	CLVContainerView* CreateMailFolderView( BRect rect);
	CLVContainerView* CreateMailRefView( BRect rect);

private:
	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;

	static bool nIsAlive;
};


#endif
