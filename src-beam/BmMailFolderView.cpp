/*
	BmMailFolderView.cpp
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
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>

#include "TextEntryAlert.h"

#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailMover.h"
#include "BmMailRefView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailFolderItem
\********************************************************************************/

enum Columns {
	COL_EXPANDER = 0,
	COL_ICON,
	COL_NAME
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::BmMailFolderItem( BString key, BmListModelItem* _item, 
												bool superitem, BMessage* archive)
	:	inherited( key, _item, true, archive)
{
	UpdateView( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::~BmMailFolderItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmMailFolder* folder = ModelItem();
	if (flags & (UPD_EXPANDER | BmMailFolder::UPD_NAME)) {
		Bold( folder->NewMailCount());
		BString displayName = folder->Name();
		if (folder->HasNewMail()) {
			int32 count = folder->NewMailCount();
			if (folder->NewMailCountForSubfolders() && !IsExpanded())
	 			count += folder->NewMailCountForSubfolders();
	 		if (count)
				displayName << " - (" << count << ")";
		}
		SetColumnContent( COL_NAME, displayName.String(), false);
		BBitmap* icon;
		if (folder->NewMailCount())
			icon = TheResources->IconByName("Folder_WithNew");
		else
			icon = TheResources->IconByName("Folder");
		SetColumnContent( COL_ICON, icon, 2.0, false);
	}
}



/********************************************************************************\
	BmMailFolderView
\********************************************************************************/


BmMailFolderView* BmMailFolderView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView* BmMailFolderView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmMailFolderView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::BmMailFolderView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FolderView", B_SINGLE_SELECTION_LIST, 
					  true, true, true, true)
{
	Initialize( BRect( 0,0,width-1,height-1), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);
	AddColumn( new CLVColumn( NULL, 10.0, 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 10.0));
	AddColumn( new CLVColumn( NULL, 18.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT, 18.0));
	AddColumn( new CLVColumn( "Folders", 300.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_NOT_MOVABLE, 300.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_NAME);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::~BmMailFolderView() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailFolderView::CreateListViewItem( BmListModelItem* item,
																		BMessage* archive) {
	if (ThePrefs->GetBool("RestoreFolderStates"))
		return new BmMailFolderItem( item->Key(), item, true, archive);
	else
		return new BmMailFolderItem( item->Key(), item, true, NULL);
}

/*------------------------------------------------------------------------------*\
	AcceptsDropOf( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderView::AcceptsDropOf( const BMessage* msg) {
	return (msg && msg->what == B_SIMPLE_DATA);
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::HandleDrop( const BMessage* msg) {
	if (msg && mCurrHighlightItem
	&& (msg->what == B_SIMPLE_DATA)) {
		BmMailFolder* folder = dynamic_cast<BmMailFolder*>( mCurrHighlightItem->ModelItem());
		if (folder) {
			BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
			entry_ref eref;
			for( int i=0; msg->FindRef( BmMailMover::MSG_REFS, i, &eref)==B_OK; ++i) {
				tmpMsg.AddRef( BmMailMover::MSG_REFS, &eref);
			}
			tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, folder->Name());
			tmpMsg.AddString( BmJobModel::MSG_MODEL, folder->Key());
			TheJobStatusWin->PostMessage( &tmpMsg);
		}
	}
	inherited::HandleDrop( msg);
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	BPoint mousePos;
	uint32 buttons;
	GetMouse( &mousePos, &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::MessageReceived( BMessage* msg) {
	try {
		BmRef<BmMailFolder> folder;
		int32 buttonPressed;
		switch( msg->what) {
			case BMM_NEW_MAILFOLDER: {
				folder = CurrentFolder();
				if (!folder)
					return;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					TextEntryAlert* alert = new TextEntryAlert( "New Mail-Folder", 
																		  	  "Enter name of new folder:",
												 							  "", "Cancel", "OK");
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BMM_NEW_MAILFOLDER), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 1) {
						BString newName = msg->FindString( "entry_text");
						if (newName.Length()>0)
							folder->CreateSubFolder( newName.String());
					}
				}
				break;
			}
			case BMM_RENAME_MAILFOLDER: {
				folder = CurrentFolder();
				if (!folder)
					return;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					TextEntryAlert* alert = new TextEntryAlert( "Rename Mail-Folder", 
																			  "Enter new name for folder:",
												 							  folder->Name().String(), "Cancel", "OK");
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BMM_RENAME_MAILFOLDER), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 1) {
						BString newName = msg->FindString( "entry_text");
						if (newName.Length()>0)
							folder->Rename( newName.String());
					}
				}
				break;
			}
			case BMM_DELETE_MAILFOLDER: {
				folder = CurrentFolder();
				if (!folder)
					return;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Trash Mail-Folder", 
														 (BString("Are you sure about trashing folder <") << folder->Name() << ">?").String(),
													 	 "Move to Trash", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BMM_DELETE_MAILFOLDER), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0)
						folder->MoveToTrash();
				}
				break;
			}
			case BMM_RECACHE_MAILFOLDER: {
				folder = CurrentFolder();
				if (!folder)
					return;
				folder->RecreateCache();
				if (TheMailRefView)
					TheMailRefView->ShowFolder( folder.Get());
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailFolderView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( msg)
		-	Hook function that is called whenever an item needs to be updated 
\*------------------------------------------------------------------------------*/
void BmMailFolderView::UpdateModelItem( BMessage* msg) {
	
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::SelectionChanged( void) {
	if (!TheMailRefView)
		return;
	BmRef<BmMailFolder> folder = CurrentFolder();
	if (folder)
		TheMailRefView->ShowFolder( folder.Get());
	else
		TheMailRefView->ShowFolder( NULL);

	BMessage msg(BM_NTFY_MAILFOLDER_SELECTION);
	msg.AddInt32( MSG_FOLDERS_SELECTED, folder ? 1 : 0);
	SendNotices( BM_NTFY_MAILFOLDER_SELECTION, &msg);
}

/*------------------------------------------------------------------------------*\
	CurrentFolder()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMailFolder> BmMailFolderView::CurrentFolder( void) const {
	int32 selection = CurrentSelection();
	if (selection >= 0) {
		BmMailFolderItem* folderItem;
		folderItem = dynamic_cast<BmMailFolderItem*>(ItemAt( selection));
		if (folderItem) {
			BmMailFolder* folder = dynamic_cast<BmMailFolder*>(folderItem->ModelItem());
			return folder;
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::ShowMenu( BPoint point) {
	BmRef<BmMailFolder> folder = CurrentFolder();

	BPopUpMenu* theMenu = new BPopUpMenu( "MailFolderViewMenu", false, false);

	BMenuItem* item;
	if (folder) {
		item = new BMenuItem( "New Folder...", 
									 new BMessage( BMM_NEW_MAILFOLDER));
		item->SetTarget( this);
		theMenu->AddItem( item);
	
		item = new BMenuItem( "Rename Folder...", 
									 new BMessage( BMM_RENAME_MAILFOLDER));
		item->SetTarget( this);
		theMenu->AddItem( item);
	
		item = new BMenuItem( "Delete Folder...", 
									 new BMessage( BMM_DELETE_MAILFOLDER));
		item->SetTarget( this);
		theMenu->AddItem( item);
	
		theMenu->AddSeparatorItem();
	
		item = new BMenuItem( "Recreate Cache", 
									 new BMessage( BMM_RECACHE_MAILFOLDER));
		item->SetTarget( this);
		theMenu->AddItem( item);
	}

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
}

