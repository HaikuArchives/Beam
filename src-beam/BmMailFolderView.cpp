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
#include "BmString.h"

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
	COL_NAME,
	COL_NEW_COUNT,
	COL_TOTAL_COUNT
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::BmMailFolderItem( ColumnListView* lv, 
												BmListModelItem* _item, 
												bool, BMessage* archive)
	:	inherited( lv, _item, true, archive)
{
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
void BmMailFolderItem::UpdateView( BmUpdFlags flags, bool redraw, 
											  uint32 updColBitmap) {
	BmMailFolder* folder( ModelItem());
	if (!folder)
		return;
	if (flags & (UPD_KEY | BmMailFolder::UPD_HAVE_NEW_STATUS)) {
		Bold( folder->NewMailCount());
		SetColumnContent( COL_NAME, folder->Name().String());
		if (redraw)
			updColBitmap = 0xFFFFFFFF;
							// Bold() may have changed font, need to redraw everything!
	}
	if (flags & (UPD_EXPANDER | BmMailFolder::UPD_HAVE_NEW_STATUS)) {
		BmBitmapHandle* icon;
		if (folder->NewMailCount()) {
			if (folder->HasNewMailInSubfolders() && !IsExpanded())
				icon = TheResources->IconByName("Folder_New_NewInSub");
			else
				icon = TheResources->IconByName("Folder_New");
		} else {
			if (folder->HasNewMailInSubfolders() && !IsExpanded())
				icon = TheResources->IconByName("Folder_NewInSub");
			else
				icon = TheResources->IconByName("Folder");
		}
		SetColumnContent( COL_ICON, icon, 2.0);
		updColBitmap |= (1UL<<COL_ICON);
	}
	if (flags & BmMailFolder::UPD_NEW_COUNT) {
		BmString newCountStr;
		int32 newCount = folder->NewMailCount();
		if (newCount == 0)
			newCountStr = "-";
		else if (newCount < 0)
			newCountStr = "?";
		else
			newCountStr << newCount;
		SetColumnContent( COL_NEW_COUNT, newCountStr.String());
		updColBitmap |= (1UL<<COL_NEW_COUNT);
	}
	if (flags & BmMailFolder::UPD_TOTAL_COUNT) {
		BmString totalCountStr;
		int32 totalCount = folder->MailCount();
		if (totalCount == 0)
			totalCountStr = "-";
		else if (totalCount < 0)
			totalCountStr = "?";
		else
			totalCountStr << totalCount;
		SetColumnContent( COL_TOTAL_COUNT, totalCountStr.String());
		updColBitmap |= (1UL<<COL_TOTAL_COUNT);
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFolderItem::CompareItems( const CLVListItem *a_Item1, 
												const CLVListItem *a_Item2, 
											   int32 KeyColumn, int32 col_flags) {
	if (ThePrefs->GetBool("InOutAlwaysAtTop", false)) {
		// handle special case for in & out-folders to be fixed at top:
		const BmMailFolderItem* item1 
			= dynamic_cast<const BmMailFolderItem*>(a_Item1);
		const BmMailFolderItem* item2 
			= dynamic_cast<const BmMailFolderItem*>(a_Item2);
		if (item1 == NULL || item2 == NULL)
			return 0;
		BmMailFolder* folder1( item1->ModelItem());
		BmMailFolder* folder2( item2->ModelItem());
		if (folder1 == NULL || folder2 == NULL)
			return 0;
			
		CLVSortMode sortmode 
			= TheMailFolderView->ColumnAt( KeyColumn)->SortMode();
		int32 rev = sortmode == Ascending ? 1 : -1;
	
		if (folder1->Name().ICompare("in")==0 
		|| folder1->Name().ICompare("in ",3)==0)
			return -1 * rev;
		if (folder2->Name().ICompare("in")==0 
		|| folder2->Name().ICompare("in ",3)==0)
			return 1 * rev;
		if (folder1->Name().ICompare("out")==0 
		|| folder1->Name().ICompare("out ",4)==0)
			return -1 * rev;
		if (folder2->Name().ICompare("out")==0 
		|| folder2->Name().ICompare("out ",4)==0)
			return 1 * rev;
		if (folder1->Name().ICompare("draft")==0 
		|| folder1->Name().ICompare("draft ",5)==0)
			return -1 * rev;
		if (folder2->Name().ICompare("draft")==0 
		|| folder2->Name().ICompare("draft ",5)==0)
			return 1 * rev;
	}
	return CLVEasyItem::CompareItems( a_Item1, a_Item2, KeyColumn, col_flags);
}


/********************************************************************************\
	BmMailFolderView
\********************************************************************************/


const char* const BmMailFolderView::MSG_CURR_FOLDER = "bm:currfolder";
const char* const BmMailFolderView::MSG_VERSION = "bm:ver";

const char* const BmMailFolderView::MSG_HAVE_SELECTED_FOLDER = "bm:fsel";

int16 BmMailFolderView::nArchiveVersion = 1;

BmMailFolderView* BmMailFolderView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView* BmMailFolderView::CreateInstance( minimax minmax, 
																	 int32 width, 
																	 int32 height) {
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
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FolderView", 
					  B_SINGLE_SELECTION_LIST, true, true, true, true)
	,	mPartnerMailRefView( NULL)
	,	mHaveSelectedFolder( false)
{
	Initialize( BRect( 0,0,width-1,height-1), 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);
	AddColumn( new CLVColumn( NULL, 10.0, 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING 
									  | CLV_NOT_MOVABLE | CLV_COLTYPE_BITMAP, 10.0));
	AddColumn( new CLVColumn( NULL, 18.0, 
									  CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE | CLV_PUSH_PASS 
									  | CLV_MERGE_WITH_RIGHT | CLV_COLTYPE_BITMAP, 
									  18.0));
	AddColumn( new CLVColumn( "Folders", 100.0, 
									  CLV_SORT_KEYABLE | CLV_LOCK_AT_BEGINNING 
									  | CLV_NOT_MOVABLE, 
									  50.0));
	AddColumn( new CLVColumn( "New", 35.0, CLV_RIGHT_JUSTIFIED, 20.0));
	AddColumn( new CLVColumn( "Mails", 35.0, CLV_RIGHT_JUSTIFIED, 20.0));
	SetSortFunction( BmMailFolderItem::CompareItems);
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
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMailFolderView::CreateContainer( bool horizontal, 
																	  bool vertical, 
												  					  bool scroll_view_corner, 
												  					  border_style border, 
																	  uint32 ResizingMode, 
																	  uint32 flags) 
{
	return 
		new BmCLVContainerView( 
			fMinMax, this, ResizingMode, flags, horizontal, vertical, 
			scroll_view_corner, border, mShowCaption, mShowBusyView, 
			be_plain_font->StringWidth(" 999 folders ")
		);
}

/*------------------------------------------------------------------------------*\
	Archive()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailFolderView::Archive(BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (ret == B_OK) {
		archive->AddInt16( MSG_VERSION, nArchiveVersion);
		BmRef<BmMailFolder> currFolder = CurrentFolder();
		if (currFolder)
			archive->AddString( MSG_CURR_FOLDER, currFolder->Key().String());
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	Unarchive()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailFolderView::Unarchive(const BMessage* archive, bool deep) {
	status_t err = inherited::Unarchive( archive, deep);
	if (err == B_OK) {
		int16 version = 0;
		archive->FindInt16( MSG_VERSION, &version);
		if (version < 1) {
			// make room for (and show) 'New'- and 'Mails'-columns:
			ColumnAt(2)->SetWidth( 100);
			ColumnAt(3)->SetWidth( 30);
			ColumnAt(4)->SetWidth( 30);
			ShowColumn( 3);
			ShowColumn( 4);
		}
		mLastActiveKey = archive->FindString( MSG_CURR_FOLDER);
	}
	return err;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailFolderView::CreateListViewItem( BmListModelItem* item,
																		BMessage* archive) {
	if (ThePrefs->GetBool("RestoreFolderStates"))
		return new BmMailFolderItem( this, item, true, archive);
	else
		return new BmMailFolderItem( this, item, true, NULL);
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
	static int jobNum = 1;
	type_code tc;
	int32 refCount;
	if (msg && mCurrHighlightItem	&& (msg->what == B_SIMPLE_DATA)
	&& msg->GetInfo( "refs", &tc, &refCount) == B_OK) {
		BmRef<BmMailFolder> folder( 
			dynamic_cast<BmMailFolder*>( mCurrHighlightItem->ModelItem())
		);
		if (folder) {
			BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
			entry_ref eref;
			entry_ref* refs = new entry_ref [refCount];
			int i=0;
			while(  msg->FindRef( "refs", i, &refs[i]) == B_OK)
				++i;
			tmpMsg.AddPointer( BmMailMover::MSG_REFS, (void*)refs);
			tmpMsg.AddInt32( BmMailMover::MSG_REF_COUNT, i);
			BmString jobName = folder->Name();
			jobName << jobNum++;
			tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
			tmpMsg.AddString( BmJobModel::MSG_MODEL, folder->Key().String());
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
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons)==B_OK 
	&& buttons == B_SECONDARY_MOUSE_BUTTON) {
		int32 clickIndex = IndexOf( point);
		if (clickIndex >= 0)
			Select( clickIndex);
		else 
			DeselectAll();
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	Hook function that is called whenever the jobmodel associated to this 
			controller indicates that it is done (meaning: the list has been 
			fetched and is now ready to be displayed).
\*------------------------------------------------------------------------------*/
void BmMailFolderView::JobIsDone( bool completed) {
	inherited::JobIsDone( completed);
	if (mLastActiveKey.Length()) {
		BmRef<BmDataModel> modelRef( DataModel());
		BmListModel* listModel = dynamic_cast<BmListModel*>( modelRef.Get());
		BmRef<BmListModelItem> item = listModel->FindItemByKey( mLastActiveKey);
		BmListViewItem* viewItem = FindViewItemFor( item.Get());
		if (viewItem)
			Select( IndexOf( viewItem));
	}
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::KeyDown(const char *bytes, int32 numBytes) {
	if ( numBytes == 1 ) {
		switch( bytes[0]) {
			// implement remote navigation within ref-/mail-view 
			// (via cursor-keys with modifiers):
			case B_PAGE_UP:
			case B_PAGE_DOWN:
			case B_UP_ARROW:
			case B_DOWN_ARROW: {
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					// leave in modifiers to address mailview:
					if (mPartnerMailRefView)
						mPartnerMailRefView->KeyDown( bytes, numBytes);
				} else if (mods & (B_SHIFT_KEY)) {
					// remove modifiers to address mailrefview:
					Window()->CurrentMessage()->ReplaceInt32("modifiers", 0);
					if (mPartnerMailRefView)
						mPartnerMailRefView->KeyDown( bytes, numBytes);
				} else 
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
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
					TextEntryAlert* alert = new TextEntryAlert( 
						"New Mail-Folder", 
						"Enter name of new folder:",
						"", "Cancel", "OK"
					);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( 
						new BMessage(BMM_NEW_MAILFOLDER), BMessenger( this)
					));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 1) {
						BmString newName = msg->FindString( "entry_text");
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
					TextEntryAlert* alert = new TextEntryAlert( 
						"Rename Mail-Folder", 
						"Enter new name for folder:",
						folder->Name().String(), "Cancel", "OK"
					);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( 
						new BMessage(BMM_RENAME_MAILFOLDER), BMessenger( this)
					));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 1) {
						BmString newName = msg->FindString( "entry_text");
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
					BAlert* alert = new BAlert( 
						"Trash Mail-Folder", 
						(BmString("Are you sure about trashing folder <") 
							<< folder->Name() << ">?").String(),
						"Move to Trash", "Cancel", NULL, 
						B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( 
						new BMessage(BMM_DELETE_MAILFOLDER), BMessenger( this)
					));
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
				if (mPartnerMailRefView)
					mPartnerMailRefView->ShowFolder( folder.Get());
				break;
			}
			case B_MOUSE_WHEEL_CHANGED: {
				if (modifiers() & (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					if (mPartnerMailRefView) {
						BMessage msg2( *msg);
						Looper()->PostMessage( &msg2, mPartnerMailRefView);
						return;
					}
				}
				if (modifiers() & B_SHIFT_KEY) {
					if (mPartnerMailRefView) {
						BMessage msg2( *msg);
						msg2.AddBool("bm:passed_on", true);
						Looper()->PostMessage( &msg2, mPartnerMailRefView);
						return;
					}
				}
				inherited::MessageReceived( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailFolderView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::SelectionChanged( void) {
	if (!mPartnerMailRefView)
		return;
	BmRef<BmMailFolder> folder = CurrentFolder();
	if (folder)
		mPartnerMailRefView->ShowFolder( folder.Get());
	else
		mPartnerMailRefView->ShowFolder( NULL);
	SendNoticesIfNeeded( folder ? true : false);
}


/*------------------------------------------------------------------------------*\
	SendNoticesIfNeeded()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::SendNoticesIfNeeded( bool haveSelectedFolder) {
	if (haveSelectedFolder != mHaveSelectedFolder) {
		mHaveSelectedFolder = haveSelectedFolder;
		BMessage msg(BM_NTFY_MAILFOLDER_SELECTION);
		msg.AddBool( MSG_HAVE_SELECTED_FOLDER, mHaveSelectedFolder);
		SendNotices( BM_NTFY_MAILFOLDER_SELECTION, &msg);
	}
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::ItemInvoked( int32 index) {
	BmMailFolderItem* folderItem;
	folderItem = dynamic_cast<BmMailFolderItem*>(ItemAt( index));
	if (folderItem) {
		BmMailFolder* folder( folderItem->ModelItem());
		if (folder) {
			// open current mail-folder in tracker:
			BMessenger tracker("application/x-vnd.Be-TRAK" );
			BMessage msg( B_REFS_RECEIVED);
			msg.AddRef( "refs", folder->EntryRefPtr());
			tracker.SendMessage( &msg);
		}
	}
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
		if (folderItem)
			return folderItem->ModelItem();
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
	BFont font( *be_plain_font);
	font.SetSize( 10);
	theMenu->SetFont( &font);

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
	theMenu->SetAsyncAutoDestruct( true);
  	theMenu->Go( point, true, false, openRect, true);
}

