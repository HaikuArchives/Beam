/*
	BmMailHeaderView.h
		$Id$
*/

#ifndef _BmMailHeaderView_h
#define _BmMailHeaderView_h

#include <View.h>

class BmMailHeader;

/*------------------------------------------------------------------------------*\
	BmMailHeaderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailHeaderView : public BView {
	typedef BView inherited;

	static const int SMALL_HEADERS = 0;
	static const int LARGE_HEADERS = 1;
	static const int FULL_HEADERS = 2;

	// archival-fieldnames:
	static const char* const MSG_MODE = 		"bm:mode";

public:
	// c'tors and d'tor:
	BmMailHeaderView( BmMailHeader* header);
	~BmMailHeaderView();

	// native methods:
	void ShowHeader( BmMailHeader* header);
	void ShowMenu( BPoint point);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);

	// overrides of BView base:
	void Draw( BRect bounds);
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);

private:
	BmMailHeader* mMailHeader;
	int16 mDisplayMode;							// 0=small, 2=large, anyother=medium
	BFont* mFont;								// font to be used for header-fields
};

#endif
