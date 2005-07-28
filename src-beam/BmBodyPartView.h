/*
	BmBodyPartView.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#ifndef _BmBodyPartView_h
#define _BmBodyPartView_h

#include "BmListController.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmBodyPartView:
\*------------------------------------------------------------------------------*/
enum {
	BM_BODYPARTVIEW_SHOWALL				 = 'bmga',
	BM_BODYPARTVIEW_SHOWATTACHMENTS	 = 'bmgb',
	BM_BODYPARTVIEW_SAVE_ATTACHMENT	 = 'bmgc',
	BM_BODYPARTVIEW_DELETE_ATTACHMENT = 'bmgd',
	BM_BODYPARTVIEW_SRC_CHARSET		 = 'bmge',
	BM_BODYPARTVIEW_DEST_CHARSET		 = 'bmgf'
};

class BFilePanel;
class BmBodyPartList;
/*------------------------------------------------------------------------------*\
	BmBodyPartView
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPartView : public BmListViewController
{
	typedef BmListViewController inherited;
	
	// archival-fieldnames:
	static const char* const MSG_SHOWALL;

public:
	// c'tors and d'tor:
	BmBodyPartView( minimax minmax, int32 width, int32 height, bool editable=false);
	~BmBodyPartView();

	// native methods:
	void AddAttachment( const BMessage* msg);
	void AddAttachment( const char* path);
	void AdjustVerticalSize();
	void ShowBody( BmBodyPartList* body);

	// overrides of listview base:
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	void KeyDown(const char *bytes, int32 numBytes);
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( const BMessage* archive, bool deep=true);

	// overrides of controller base:
	void AddAllModelItems();
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void ItemInvoked( int32 index);
	void RemoveModelItem( BmListModelItem* item);
	BmString StateInfoBasename();
	void HandleDrop( BMessage*);

	// getters:
	inline float FixedWidth() 				{ return 5000; }
	inline bool ShowAllParts()				{ return mShowAllParts; }

	// setters:
	inline void IsUsedForPrinting( bool b) 
													{ mIsUsedForPrinting = b; }
	inline void DefaultCharset( BmString s) 
													{ mDefaultCharset = s; }

	static const int16 nFirstTextCol;

private:
	void ShowMenu( BPoint point);

	static float nColWidths[10];
	float mColWidths[10];
	bool mShowAllParts;
	bool mEditable;
	bool mIsUsedForPrinting;
	BmString mDefaultCharset;
							// holds charset that will be used for dropped 
							// text-attachments (or UTF-8, depending on prefs).

	BFilePanel* mSavePanel;

	// Hide copy-constructor and assignment:
	BmBodyPartView( const BmBodyPartView&);
	BmBodyPartView operator=( const BmBodyPartView&);
};

#endif
