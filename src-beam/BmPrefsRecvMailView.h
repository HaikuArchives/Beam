/*
	BmPrefsRecvMailView.h
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


#ifndef _BmPrefsRecvMailView_h
#define _BmPrefsRecvMailView_h

#include "BmListController.h"
#include "BmPopAccount.h"
#define BmRecvAcc BmPopAccount
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmRecvAccItem
		-	
\*------------------------------------------------------------------------------*/
class BmRecvAccItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmRecvAccItem( ColumnListView* lv, BmListModelItem* item);
	~BmRecvAccItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
	BmRecvAcc* ModelItem() const 			{ return dynamic_cast< BmRecvAcc*>( mModelItem.Get()); }

private:
	// Hide copy-constructor and assignment:
	BmRecvAccItem( const BmRecvAccItem&);
	BmRecvAccItem operator=( const BmRecvAccItem&);
};



/*------------------------------------------------------------------------------*\
	BmRecvAccView
		-	
\*------------------------------------------------------------------------------*/
class BmRecvAccView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// c'tors and d'tor:
	BmRecvAccView( int32 width, int32 height);
	~BmRecvAccView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "RecvAccView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "account"; }

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

private:

	// Hide copy-constructor and assignment:
	BmRecvAccView( const BmRecvAccView&);
	BmRecvAccView operator=( const BmRecvAccView&);
};




class BmTextControl;
class BmMenuControl;
class BmCheckControl;
class MButton;
class MStringView;
/*------------------------------------------------------------------------------*\
	BmPrefsRecvMailView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsRecvMailView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_AUTH_SELECTED 			 	= 'bmAS',
		BM_FILTER_CHAIN_SELECTED 	= 'bmFS',
		BM_HOME_FOLDER_SELECTED  	= 'bmHS',
		BM_CHECK_MAIL_CHANGED 	 	= 'bmCC',
		BM_CHECK_EVERY_CHANGED 	 	= 'bmCE',
		BM_REMOVE_MAIL_CHANGED 	 	= 'bmRC',
		BM_PWD_STORED_CHANGED 	 	= 'bmPC',
		BM_CHECK_AND_SUGGEST		 	= 'bmCS',
		BM_ADD_ACCOUNT 			 	= 'bmAA',
		BM_REMOVE_ACCOUNT 		 	= 'bmRA',
		BM_CHECK_IF_PPP_UP_CHANGED = 'bmCP'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsRecvMailView();
	virtual ~BmPrefsRecvMailView();
	
	// native methods:
	void ShowAccount( int32 selection);

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
	BmListViewController* mAccListView;
	BmTextControl* mAccountControl;
	BmTextControl* mLoginControl;
	BmTextControl* mPortControl;
	BmTextControl* mPwdControl;
	BmTextControl* mServerControl;
	BmTextControl* mCheckIntervalControl;
	BmTextControl* mDeleteMailDelayControl;
	BmMenuControl* mAuthControl;
	BmMenuControl* mFilterChainControl;
	BmMenuControl* mHomeFolderControl;
	BmCheckControl* mCheckAccountControl;
	BmCheckControl* mRemoveMailControl;
	BmCheckControl* mStorePwdControl;
	BmCheckControl* mCheckEveryControl;
	BmCheckControl* mAutoCheckIfPppUpControl;

	MButton* mCheckAndSuggestButton;
	MButton* mAddButton;
	MButton* mRemoveButton;
	MStringView* mMinutesLabel;
	MStringView* mDaysLabel;

	BmRef<BmPopAccount> mCurrAcc;
	
	// Hide copy-constructor and assignment:
	BmPrefsRecvMailView( const BmPrefsRecvMailView&);
	BmPrefsRecvMailView operator=( const BmPrefsRecvMailView&);
};

#endif
