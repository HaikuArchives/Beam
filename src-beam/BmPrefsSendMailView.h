/*
	BmPrefsSendMailView.h
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


#ifndef _BmPrefsSendMailView_h
#define _BmPrefsSendMailView_h

#include "BmListController.h"
#include "BmSmtpAccount.h"
#define BmSendAcc BmSmtpAccount
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmSendAccItem
		-	
\*------------------------------------------------------------------------------*/
class BmSendAccItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmSendAccItem( BmString key, BmListModelItem* item);
	~BmSendAccItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);
	BmSendAcc* ModelItem() const 			{ return dynamic_cast< BmSendAcc*>( mModelItem.Get()); }

private:
	// Hide copy-constructor and assignment:
	BmSendAccItem( const BmSendAccItem&);
	BmSendAccItem operator=( const BmSendAccItem&);
};



/*------------------------------------------------------------------------------*\
	BmSendAccView
		-	
\*------------------------------------------------------------------------------*/
class BmSendAccView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmSendAccView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmSendAccView(  minimax minmax, int32 width, int32 height);
	~BmSendAccView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "SendAccView"; }
	void UpdateModelItem( BMessage* msg);
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "account"; }
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, 
												  border_style border, 
												  uint32 ResizingMode, 
												  uint32 flags);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

	static BmSendAccView* theInstance;

private:

	// Hide copy-constructor and assignment:
	BmSendAccView( const BmSendAccView&);
	BmSendAccView operator=( const BmSendAccView&);
};



#define BM_AUTH_SELECTED 			'bmAS'
#define BM_POP_SELECTED 			'bmPS'
#define BM_PWD_STORED_CHANGED 	'bmPC'
#define BM_CHECK_AND_SUGGEST		'bmCS'
#define BM_ADD_ACCOUNT 				'bmAA'
#define BM_REMOVE_ACCOUNT 			'bmRA'


class BmTextControl;
class BmMenuControl;
class BmCheckControl;
class MButton;
/*------------------------------------------------------------------------------*\
	BmPrefsSendMailView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsSendMailView : public BmPrefsView {
	typedef BmPrefsView inherited;

public:
	// c'tors and d'tor:
	BmPrefsSendMailView();
	virtual ~BmPrefsSendMailView();
	
	// native methods:
	void ShowAccount( int32 selection);
	void UpdateState();

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
	CLVContainerView* CreateAccListView( minimax minmax, int32 width, int32 height);

	BmListViewController* mAccListView;
	BmTextControl* mAccountControl;
	BmTextControl* mDomainControl;
	BmTextControl* mLoginControl;
	BmTextControl* mPortControl;
	BmTextControl* mPwdControl;
	BmTextControl* mServerControl;
	BmMenuControl* mAuthControl;
	BmMenuControl* mPopControl;
	BmCheckControl* mStorePwdControl;
	MButton* mCheckAndSuggestButton;
	MButton* mAddButton;
	MButton* mRemoveButton;

	BmRef<BmSmtpAccount> mCurrAcc;
	
	static const BmString nEmptyItemLabel;

	// Hide copy-constructor and assignment:
	BmPrefsSendMailView( const BmPrefsSendMailView&);
	BmPrefsSendMailView operator=( const BmPrefsSendMailView&);
};

#endif
