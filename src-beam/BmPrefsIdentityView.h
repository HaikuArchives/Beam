/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	BmRecvIdentItem( ColumnListView* lv, BmListModelItem* item);
	~BmRecvIdentItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
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
	// c'tors and d'tor:
	BmRecvIdentView( int32 width, int32 height);
	~BmRecvIdentView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "IdentView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

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
		BM_RECV_SELECTED 			= 'bmRS',
		BM_SIGNATURE_SELECTED 	= 'bmGS',
		BM_SMTP_SELECTED 			= 'bmSS',
		BM_IS_BUCKET_CHANGED	 	= 'bmFC',
		BM_SET_SPECIAL_HEADERS 	= 'bmSH',
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
	BmListViewController* mIdentListView;
	BmTextControl* mIdentityControl;
	BmTextControl* mMailAddrControl;
	BmTextControl* mRealNameControl;
	BmTextControl* mReplyToControl;
	MButton* mSpecialHeadersButton;
	BmMenuControl* mSignatureControl;
	BmMenuControl* mRecvControl;
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
