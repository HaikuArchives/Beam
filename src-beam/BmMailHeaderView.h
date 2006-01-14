/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailHeaderView_h
#define _BmMailHeaderView_h

#include <vector>

#include <MessageFilter.h>
#include <View.h>

#include "BmMailHeader.h"
#include "BmRefManager.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailHeaderView:
\*------------------------------------------------------------------------------*/
enum {
	BM_HEADERVIEW_SMALL			= 'bmfa',
	BM_HEADERVIEW_LARGE			= 'bmfb',
	BM_HEADERVIEW_FULL			= 'bmfc',
	BM_HEADERVIEW_SWITCH_RESENT= 'bmfd',
	BM_HEADERVIEW_COPY_HEADER	= 'bmfe'
};

/*------------------------------------------------------------------------------*\
	BmMailHeaderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailHeaderView : public BView {
	typedef BView inherited;
	class BmMailHeaderFieldView;

	// archival-fieldnames:
	static const char* const MSG_VERSION;
	static const char* const MSG_MODE;
	static const char* const MSG_REDIRECT_MODE;
	static const char* const MSG_FONTNAME;
	static const char* const MSG_FONTSIZE;

public:
	enum {
		SMALL_HEADERS = 0,
		LARGE_HEADERS,
		FULL_HEADERS
	};

	// c'tors and d'tor:
	BmMailHeaderView( BmMailHeader* header);
	~BmMailHeaderView();

	// native methods:
	void ShowHeader( BmMailHeader* header, bool invalidate=true);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);
	float AddFieldViews();
	void RemoveFieldViews();
	float FixedWidth() const				{ return 5000; }
	int16 DisplayMode() const				{ return mDisplayMode; }
	bool ShowRedirectFields() const		{ return mShowRedirectFields; }

	// overrides of BView base:
	void Draw( BRect bounds);
	void MessageReceived( BMessage* msg);
	status_t UISettingsChanged(const BMessage* changes, uint32 flags);

private:
	BmRef<BmMailHeader> mMailHeader;
	int16 mDisplayMode;
							// 0=small, 2=large, anyother=medium
	BFont mFont;		
							// font to be used for header-fields
	bool mShowRedirectFields;
							// true=>show redirect-fields false=>show original fields
	float mMaxTitleWidth;
	vector<BmMailHeaderFieldView*> mFieldViews;

	// Hide copy-constructor and assignment:
	BmMailHeaderView( const BmMailHeaderView&);
	BmMailHeaderView operator=( const BmMailHeaderView&);
};

#endif
