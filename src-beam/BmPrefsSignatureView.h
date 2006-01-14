/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	BmSignatureItem( ColumnListView* lv, BmListModelItem* item);
	~BmSignatureItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);

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
	// c'tors and d'tor:
	BmSignatureView( int32 width, int32 height);
	~BmSignatureView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "SignatureView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "signature"; }

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

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
	BmListViewController* mSigListView;
	BmTextControl* mSignatureControl;
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
