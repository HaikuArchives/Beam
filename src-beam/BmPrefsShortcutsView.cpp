/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Alert.h>

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif
#include <HGroup.h>
#include <LayeredGroup.h>
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmPrefsShortcutsView.h"
#include "BmResources.h"
#include "BmTextControl.h"
#include "BmUtil.h"


/********************************************************************************\
	BmShortcutControl
\********************************************************************************/

BmShortcutControl* BmShortcutControl::nTheInstance = NULL;

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
BmShortcutControl::BmShortcutControl( const char* label)
	: 	inherited( label)
{
	nTheInstance = this;
	TextView()->AddFilter( new BMessageFilter( B_ANY_DELIVERY, B_ANY_SOURCE, FilterHook));
}

/*------------------------------------------------------------------------------*\
	FilterHook()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmShortcutControl::FilterHook( BMessage* msg, BHandler** handler,
															BMessageFilter*) {
	if (msg->what == B_KEY_DOWN)
		*handler = BmShortcutControl::nTheInstance;
	return B_DISPATCH_MESSAGE;
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmShortcutControl::KeyDown(const char *bytes, int32 numBytes) { 
	if (numBytes == 1) {
		BMessage* currMsg = Window()->CurrentMessage();
		int32 rawKey = currMsg->FindInt32( "raw_char");
		BmString key;
		BmString legalKeys("abcdefghijklmnopqrstuvwxyz0123456789<>@#.,:;+-/*^");
		if (legalKeys.FindFirst((char)rawKey) != B_ERROR) {
			key.SetTo( (char)rawKey, 1);
			key.ToUpper();
		} else if (rawKey==B_RIGHT_ARROW)
			key = BmString("<RIGHT_ARROW>");
		else if (rawKey==B_LEFT_ARROW)
			key = BmString("<LEFT_ARROW>");
		else if (rawKey==B_UP_ARROW)
			key = BmString("<UP_ARROW>");
		else if (rawKey==B_DOWN_ARROW)
			key = BmString("<DOWN_ARROW>");
		if (key.Length()) {
			int32 mods = currMsg->FindInt32("modifiers");
			if (mods & B_SHIFT_KEY)
				key.Prepend("<SHIFT>");
			SetText( key.String());
		} else
			SetText( "");
	}
	BView::KeyDown( bytes, numBytes);
}



/********************************************************************************\
	BmPrefsShortcutsView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsShortcutsView::BmPrefsShortcutsView() 
	:	inherited( "Shortcuts (changes require a restart)")
{
	MView* view = 
		new VGroup(
			new BetterScrollView( 
				minimax(400,300,1E5,1E5), 
				CreateListView( 400, 400),
				BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
				| BM_SV_CAPTION,
				"99 shortcuts"
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Shortcut Info",
				new VGroup(
					mNameControl = new BmTextControl( "Menu-Item:"),
					new Space( minimax(0,5,0,5)),
					mShortcutControl = new BmShortcutControl( "Shortcut:"),
					0
				)
			),
			new Space(minimax(0,0,1E5,1E5,0.5)),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));

	BmDividable::DivideSame(
		mShortcutControl,
		mNameControl,
		NULL
	);
	mNameControl->SetEnabled( false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsShortcutsView::~BmPrefsShortcutsView() {
	TheBubbleHelper->SetHelp( mListView, NULL);
	TheBubbleHelper->SetHelp( mShortcutControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( mListView, "This listview contains all menu-items of Beam \nwith their current shortcuts.");
	TheBubbleHelper->SetHelp( mShortcutControl, "Here you can define the shortcut to be used for the currently selected item.\nJust type the shortcut you wish to use, but leave out the menu-key \n(i.e. <ALT> or <CTRL>), that is added automatically.");

	mShortcutControl->SetTarget( this);

	mListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mListView->SetTarget( this);
	Update();
	ShowShortcut( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::Update() {
	BAutolock lock( ThePrefs->Locker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Unable to get lock on Prefs!");
	BMessage* scMsg = ThePrefs->ShortcutsMsg();
	if (scMsg) {
		CLVEasyItem* item;
		type_code type;
#ifdef B_BEOS_VERSION_DANO
		const char* name;
#else
		char* name;
#endif
		mListView->MakeEmpty();
		for( int32 i=0; scMsg->GetInfo( B_STRING_TYPE, i, &name, &type)==B_OK; ++i) {
			item = new CLVEasyItem( 0, false, false, mListView);
			item->SetColumnContent( 0, name);
			item->SetColumnContent( 1, scMsg->FindString( name));
			mListView->AddItem( item);
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::SaveData() {
	// prefs are already stored by General View
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::UndoChanges() {
	// prefs are already undone by General View
	ShowShortcut( -1);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mListView->CurrentSelection( 0);
				ShowShortcut( index);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if (source == mShortcutControl) {
					BmString sc = mShortcutControl->Text();
					ThePrefs->SetShortcutFor( mNameControl->Text(), sc.String());
					int32 index = mListView->CurrentSelection( 0);
					if (index != -1) {
						CLVEasyItem* scItem = dynamic_cast<CLVEasyItem*>(mListView->ItemAt( index));
						if (scItem)
							scItem->SetColumnContent( 1, sc.String());
						mListView->InvalidateItem( index);
					}
					NoticeChange();
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsShortcutsView::ShowShortcut( int32 selection) {
	bool enabled = (selection != -1);
	mShortcutControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mShortcutControl->SetTextSilently( "");
		mNameControl->SetTextSilently( "");
	} else {
		CLVEasyItem* scItem = dynamic_cast<CLVEasyItem*>(mListView->ItemAt( selection));
		if (scItem) {
			BmString name = scItem->GetColumnContentText( 0);
			BmString sc = ThePrefs->GetShortcutFor( name.String());
			mNameControl->SetTextSilently( name.String());
			mShortcutControl->SetTextSilently( sc.String());
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
ColumnListView* BmPrefsShortcutsView::CreateListView( int32 width, int32 height) {
	mListView = new ColumnListView( BRect( 0, 0, width-1, height-1), NULL, 
											  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
											  B_SINGLE_SELECTION_LIST);

	mListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mListView->SetTarget( this);
	mListView->ClickSetsFocus( true);

	mListView->SetMinItemHeight( 
		MAX( TheResources->FontLineHeight(),
			  ThePrefs->GetInt( "ListviewFlatMinItemHeight", 16))
	);

	int32 flags = 0;
	if (ThePrefs->GetBool("StripedListView"))
		mListView->SetStripedBackground( true);

	mListView->AddColumn( new CLVColumn( "Menu-Item", 300.0, CLV_SORT_KEYABLE|flags, 100.0));
	mListView->AddColumn( new CLVColumn( "Shortcut", 80.0, CLV_SORT_KEYABLE|flags, 4.0));

	mListView->SetSortFunction( CLVEasyItem::CompareItems);
	mListView->SetSortKey( 0);

	return mListView;
}
