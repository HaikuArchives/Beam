/*
	BmCaption.h
		$Id$
*/

#ifndef _BmCaption_h
#define _BmCaption_h

#include <StringView.h>

class BmCaption : public BStringView
{
	typedef BStringView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmCaption( BRect frame, const char* text);
	~BmCaption();

	// overrides of BStringView base:
	void Draw( BRect bounds);

private:
	// Hide copy-constructor and assignment:
	BmCaption( const BmCaption&);
	BmCaption operator=( const BmCaption&);

};


#endif
