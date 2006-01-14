/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	BmSendAccItem( ColumnListView* lv, BmListModelItem* item);
	~BmSendAccItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
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
	// c'tors and d'tor:
	BmSendAccView( int32 width, int32 height);
	~BmSendAccView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "SendAccView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "account"; }

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

private:

	// Hide copy-constructor and assignment:
	BmSendAccView( const BmSendAccView&);
	BmSendAccView operator=( const BmSendAccView&);
};



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

	enum {
		BM_AUTH_SELECTED 			= 'bmAS',
		BM_ENCRYPTION_SELECTED 		= 'bmES',
		BM_POP_SELECTED 			= 'bmPS',
		BM_PWD_STORED_CHANGED 	= 'bmPC',
		BM_CHECK_AND_SUGGEST		= 'bmCS',
		BM_ADD_ACCOUNT 			= 'bmAA',
		BM_REMOVE_ACCOUNT 		= 'bmRA'
	};


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
	BmListViewController* mAccListView;
	BmTextControl* mAccountControl;
	BmTextControl* mDomainControl;
	BmTextControl* mLoginControl;
	BmTextControl* mPortControl;
	BmTextControl* mPwdControl;
	BmTextControl* mServerControl;
	BmMenuControl* mEncryptionControl;
	BmMenuControl* mAuthControl;
	BmMenuControl* mPopControl;
	BmCheckControl* mStorePwdControl;
	MButton* mCheckAndSuggestButton;
	MButton* mAddButton;
	MButton* mRemoveButton;

	BmRef<BmSmtpAccount> mCurrAcc;
	
	// Hide copy-constructor and assignment:
	BmPrefsSendMailView( const BmPrefsSendMailView&);
	BmPrefsSendMailView operator=( const BmPrefsSendMailView&);
};

#endif
