/*
	BmPrefsGeneralView.cpp
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

#include <Alert.h>
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
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmApp.h"
#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMenuControl.h"
#include "BmPrefs.h"
#include "BmPrefsGeneralView.h"
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
	:	inherited( "General Options")
	,	mMailboxPanel( NULL)
	,	mIconboxPanel( NULL)
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"General GUI Options",
					new VGroup(
						mWorkspaceControl = new BmMenuControl( 
							"Workspace to start in:", 
							new BPopUpMenu("")
						),
						new Space( minimax(0,10,0,10)),
						mListviewLikeTrackerControl = new BmCheckControl( 
							"Simulate Tracker's listview", 
					 		new BMessage(BM_LISTVIEW_LIKE_TRACKER_CHANGED), 
					 		this, ThePrefs->GetBool("ListviewLikeTracker", false)
					 	),
						new Space( minimax(0,10,0,10)),
						mShowTooltipsControl = new BmCheckControl( 
							"Show Tooltips for Toolbar-Buttons and Prefs", 
							new BMessage(BM_SHOW_TOOLTIPS_CHANGED), 
							this, ThePrefs->GetBool("ShowTooltips", true)
						),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Toolbar Options",
					new VGroup(
						mShowToolbarBorderControl = new BmCheckControl( 
							"Show Border around Toolbar-Buttons", 
							new BMessage(BM_SHOW_TOOLBAR_BORDER_CHANGED), 
							this, ThePrefs->GetBool("ShowToolbarBorder", false)
						),
						new Space( minimax(0,10,0,10)),
						mToolbarLabelControl = new BmMenuControl( 
							"Layout of Labels in Toolbar:", 
							new BPopUpMenu("")
						),
						new Space( minimax(0,10,0,10)),
						new HGroup(
							mIconboxButton = new MButton( 
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
				mMailboxButton = new MButton( 
					MailboxButtonLabel().String(), 
					new BMessage( BM_SELECT_MAILBOX), 
					this, minimax(-1,-1,-1,-1)
				),
				new Space(),
				0
			),
			new Space( minimax(10,0,10,0)),
			new HGroup( 
				new MButton( 
					"Make Beam preferred-app for email...", 
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
	TheBubbleHelper->SetHelp( mShowToolbarBorderControl, NULL);
	TheBubbleHelper->SetHelp( mToolbarLabelControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsGeneralView::MailboxButtonLabel() {
	BmString label( "Set Mailbox Folder (currently '");
	label << ThePrefs->GetString( "MailboxPath") << "')...";
	return label;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsGeneralView::IconboxButtonLabel() {
	BmString label( "Select Icon Folder (currently '");
	BmString defaultIconPath = bmApp->AppPath() + ThePrefs->nDefaultIconset;
	BmString iconPath 
		= ThePrefs->GetString( "IconPath",	defaultIconPath.String());
	int32 pos = iconPath.FindLast("/");
	if (pos >= B_OK)
		iconPath.Remove(0,pos+1);
	label << iconPath << "')...";
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
		"If checked, Beam's listviews will behave like the one's in Tracker:\n"
		"Shift-Click toggles selection, Option-Click expands selection."
	);
	TheBubbleHelper->SetHelp( 
		mShowTooltipsControl, 
		"Checking this makes Beam show a small \n"
		"info-window (just like this one) when the \n"
		"mouse-pointer lingers over a GUI-item."
	);
	TheBubbleHelper->SetHelp( 
		mShowToolbarBorderControl, 
		"If you check this a border is drawn around\n"
		"each toolbar-button (like in Postmaster)."
	);
	TheBubbleHelper->SetHelp( 
		mToolbarLabelControl, 
		"Here you can select if and where the label\n"
		"shall be shown inside a toolbar-button"
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
	mShowToolbarBorderControl->SetValueSilently( 
		ThePrefs->GetBool("ShowToolbarBorder", true)
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
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::SetDefaults() {
	ThePrefs->ResetToDefault();
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
			case BM_SHOW_TOOLBAR_BORDER_CHANGED: {
				ThePrefs->SetBool(
					"ShowToolbarBorder", 
					mShowToolbarBorderControl->Value()
				);
				NoticeChange();
				break;
			}
			case BM_TOOLBAR_LABEL_SELECTED: {
				BMenuItem* item = mToolbarLabelControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetString( "ShowToolbarLabel", item->Label());
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
						"Set Beam As Preferred App", 
						"This will make Beam the preferred Application for "
							"the following mimetypes:\n\n"
							"\tEmail (text/x-email)\n"
							"\tInternet-messages (message/rfc822).",
						"Ok, do it", "Cancel", NULL, B_WIDTH_AS_USUAL,
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
							"Set Beam As Preferred App", 
							"Done, Beam is now the preferred Application for email.",
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
						mMailboxPanel = new BFilePanel( 
							B_OPEN_PANEL, new BMessenger(this), NULL,
							B_DIRECTORY_NODE, false, msg
						);
					}
					mMailboxPanel->SetPanelDirectory( 
						ThePrefs->GetString("MailboxPath").String()
					);
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
							"Mailbox Path", 
							"Done, Beam will use the new mailbox after a restart",
							"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
							B_INFO_ALERT
						);
						alert->Go();
					}
				}
				break;
			}
			case BM_SELECT_ICONBOX: {
				entry_ref iconboxRef;
				if (msg->FindRef( "refs", 0, &iconboxRef) != B_OK) {
					// first step, let user select new iconbox:
					if (!mIconboxPanel) {
						mIconboxPanel = new BFilePanel( 
							B_OPEN_PANEL, new BMessenger(this), NULL,
							B_DIRECTORY_NODE, false, msg
						);
					}
					mIconboxPanel->SetPanelDirectory( 
						ThePrefs->GetString("IconPath").String()
					);
					mIconboxPanel->Show();
				} else {
					// second step, set iconbox accordingly:
					BEntry entry( &iconboxRef);
					BPath path;
					if (entry.GetPath( &path) == B_OK) {
						ThePrefs->SetString( "IconPath", path.Path());
						mIconboxButton->SetLabel( IconboxButtonLabel().String());
						NoticeChange();
						BAlert* alert = new BAlert( 
							"Icon Path", 
							"Done, Beam will use the new icons after a restart",
							"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
							B_INFO_ALERT
						);
						alert->Go();
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
