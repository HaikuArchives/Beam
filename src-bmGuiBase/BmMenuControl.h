/*
	BmMenuControl.h
		$Id$
*/

#ifndef _BmMenuControl_h
#define _BmMenuControl_h

#include <MenuField.h>

#include <layout.h>

class HGroup;

class BmMenuControl : public MView, public BMenuField
{
	typedef BMenuField inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMenuControl( const char* label, BMenu* menu);
	~BmMenuControl();
	
	// native methods:
	void DetachFromParent();
	void ReattachToParent();

private:
	minimax layoutprefs();
	BRect layout(BRect frame);
	
	HGroup* mParent;
};


#endif
