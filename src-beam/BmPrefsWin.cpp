/*
	BmPrefsWin.cpp
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


#include <Autolock.h>
#include <MenuField.h>
#include <Font.h>
#include <Message.h>
#include "BmString.h"

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif

#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "UserResizeSplitView.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmPrefsView.h"
#include "BmPrefsFilterView.h"
#include "BmPrefsFilterChainView.h"
#include "BmPrefsGeneralView.h"
#include "BmPrefsIdentityView.h"
#include "BmPrefsLoggingView.h"
#include "BmPrefsMailConstrView.h"
#include "BmPrefsMailReadView.h"
#include "BmPrefsRecvMailView.h"
#include "BmPrefsSendMailView.h"
#include "BmPrefsShortcutsView.h"
#include "BmPrefsSignatureView.h"
#include "BmPrefsWin.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsWin
\********************************************************************************/

enum {
	BM_APPLY_CHANGES   = 'bmWA',
	BM_REVERT_CHANGES  = 'bmWR',
	BM_SET_DEFAULTS    = 'bmWD'
};

BmPrefsWin* BmPrefsWin::theInstance = NULL;

const char* const BmPrefsWin::MSG_VSPLITTER = 	"bm:vspl";

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates the app's main window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmPrefsWin* BmPrefsWin::CreateInstance() 
{
	if (!theInstance) {
		theInstance = new BmPrefsWin;
		theInstance->ReadStateInfo();
	}
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsWin::BmPrefsWin()
	:	inherited( "PrefsWin", BRect(50,50,549,449),
					  "Beam Preferences",
					  B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					  B_ASYNCHRONOUS_CONTROLS)
	,	mPrefsListView( NULL)
	,	mPrefsViewContainer( NULL)
	,	mVertSplitter( NULL)
	,	mChanged( false)
{
	MView* mOuterGroup = 
		new VGroup( 
			minimax( 700, 550),
			new MBorder( M_RAISED_BORDER, 5, NULL,
				new HGroup(
					minimax(500,200),
					mVertSplitter = new UserResizeSplitView(
						new VGroup(
							minimax(100,200,200,1E5),
							CreatePrefsListView( minimax(100,200,200,1E5), 120, 200),
							new Space( minimax(0,2,0,2)),
							0
						),
						mPrefsViewContainer = new BmPrefsViewContainer(
							new LayeredGroup( 
								new BmPrefsView( NULL),
								new BmPrefsGeneralView(),
								new BmPrefsShortcutsView(),
								new BmPrefsLoggingView(),
								new BmPrefsMailConstrView(),
								new BmPrefsSendMailView(),
								new BmPrefsMailReadView(),
								new BmPrefsRecvMailView(),
								new BmPrefsIdentityView(),
								new BmPrefsSignatureView(),
								new BmPrefsFilterView(),
								new BmPrefsFilterChainView(),
								0
							)
						),
						"vsplitter", 120, B_VERTICAL, true, true, false, B_FOLLOW_NONE
					),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new Space(),
				mDefaultsButton = new MButton( "Defaults", new BMessage(BM_SET_DEFAULTS), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(40,0,40,0)),
				mRevertButton = new MButton( "Revert", new BMessage(BM_REVERT_CHANGES), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(20,0,20,0)),
				mApplyButton = new MButton( "Apply", new BMessage(BM_APPLY_CHANGES), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(20,0,20,0)),
				0
			),
			new Space( minimax(0,10,0,10)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));
	mApplyButton->SetEnabled( false);
	mRevertButton->SetEnabled( false);
	mPrefsListView->Select( 0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsWin::~BmPrefsWin() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmPrefsWin::ArchiveState( BMessage* archive) const {
	status_t ret = inherited::ArchiveState( archive)
						|| archive->AddFloat( MSG_VSPLITTER, mVertSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmPrefsWin::UnarchiveState( BMessage* archive) {
	float vDividerPos;
	status_t ret = inherited::UnarchiveState( archive)
						|| archive->FindFloat( MSG_VSPLITTER, &vDividerPos);
	if (ret == B_OK) {
		mVertSplitter->SetPreferredDividerLeftOrTop( vDividerPos);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsWin::CreatePrefsListView( minimax minmax, int32 width, int32 height) {
	mPrefsListView = new ColumnListView( minmax, BRect( 0, 0, width-1, height-1), NULL, 
													 B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
													 B_SINGLE_SELECTION_LIST, true, true);

	BFont font(*be_bold_font);
	mPrefsListView->SetFont( &font);
	mPrefsListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mPrefsListView->SetTarget( this);
	mPrefsListView->ClickSetsFocus( true);
	CLVContainerView* container 
		= mPrefsListView->Initialize( BRect( 0,0,width-1,height-1), 
												B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
												B_FOLLOW_TOP_BOTTOM, false, true, true, B_FANCY_BORDER,
												be_bold_font);
	mPrefsListView->AddColumn( 
		new CLVColumn( NULL, 10.0, 
							CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 10.0));
	mPrefsListView->AddColumn( 
		new CLVColumn( "   Category", 300.0, 
							CLV_NOT_MOVABLE|CLV_NOT_RESIZABLE|CLV_TELL_ITEMS_WIDTH, 300.0));

	CLVEasyItem* item;	
	item = new CLVEasyItem( 0, false, false, 18.0);
	item->SetColumnContent( 1, "General");
	mPrefsListView->AddItem( item);
	mPrefsListView->Expand( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Shortcuts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Logging");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, 18.0);
	item->SetColumnContent( 1, "Sending Mail");
	mPrefsListView->AddItem( item);
	mPrefsListView->Expand( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Accounts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, 18.0);
	item->SetColumnContent( 1, "Receiving Mail");
	mPrefsListView->AddItem( item);
	mPrefsListView->Expand( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Accounts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, 18.0);
	item->SetColumnContent( 1, "Identities");
	mPrefsListView->AddItem( item);
	mPrefsListView->Expand( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Signatures");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, 18.0);
	item->SetColumnContent( 1, "Filtering Mail");
	mPrefsListView->AddItem( item);
	mPrefsListView->Expand( item);

	item = new CLVEasyItem( 1, false, false, 18.0);
	item->SetColumnContent( 1, "Chains");
	mPrefsListView->AddItem( item);

	return container;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::SendMsgToSubView( const BmString& subViewName, BMessage* msg) {
	BmPrefsView* pv = mPrefsViewContainer->ShowPrefsByName( subViewName);
	if (pv) {
		PostMessage( msg, pv);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mPrefsListView->CurrentSelection( 0);
				int32 fullIndex = mPrefsListView->FullListIndexOf( 
					(CLVListItem*)mPrefsListView->ItemAt( index)
				);
				mPrefsViewContainer->ShowPrefs( 1+fullIndex);
				break;
			}
			case BM_PREFS_CHANGED: {
				mChanged = true;
				mApplyButton->SetEnabled( true);
				mRevertButton->SetEnabled( true);
				break;
			}
			case BM_APPLY_CHANGES: {
				if (mPrefsViewContainer->ApplyChanges()) {
					mApplyButton->SetEnabled( false);
					mRevertButton->SetEnabled( false);
					mChanged = false;
				}
				break;
			}
			case BM_REVERT_CHANGES: {
				mPrefsViewContainer->RevertChanges();
				mApplyButton->SetEnabled( false);
				mRevertButton->SetEnabled( false);
				mChanged = false;
				break;
			}
			case BM_SET_DEFAULTS: {
				mPrefsViewContainer->SetDefaults();
				mApplyButton->SetEnabled( true);
				mRevertButton->SetEnabled( true);
				mChanged = true;
				break;
			}
			case BMM_PREFERENCES: {
				BmString subViewName = msg->FindString( "SubViewName");
				if (subViewName.Length()) {
					BMessage subViewMsg;
					if (msg->FindMessage( "SubViewMsg", &subViewMsg) == B_OK)
						SendMsgToSubView( subViewName, &subViewMsg);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmPrefsWin::QuitRequested() {
	BM_LOG2( BM_LogApp, BmString("PrefsWin has been asked to quit"));
	if (mChanged) {
		if (IsMinimized())
			Minimize( false);
		Activate();
		BAlert* alert = new BAlert( "title", "The preferences have changed. Would you like to save changes before closing?",
											 "Cancel", "Don't Save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut( 0, B_ESCAPE);
		int32 result = alert->Go();
		switch( result) {
			case 0:
				return false;
			case 1:
				mPrefsViewContainer->RevertChanges();
				return true;
			case 2:
				return mPrefsViewContainer->ApplyChanges();
		}
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmPrefsWin::Quit() {
	mPrefsViewContainer->WriteStateInfo();
	BM_LOG2( BM_LogApp, BmString("PrefsWin has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	PrefsListSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::PrefsListSelectionChanged( int32) {
}
