/*
	BmTextControl.h
		$Id$
*/

#ifndef _BmTextControl_h
#define _BmTextControl_h

#include <TextControl.h>

#include <layout.h>

class HGroup;

class BmTextControl : public MView, public BTextControl
{
	typedef BTextControl inherited;

public:
	// creator-func, c'tors and d'tor:
	BmTextControl( const char* label, bool labelIsMenu=false);
	~BmTextControl();
	
	// native methods:
	void DetachFromParent();
	void ReattachToParent();

	// overrides of BTextControl:
	void FrameResized( float new_width, float new_height);

	// getters:
	BTextView* TextView() const 			{ return mTextView; }
	BMenuField* MenuField() const 		{ return mMenuField; }

private:
	minimax layoutprefs();
	BRect layout(BRect frame);

	bool mLabelIsMenu;
	BTextView* mTextView;
	BMenuField* mMenuField;
	HGroup* mParent;
};


#endif
