/*
	BmMailView.h
		$Id$
*/

#ifndef _BmMailView_h
#define _BmMailView_h

#include <layout.h>

#include "BetterScrollView.h"
#include "WrappingTextView.h"

#include "BmController.h"
#include "BmMail.h"

class BmMailRef;
class BmMailViewContainer;
class BmMailHeaderView;

/*------------------------------------------------------------------------------*\
	BmMailView
		-	
\*------------------------------------------------------------------------------*/
class BmMailView : public WrappingTextView, public BmJobController {
	typedef WrappingTextView inherited;
	typedef BmJobController inheritedController;

	// archival-fieldnames:
	static const char* const MSG_RAW = 			"bm:raw";
	static const char* const MSG_FONTNAME = 	"bm:fnt";
	static const char* const MSG_FONTSIZE =	"bm:fntsz";

public:
	// creator-func, c'tors and d'tor:
	static BmMailView* CreateInstance(  minimax minmax, BRect frame, bool editable);
	BmMailView( minimax minmax, BRect frame, bool editable);
	~BmMailView();

	// native methods:
	void ShowMail( BmMailRef* ref);
	void DisplayBodyPart( BString& displayText, const BmBodyPart& bodyPart);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);
	bool Store();

	// overrides of BTextView base:
	void FrameResized( float newWidth, float newHeight);
	void MakeFocus(bool focused);
	void MessageReceived( BMessage* msg);

	// overrides of BmController base:
	BHandler* GetControllerHandler()	{ return this; }
	void JobIsDone( bool completed);
	void DetachModel();

	// getters:
	BmMailViewContainer* ContainerView() const	{ return mScrollView; }

private:
	// will not be archived:
	bool mEditMode;
	BmRef< BmMail> mCurrMail;
	BmMailViewContainer* mScrollView;
	BmMailHeaderView* mHeaderView;
	// will be archived:
	BString mFontName;
	int16 mFontSize;
	bool mRaw;
};

class BmBusyView;
/*------------------------------------------------------------------------------*\
	BmMailViewContainer
		-	
\*------------------------------------------------------------------------------*/
class BmMailViewContainer : public MView, public BetterScrollView {
	typedef BetterScrollView inherited;

public:
	BmMailViewContainer( minimax minmax, BmMailView* target, uint32 resizingMode, 
								uint32 flags);
	~BmMailViewContainer();

	// native methods:
	void SetBusy();
	void UnsetBusy();
	void PulseBusyView();

	// overrides of BView base:
	void FrameResized(float new_width, float new_height);
	void Draw( BRect bounds);

	// additions for liblayout:
	virtual minimax layoutprefs();
	virtual BRect layout(BRect);

private:
	BmBusyView* mBusyView;
};

#endif
