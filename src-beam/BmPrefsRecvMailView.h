/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsRecvMailView_h
#define _BmPrefsRecvMailView_h

#include "BmListController.h"
#include "BmRecvAccount.h"
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
	BmRecvAccount* ModelItem() const { 
		return dynamic_cast< BmRecvAccount*>( mModelItem.Get()); 
	}

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
class BFilePanel;
/*------------------------------------------------------------------------------*\
	BmPrefsRecvMailView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsRecvMailView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_AUTH_SELECTED 			 	= 'bmAS',
		BM_ENCRYPTION_SELECTED		= 'bmES',
		BM_FILTER_CHAIN_SELECTED 	= 'bmFS',
		BM_HOME_FOLDER_SELECTED  	= 'bmHS',
		BM_CHECK_MAIL_CHANGED 	 	= 'bmCC',
		BM_CHECK_EVERY_CHANGED 	 	= 'bmCE',
		BM_REMOVE_MAIL_CHANGED 	 	= 'bmRC',
		BM_PWD_STORED_CHANGED 	 	= 'bmPC',
		BM_CHECK_AND_SUGGEST		 	= 'bmCS',
		BM_ADD_POP_ACCOUNT 			= 'bmAP',
		BM_ADD_IMAP_ACCOUNT 			= 'bmAI',
		BM_REMOVE_ACCOUNT 		 	= 'bmRA',
		BM_CHECK_IF_PPP_UP_CHANGED = 'bmCP',
		BM_CLIENT_CERT_SELECTED 	= 'bmCT',
		BM_CLEAR_ACCEPTED_CERT  	= 'bmCA'
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
	void AuthTypeSelected();
	void EncryptionSelected();

	BmListViewController* mAccListView;
	BmTextControl* mAccountControl;
	BmTextControl* mLoginControl;
	BmTextControl* mPortControl;
	BmTextControl* mPwdControl;
	BmTextControl* mServerControl;
	BmTextControl* mCheckIntervalControl;
	BmTextControl* mDeleteMailDelayControl;
	BmTextControl* mClientCertControl;
	BmTextControl* mAcceptedCertControl;
	BmMenuControl* mEncryptionControl;
	BmMenuControl* mAuthControl;
	BmMenuControl* mFilterChainControl;
	BmMenuControl* mHomeFolderControl;
	BmCheckControl* mCheckAccountControl;
	BmCheckControl* mRemoveMailControl;
	BmCheckControl* mStorePwdControl;
	BmCheckControl* mCheckEveryControl;
	BmCheckControl* mAutoCheckIfPppUpControl;

	MButton* mCheckAndSuggestButton;
	MButton* mAddPopButton;
	MButton* mAddImapButton;
	MButton* mRemoveButton;
	MButton* mSelectClientCertButton;
	MButton* mClearAcceptedCertButton;
	MStringView* mMinutesLabel;
	MStringView* mDaysLabel;

	BFilePanel* mClientCertPanel;

	BmRef<BmRecvAccount> mCurrAcc;
	
	// Hide copy-constructor and assignment:
	BmPrefsRecvMailView( const BmPrefsRecvMailView&);
	BmPrefsRecvMailView operator=( const BmPrefsRecvMailView&);
};

#endif
