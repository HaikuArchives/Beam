/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Alert.h>
#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif
#include <HGroup.h>
#include <LayeredGroup.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BetterButton.h"
#include "BubbleHelper.h"
#include "Colors.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMenuControl.h"
#include "BmPrefs.h"
#include "BmPrefsGeneralView.h"
#include "BmResources.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsGeneralView
\********************************************************************************/

const char* const BmPrefsGeneralView::MSG_WORKSPACE = "wspace";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::BmPrefsGeneralView() 
	:	inherited( "General options")
	,	mMailboxPanel( NULL)
	,	mIconboxPanel( NULL)
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"General GUI options",
					new VGroup(
						mWorkspaceControl = new BmMenuControl( 
							"Workspace to start in:", 
							new BPopUpMenu("")
						),
						new Space( minimax(0,10,0,10)),
						mListviewLikeTrackerControl = new BmCheckControl( 
							"Simulate Tracker's list view", 
					 		new BMessage(BM_LISTVIEW_LIKE_TRACKER_CHANGED), 
					 		this, ThePrefs->GetBool("ListviewLikeTracker", false)
					 	),
						new Space( minimax(0,10,0,10)),
						mShowTooltipsControl = new BmCheckControl( 
							"Show tooltips for toolbar buttons and prefs", 
							new BMessage(BM_SHOW_TOOLTIPS_CHANGED), 
							this, ThePrefs->GetBool("ShowTooltips", true)
						),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Toolbar options",
					new VGroup(
						mToolbarLabelControl = new BmMenuControl( 
							"Layout of labels in toolbar:", 
							new BPopUpMenu("")
						),
						new Space( minimax(0,10,0,10)),
						new HGroup(
							mIconboxButton = new BetterButton( 
								IconboxButtonLabel().String(), 
								new BMessage( BM_SELECT_ICONBOX), 
								this, minimax(-1,-1,-1,-1)
							),
							new Space(),
							0
						),
						0
					)
				),
				0
			),
			new HGroup( 
				mMailboxButton = new BetterButton( 
					MailboxButtonLabel().String(), 
					new BMessage( BM_SELECT_MAILBOX), 
					this, minimax(-1,-1,-1,-1)
				),
				new Space(),
				0
			),
			new Space( minimax(10,0,10,0)),
			new HGroup( 
				new BetterButton( 
					"Make Beam preferred app for email" B_UTF8_ELLIPSIS, 
					new BMessage( BM_MAKE_BEAM_STD_APP), 
					this, minimax(-1,-1,-1,-1)
				),
				new Space(),
				0
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::~BmPrefsGeneralView() {
	delete mIconboxPanel;
	delete mMailboxPanel;
	TheBubbleHelper->SetHelp( mWorkspaceControl, NULL);
	TheBubbleHelper->SetHelp( mListviewLikeTrackerControl, NULL);
	TheBubbleHelper->SetHelp( mShowTooltipsControl, NULL);
	TheBubbleHelper->SetHelp( mToolbarLabelControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsGeneralView::MailboxButtonLabel() {
	BmString label( "Set mailbox folder (currently '");
	label << ThePrefs->GetString( "MailboxPath") << "')" B_UTF8_ELLIPSIS;
	return label;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsGeneralView::IconboxButtonLabel() {
	BmString label( "Select icon folder (currently '");
	BmString iconPath = ThePrefs->GetString("IconPath");
	int32 pos = iconPath.FindLast("/");
	if (pos >= B_OK)
		iconPath.Remove(0,pos+1);
	label << iconPath << "')" B_UTF8_ELLIPSIS;
	return label;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::Initialize() {
	inherited::Initialize();
	
	TheBubbleHelper->SetHelp( 
		mWorkspaceControl, 
		"In this menu you can select the workspace that Beam should live in."
	);
	TheBubbleHelper->SetHelp( 
		mListviewLikeTrackerControl, 
		"If checked, Beam's list views will behave like the one's in Tracker:\n"
		"Shift-click toggles selection, Option-click expands selection."
	);
	TheBubbleHelper->SetHelp( 
		mShowTooltipsControl, 
		"Checking this makes Beam show a small \n"
		"info window (just like this one) when the \n"
		"mouse pointer hovers over a GUI item."
	);
	TheBubbleHelper->SetHelp( 
		mToolbarLabelControl, 
		"Here you can select if and where the label\n"
		"shall be shown inside a toolbar button"
	);
	
	// add workspaces:
	int32 count = count_workspaces();
	for( int32 i=0; i<=count; ++i) {
		BMessage* msg = new BMessage( BM_WORKSPACE_SELECTED);
		msg->AddInt32( MSG_WORKSPACE, i);
		BmString label;
		if (i)
			label << i;
		else
			label = "Current";
		AddItemToMenu( 
			mWorkspaceControl->Menu(), 
			new BMenuItem( label.String(), msg), 
			this
		);
	}

	// add label-layouts:
	const char* layouts[] = {
		"Left",
		"Right",
		"Top",
		"Bottom",
		"Hide",
		NULL
	};
	for( int32 i=0; layouts[i]; ++i) {
		BMessage* msg = new BMessage( BM_TOOLBAR_LABEL_SELECTED);
		AddItemToMenu( 
			mToolbarLabelControl->Menu(), 
			new BMenuItem( layouts[i], msg), 
			this
		);
	}

	Update();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::Update() {
	mWorkspaceControl->MarkItem( 
		ThePrefs->GetString("Workspace", "Current").String()
	);
	mToolbarLabelControl->MarkItem( 
		ThePrefs->GetString("ShowToolbarLabel", "Bottom").String()
	);
	mListviewLikeTrackerControl->SetValueSilently( 
		ThePrefs->GetBool("ListviewLikeTracker", false)
	);
	mShowTooltipsControl->SetValueSilently( 
		ThePrefs->GetBool("ShowTooltips", true)
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::WriteStateInfo() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::SaveData() {
	ThePrefs->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::UndoChanges() {
	ThePrefs->ResetToSaved();
	TheResources->InitializeWithPrefs();
	TheToolbarManager->UpdateAll();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::SetDefaults() {
	ThePrefs->ResetToDefault();
	TheResources->InitializeWithPrefs();
	TheToolbarManager->UpdateAll();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_LISTVIEW_LIKE_TRACKER_CHANGED: {
				bool b = mListviewLikeTrackerControl->Value();
				ThePrefs->SetBool("ListviewLikeTracker", b);
				ColumnListView::SetExtendedSelectionPolicy( b);
				NoticeChange();
				break;
			}
			case BM_SHOW_TOOLTIPS_CHANGED: {
				ThePrefs->SetBool("ShowTooltips", mShowTooltipsControl->Value());
				TheBubbleHelper->EnableHelp( mShowTooltipsControl->Value());
				NoticeChange();
				break;
			}
			case BM_TOOLBAR_LABEL_SELECTED: {
				BMenuItem* item = mToolbarLabelControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetString( "ShowToolbarLabel", item->Label());
					TheToolbarManager->UpdateAll();
					NoticeChange();
				}
			}
			case BM_WORKSPACE_SELECTED: {
				BMenuItem* item = mWorkspaceControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetString( "Workspace", item->Label());
					NoticeChange();
				}
				break;
			}
			case BM_MAKE_BEAM_STD_APP: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( 
						"Set Beam as preferred app", 
						"This will make Beam the preferred application for "
							"the following mimetypes:\n\n"
							"\tEmail (text/x-email)\n"
							"\tInternet-messages (message/rfc822).",
						"OK, do it", "Cancel", NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( 
						new BInvoker( 
							new BMessage(BM_MAKE_BEAM_STD_APP), 
							BMessenger( this)
						)
					);
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						app_info appInfo;
						if (be_app->GetAppInfo( &appInfo) != B_OK)
							break;
						BMimeType mt("text/x-email");
						if (mt.InitCheck() == B_OK)
							mt.SetPreferredApp( appInfo.signature);
						mt.SetTo("message/rfc822");
						if (mt.InitCheck() == B_OK)
							mt.SetPreferredApp( appInfo.signature);
						NoticeChange();
						BAlert* alert = new BAlert( 
							"Set Beam as preferred app", 
							"Done, Beam is now the preferred application for email.",
							"Good", NULL, NULL, B_WIDTH_AS_USUAL,
							B_INFO_ALERT
						);
						alert->Go();
					}
				}
				break;
			}
			case BM_SELECT_MAILBOX: {
				entry_ref mailboxRef;
				if (msg->FindRef( "refs", 0, &mailboxRef) != B_OK) {
					// first step, let user select new mailbox:
					if (!mMailboxPanel) {
						entry_ref eref;
						status_t err = get_ref_for_path(
							ThePrefs->GetString("MailboxPath").String(),	&eref
						);
						mMailboxPanel = new BFilePanel( 
							B_OPEN_PANEL, new BMessenger(this), 
							err == B_OK ? &eref : NULL, B_DIRECTORY_NODE, false, msg
						);
					}
					mMailboxPanel->Show();
				} else {
					// second step, set mailbox accordingly:
					BEntry entry( &mailboxRef);
					BPath path;
					if (entry.GetPath( &path) == B_OK) {
						ThePrefs->SetString( "MailboxPath", path.Path());
						mMailboxButton->SetLabel( MailboxButtonLabel().String());
						TheMailFolderList->MailboxPathHasChanged( true);
						NoticeChange();
						BAlert* alert = new BAlert( 
							"Mailbox path", 
							"Done, Beam will use the new mailbox after a restart",
							"OK", NULL, NULL, B_WIDTH_AS_USUAL,
							B_INFO_ALERT
						);
						alert->Go();
					}
				}
				break;
			}
			case BM_SELECT_ICONBOX: {
				entry_ref iconboxRef;
				BMessenger msnger(this);
				if (msg->FindRef( "refs", 0, &iconboxRef) != B_OK) {
					// first step, let user select new iconbox:
					if (!mIconboxPanel) {
						BmString iconPath = ThePrefs->GetString("IconPath");
						int32 pos = iconPath.FindLast('/');
						if (pos >= 0)
							iconPath.Truncate(pos);
						entry_ref eref;
						status_t err = get_ref_for_path(iconPath.String(), &eref);
						mIconboxPanel = new BFilePanel( 
							B_OPEN_PANEL, &msnger, err == B_OK ? &eref : NULL,
							B_DIRECTORY_NODE, false, msg
						);
					}
					mIconboxPanel->Show();
				} else {
					// second step, set iconbox accordingly:
					BEntry entry( &iconboxRef);
					BPath path;
					if (entry.GetPath( &path) == B_OK) {
						ThePrefs->SetString( "IconPath", path.Path());
						mIconboxButton->SetLabel( IconboxButtonLabel().String());
						TheResources->InitializeWithPrefs();
						TheToolbarManager->UpdateAll();
						NoticeChange();
					}
				}
				break;
			}
			case B_CANCEL: {
				// since a SavePanel seems to avoid quitting, thus stopping Beam 
				// from proper exit, we detroy the panel:
				if (mMailboxPanel && !mMailboxPanel->IsShowing()) {
					delete mMailboxPanel;
					mMailboxPanel = NULL;
				}
				if (mIconboxPanel && !mIconboxPanel->IsShowing()) {
					delete mIconboxPanel;
					mIconboxPanel = NULL;
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
