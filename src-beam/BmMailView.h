/*
	BmMailView.h
		$Id$
*/

#ifndef _BmMailView_h
#define _BmMailView_h

#include <TextView.h>

#include <layout.h>

#include "BetterScrollView.h"

#include "BmController.h"

class BmMail;
class BmMailRef;
class BmMailViewContainer;

/*------------------------------------------------------------------------------*\
	BmMailView
		-	
\*------------------------------------------------------------------------------*/
class BmMailView : public BTextView, public BmJobController {
	typedef BTextView inherited;
	typedef BmJobController inheritedController;

public:
	// creator-func, c'tors and d'tor:
	static BmMailView* CreateInstance(  minimax minmax, BRect frame, bool editable);
	BmMailView( minimax minmax, BRect frame, bool editable);
	~BmMailView();

	// native methods:
	void ShowMail( BmMailRef* ref);

	// overrides of BTextView base:
	void FrameResized(float new_width, float new_height);
	void MessageReceived( BMessage* msg);

	// overrides of BmController base:
	BHandler* GetControllerHandler()	{ return this; }
	void JobIsDone( bool completed);

	// getters:
	BmMailViewContainer* ContainerView() const	{ return mScrollView; }

	static BmMailView* theInstance;
	
private:
	bool mEditMode;
	BmRef< BmMail> mCurrMail;
	BmMailViewContainer* mScrollView;
};

#define TheMailView BmMailView::theInstance


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

	// overrides of BView base:
	void FrameResized(float new_width, float new_height);
	void ResizeTo( float w,float h);

	// additions for liblayout:
	virtual minimax layoutprefs();
	virtual BRect layout(BRect);

};

#endif
