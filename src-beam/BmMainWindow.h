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
class UserResizeSplitView;

class BmMainWindow : public MWindow
{
	typedef MWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_FRAME = 		"bm:frm";
	static const char* const MSG_VSPLITTER = 	"bm:vspl";

public:
	// creator-func, c'tors and d'tor:
	static BmMainWindow* CreateInstance();
	BmMainWindow();
	~BmMainWindow();

	// class-methods:
	static bool IsAlive();

	// native methods:
	void BeginLife();
	CLVContainerView* CreateMailFolderView( minimax minmax, int32 width, int32 height);
	CLVContainerView* CreateMailRefView( minimax minmax, int32 width, int32 height);
	status_t Unarchive( BMessage* archive, bool deep=true);
	bool Store();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t Archive( BMessage* archive, bool deep=true) const;

private:
	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	UserResizeSplitView* mVertSplitter;

	static bool nIsAlive;
};


#endif
