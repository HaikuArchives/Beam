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
	BmRecvAccItem( const BmString& key, BmListModelItem* item);
	~BmRecvAccItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);
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
	// creator-func, c'tors and d'tor:
	static BmRecvAccView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmRecvAccView(  minimax minmax, int32 width, int32 height);
	~BmRecvAccView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "RecvAccView"; }
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

	static BmRecvAccView* theInstance;

private:

	// Hide copy-constructor and assignment:
	BmRecvAccView( const BmRecvAccView&);
	BmRecvAccView operator=( const BmRecvAccView&);
};



#define BM_AUTH_SELECTED 			'bmAS'
#define BM_SIGNATURE_SELECTED 	'bmGS'
#define BM_SMTP_SELECTED 			'bmSS'
#define BM_FILTER_CHAIN_SELECTED 'bmFS'
#define BM_CHECK_MAIL_CHANGED 	'bmCC'
#define BM_CHECK_EVERY_CHANGED 	'bmCE'
#define BM_REMOVE_MAIL_CHANGED 	'bmRC'
#define BM_IS_DEFAULT_CHANGED 	'bmDC'
#define BM_IS_BUCKET_CHANGED	 	'bmFC'
#define BM_PWD_STORED_CHANGED 	'bmPC'
#define BM_CHECK_AND_SUGGEST		'bmCS'
#define BM_ADD_ACCOUNT 				'bmAA'
#define BM_REMOVE_ACCOUNT 			'bmRA'


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
	CLVContainerView* CreateAccListView( minimax minmax, int32 width, int32 height);

	BmListViewController* mAccListView;
	BmTextControl* mAccountControl;
	BmTextControl* mAliasesControl;
	BmTextControl* mLoginControl;
	BmTextControl* mMailAddrControl;
	BmTextControl* mPortControl;
	BmTextControl* mPwdControl;
	BmTextControl* mRealNameControl;
	BmTextControl* mServerControl;
	BmTextControl* mCheckIntervalControl;
	BmMenuControl* mAuthControl;
	BmMenuControl* mSignatureControl;
	BmMenuControl* mSmtpControl;
	BmMenuControl* mFilterChainControl;
	BmCheckControl* mCheckAccountControl;
	BmCheckControl* mIsBucketControl;
	BmCheckControl* mIsDefaultControl;
	BmCheckControl* mRemoveMailControl;
	BmCheckControl* mStorePwdControl;
	BmCheckControl* mCheckEveryControl;
	MButton* mCheckAndSuggestButton;
	MButton* mAddButton;
	MButton* mRemoveButton;
	MStringView* mMinutesLabel;

	BmRef<BmPopAccount> mCurrAcc;
	
	static const BmString nEmptyItemLabel;

	// Hide copy-constructor and assignment:
	BmPrefsRecvMailView( const BmPrefsRecvMailView&);
	BmPrefsRecvMailView operator=( const BmPrefsRecvMailView&);
};

#endif
