/*
	BmMainWindow.h
		$Id$
*/

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <MWindow.h>

class MMenuBar;

class BmMailFolderView;
class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmToolbarButton;
class CLVContainerView;
class UserResizeSplitView;

#define BMM_NEW_MAILFOLDER				'bMfa'
#define BMM_PAGE_SETUP					'bMfb'
#define BMM_PRINT							'bMfc'
#define BMM_PREFERENCES					'bMfd'
#define BMM_FIND							'bMfe'
#define BMM_FIND_MESSAGES				'bMff'
#define BMM_FIND_NEXT					'bMfg'
#define BMM_CHECK_MAIL					'bMfh'
#define BMM_CHECK_ALL					'bMfi'
#define BMM_SEND_PENDING				'bMfj'
#define BMM_NEW_MAIL						'bMfk'
#define BMM_REPLY							'bMfl'
#define BMM_REPLY_ALL					'bMfm'
#define BMM_FORWARD						'bMfn'
#define BMM_FORWARD_ATTACHMENTS		'bMfo'
#define BMM_BOUNCE						'bMfp'
#define BMM_FILTER						'bMfq'
#define BMM_TRASH							'bMfr'

class BmMainWindow : public MWindow
{
	typedef MWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_FRAME = 		"bm:frm";
	static const char* const MSG_VSPLITTER = 	"bm:vspl";
	static const char* const MSG_HSPLITTER = 	"bm:hspl";

public:
	// creator-func, c'tors and d'tor:
	static BmMainWindow* CreateInstance();
	BmMainWindow();
	~BmMainWindow();

	// class-methods:
	static bool IsAlive();

	// native methods:
	void BeginLife();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t Archive( BMessage* archive, bool deep=true) const;

	static BmMainWindow* theInstance;

private:
	CLVContainerView* CreateMailFolderView( minimax minmax, int32 width, int32 height);
	CLVContainerView* CreateMailRefView( minimax minmax, int32 width, int32 height);
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	MMenuBar* CreateMenu();
	bool Store();
	status_t Unarchive( BMessage* archive, bool deep=true);

	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	UserResizeSplitView* mVertSplitter;
	UserResizeSplitView* mHorzSplitter;
	
	BmToolbarButton* mCheckButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mReplyAllButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mBounceButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;

	static bool nIsAlive;
};

#define TheMainWindow BmMainWindow::theInstance

#endif
