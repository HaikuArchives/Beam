/*
	BmTextControl.h
		$Id$
*/

#ifndef _BmTextControl_h
#define _BmTextControl_h

#include <TextControl.h>

#include <layout.h>

class HGroup;

#define BM_TEXTFIELD_MODIFIED 'bmfm'

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
	void SetTextSilently( const char* text);

	// overrides of BTextControl:
	void FrameResized( float new_width, float new_height);
	void SetEnabled( bool enabled);
	void SetText( const char* text);

	// getters:
	inline BTextView* TextView() const 	{ return mTextView; }
	inline BMenuField* MenuField() const	{ return mMenuField; }
	inline BMenu* Menu() const 			{ return mMenuField ? mMenuField->Menu() : NULL; }

private:
	minimax layoutprefs();
	BRect layout(BRect frame);

	bool mLabelIsMenu;
	BTextView* mTextView;
	BMenuField* mMenuField;
	HGroup* mParent;

	// Hide copy-constructor and assignment:
	BmTextControl( const BmTextControl&);
	BmTextControl operator=( const BmTextControl&);
};


#endif
