/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmCaption_h
#define _BmCaption_h

#include <View.h>

#include "BmGuiBase.h"

#include "BmString.h"

class IMPEXPBMGUIBASE BmCaption : public BView
{
	typedef BView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmCaption( BRect frame, const char* text);
	~BmCaption();

	const char* Text() const				{ return mText.String(); }
	void SetText( const char* txt);

	void SetHighlight( bool highlight, const char* label="");

	// overrides of BView base:
	virtual void Draw( BRect bounds);

private:
	// Hide copy-constructor and assignment:
	BmCaption( const BmCaption&);
	BmCaption operator=( const BmCaption&);
	
	BmString mText;
	bool mHighlight;
	BmString mHighlightLabel;
};


#endif
