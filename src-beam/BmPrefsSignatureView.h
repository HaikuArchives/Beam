/*
	BmPrefsSignatureView.h
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


#ifndef _BmPrefsSignatureView_h
#define _BmPrefsSignatureView_h

#include "BmListController.h"
#include "BmSignature.h"
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmSignatureItem
		-	
\*------------------------------------------------------------------------------*/
class BmSignatureItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmSignatureItem( ColumnListView* lv, const BmString& key, 
						  BmListModelItem* item);
	~BmSignatureItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);

private:
	// Hide copy-constructor and assignment:
	BmSignatureItem( const BmSignatureItem&);
	BmSignatureItem operator=( const BmSignatureItem&);
};



/*------------------------------------------------------------------------------*\
	BmSignatureView
		-	
\*------------------------------------------------------------------------------*/
class BmSignatureView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmSignatureView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmSignatureView(  minimax minmax, int32 width, int32 height);
	~BmSignatureView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "SignatureView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "signature"; }
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, 
												  border_style border, 
												  uint32 ResizingMode, 
												  uint32 flags);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

	static BmSignatureView* theInstance;

private:

	// Hide copy-constructor and assignment:
	BmSignatureView( const BmSignatureView&);
	BmSignatureView operator=( const BmSignatureView&);
};




class BmMultiLineTextControl;
class BmTextControl;
class BmCheckControl;
class BmMenuControl;
class MButton;
/*------------------------------------------------------------------------------*\
	BmPrefsSignatureView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsSignatureView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_DYNAMIC_CHANGED 		= 'bmDC',
		BM_CHARSET_SELECTED 		= 'bmCS',
		BM_ADD_SIGNATURE			= 'bmAS',
		BM_REMOVE_SIGNATURE		= 'bmRS',
		BM_TEST_SIGNATURE			= 'bmTS'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsSignatureView();
	virtual ~BmPrefsSignatureView();
	
	// native methods:
	void ShowSignature( int32 selection);

	// overrides of BmPrefsView base:
	void Initialize();
	void Activated();
	void WriteStateInfo();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:

	// setters:

private:
	CLVContainerView* CreateSigListView( minimax minmax, int32 width, int32 height);

	BmListViewController* mSigListView;
	BmTextControl* mSignatureControl;
	BmTextControl* mSignatureRxControl;
	BmMultiLineTextControl* mContentControl;
	BmMenuControl* mCharsetControl;
	BmCheckControl* mDynamicControl;
	MButton* mAddButton;
	MButton* mRemoveButton;
	MButton* mTestButton;

	BmRef<BmSignature> mCurrSig;
	
	// Hide copy-constructor and assignment:
	BmPrefsSignatureView( const BmPrefsSignatureView&);
	BmPrefsSignatureView operator=( const BmPrefsSignatureView&);
};

#endif
