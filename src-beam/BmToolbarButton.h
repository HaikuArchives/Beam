/*
	BmToolbarButton.h
		$Id$
*/

#ifndef _BmToolbarButton_h
#define _BmToolbarButton_h

#include <MPictureButton.h>

class BmToolbarButton : public MPictureButton
{
	typedef MPictureButton inherited;

public:
	// creator-func, c'tors and d'tor:
	BmToolbarButton( const char *label, BBitmap* image, 
						  BMessage *message, BHandler *handler, 
						  const char* tipText=NULL);
	~BmToolbarButton();

	// native methods:
	BPicture* CreateOnPictureFor( const char* label, BBitmap* image);
	BPicture* CreateOffPictureFor( const char* label, BBitmap* image);

	// overrides of Button base:
	void Draw( BRect bounds);

private:
	bool mHighlighted;

	// Hide copy-constructor and assignment:
	BmToolbarButton( const BmToolbarButton&);
	BmToolbarButton operator=( const BmToolbarButton&);
};


#endif
