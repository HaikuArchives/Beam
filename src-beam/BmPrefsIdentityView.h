/*
	BmPrefsIdentityView.h
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


#ifndef _BmPrefsIdentityView_h
#define _BmPrefsIdentityView_h

#include "BmListController.h"
#include "BmIdentity.h"
#define BmRecvIdent BmIdentity
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmRecvIdentItem
		-	
\*------------------------------------------------------------------------------*/
class BmRecvIdentItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmRecvIdentItem( ColumnListView* lv, const BmString& key, 
						  BmListModelItem* item);
	~BmRecvIdentItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);
	BmRecvIdent* ModelItem() const 			{ return dynamic_cast< BmRecvIdent*>( mModelItem.Get()); }

private:
	// Hide copy-constructor and assignment:
	BmRecvIdentItem( const BmRecvIdentItem&);
	BmRecvIdentItem operator=( const BmRecvIdentItem&);
};



/*------------------------------------------------------------------------------*\
	BmRecvIdentView
		-	
\*------------------------------------------------------------------------------*/
class BmRecvIdentView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmRecvIdentView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmRecvIdentView(  minimax minmax, int32 width, int32 height);
	~BmRecvIdentView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "IdentView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, 
												  border_style border, 
												  uint32 ResizingMode, 
												  uint32 flags);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

	static BmRecvIdentView* theInstance;

private:

	// Hide copy-constructor and assignment:
	BmRecvIdentView( const BmRecvIdentView&);
	BmRecvIdentView operator=( const BmRecvIdentView&);
};




class BmTextControl;
class BmMenuControl;
class BmCheckControl;
class MButton;
/*------------------------------------------------------------------------------*\
	BmPrefsIdentityView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsIdentityView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_POP_SELECTED 			= 'bmPS',
		BM_SIGNATURE_SELECTED 	= 'bmGS',
		BM_SMTP_SELECTED 			= 'bmSS',
		BM_IS_BUCKET_CHANGED	 	= 'bmFC',
		BM_ADD_IDENTITY 			= 'bmAI',
		BM_REMOVE_IDENTITY 		= 'bmRI'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsIdentityView();
	virtual ~BmPrefsIdentityView();
	
	// native methods:
	void ShowIdentity( int32 selection);

	// overrides of BmPrefsView base:
	void Initialize();
	void Activated();
	void WriteStateInfo();
	void SaveData();
	bool SanityCheck();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:

	// setters:

private:
	CLVContainerView* CreateIdentListView( minimax minmax, int32 width, int32 height);

	BmListViewController* mIdentListView;
	BmTextControl* mIdentityControl;
	BmTextControl* mMailAddrControl;
	BmTextControl* mRealNameControl;
	BmMenuControl* mSignatureControl;
	BmMenuControl* mPopControl;
	BmMenuControl* mSmtpControl;
	BmCheckControl* mIsBucketControl;
	BmTextControl* mAliasesControl;
	MButton* mAddButton;
	MButton* mRemoveButton;

	BmRef<BmIdentity> mCurrIdent;
	
	// Hide copy-constructor and assignment:
	BmPrefsIdentityView( const BmPrefsIdentityView&);
	BmPrefsIdentityView operator=( const BmPrefsIdentityView&);
};

#endif
