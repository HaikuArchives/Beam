/*
	BmMailRefView.cpp
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


#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailMover.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

const int16 BmMailRefItem::nFirstTextCol = 3;

struct BmIndexDesc {
	BmIndexDesc()
		: start( 0), end( 0) {}
	BmIndexDesc( int32 s)
		: start( s), end( s) {}
	int32 start;
	int32 end;
};

enum Columns {
	COL_STATUS_I = 0,
	COL_ATTACHMENTS_I,
	COL_PRIORITY_I,
	COL_FROM,
	COL_SUBJECT,
	COL_DATE,
	COL_SIZE,
	COL_CC,
	COL_ACCOUNT,
	COL_TO,
	COL_REPLY_TO,
	COL_NAME,
	COL_CREATED,
	COL_TRACKER_NAME,
	COL_STATUS,
	COL_ATTACHMENT,
	COL_PRIORITY,
	COL_IDENTITY,
	COL_END
};

/********************************************************************************\
	BmMailRefItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem( const BmString& key, BmListModelItem* _item)
	:	inherited( key, _item)
{
	UpdateView( UPD_ALL);
	for( int col=nFirstTextCol; col<COL_END; ++col) {
		if (col==COL_SIZE)
			SetColumnUserTextContent( col, true);
		else
			SetColumnUserTextContent( col, false);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::~BmMailRefItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmMailRef* ref( ModelItem());
	if (!ref)
		return;
	BBitmap* icon = NULL;

	if (flags & BmMailRef::UPD_STATUS) {
		Bold( ref->IsNew());
		BmString st = BmString("Mail_") << ref->Status();
		icon = TheResources->IconByName(st);
		SetColumnContent( COL_STATUS_I, icon);
		SetColumnContent( COL_STATUS, ref->Status().String(), false);
	}

	if (flags == UPD_ALL) {
		if (ref->HasAttachments()) {
			icon = TheResources->IconByName("Attachment");
			SetColumnContent( COL_ATTACHMENTS_I, icon);
		}
		BmString priority = BmString("Priority_") << ref->Priority();
		if ((icon = TheResources->IconByName(priority))!=NULL) {
			SetColumnContent( COL_PRIORITY_I, icon);
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const int32 BmMailRefItem::GetNumValueForColumn( int32 column_index) const {
	BmMailRef* ref( ModelItem());
	if (column_index == 0 || column_index == 14) {
		// status
		BmString st = ref->Status();
		return st == BM_MAIL_STATUS_NEW			? 0 :
				 st == BM_MAIL_STATUS_DRAFT		? 1 :
				 st == BM_MAIL_STATUS_PENDING		? 2 :
				 st == BM_MAIL_STATUS_READ			? 3 :
				 st == BM_MAIL_STATUS_SENT			? 4 : 
				 st == BM_MAIL_STATUS_FORWARDED	? 5 :
				 st == BM_MAIL_STATUS_REPLIED		? 6 :
				 st == BM_MAIL_STATUS_REDIRECTED	? 7 : 99;
	} else if (column_index == COL_ATTACHMENTS_I || column_index == COL_ATTACHMENT) {
		return ref->HasAttachments() ? 0 : 1;	
							// show mails with attachment at top
	} else if (column_index == COL_PRIORITY_I || column_index == COL_PRIORITY) {
		int16 prio = atol( ref->Priority().String());
		return (prio>=1 && prio<=5) ? prio : 3;
							// illdefined priority means medium priority (=3)
	} else if (column_index == COL_SIZE) {
		return ref->Size();
	} else {
		return 0;		// we don't know this number-column !?!
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const time_t BmMailRefItem::GetDateValueForColumn( int32 column_index) const {
	BmMailRef* ref( ModelItem());
	if (column_index == COL_DATE)
		return ref->When();
	else if (column_index == COL_CREATED)
		return ref->Created();
	else
		return 0;		// we don't know this date-column !?!
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const char* BmMailRefItem::GetUserText(int32 colIdx, float colWidth) const {
	BmMailRef* ref( ModelItem());
	if (!ref)
		return "";
	const char* text;
	switch( colIdx) {
	case COL_FROM:
		text = ref->From().String();
		break;
	case COL_SUBJECT:
		text = ref->Subject().String();
		break;
	case COL_DATE:
		text = ref->WhenString().String();
		break;
	case COL_SIZE:
		text = ref->SizeString().String();
		break;
	case COL_CC:
		text = ref->Cc().String();
		break;
	case COL_ACCOUNT:
		text = ref->Account().String();
		break;
	case COL_TO:
		text = ref->To().String();
		break;
	case COL_REPLY_TO:
		text = ref->ReplyTo().String();
		break;
	case COL_NAME:
		text = ref->Name().String();
		break;
	case COL_CREATED:
		text = ref->CreatedString().String();
		break;
	case COL_TRACKER_NAME:
		text = ref->TrackerName();
		break;
	case COL_STATUS:
		text = ref->Status().String();
		break;
	case COL_ATTACHMENT:
		text = ref->HasAttachments() ? "*" : "";
		break;
	case COL_PRIORITY:
		text = ref->Priority().String();
		break;
	case COL_IDENTITY:
		text = ref->Identity().String();
		break;
	default:
		return "";
	};
	if (text)
		return text;
	return "";
}



/********************************************************************************\
	BmMailRefView
\********************************************************************************/


const char* const BmMailRefView::MSG_MAILS_SELECTED = 	"bm:msel";
const char* const BmMailRefView::MENU_MARK_AS =				"Mark Message As";
const char* const BmMailRefView::MENU_FILTER = 				"Apply Specific Filter";
const char* const BmMailRefView::MENU_MOVE =					"Move Message To";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( minimax minmax, int32 width, int32 height) {
	return new BmMailRefView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_MailRefView", B_MULTIPLE_SELECTION_LIST, 
					  false, true, true, true)
	,	mCurrFolder( NULL)
	,	mAvoidInvoke( false)
	,	mHaveSelectedRef( false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	Initialize( BRect(0,0,width-1,height-1), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_NONE, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "Status [Icon]"));
	AddColumn( new CLVColumn( "A", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "(A)ttachments [Icon]"));
	AddColumn( new CLVColumn( "P", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "(P)riority [Icon]"));
	AddColumn( new CLVColumn( "From", 200.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Subject", 200.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Date", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE | flags, 20.0));
	AddColumn( new CLVColumn( "Size", 50.0, CLV_SORT_KEYABLE | CLV_COLDATA_NUMBER | CLV_RIGHT_JUSTIFIED | flags, 20.0));
	AddColumn( new CLVColumn( "Cc", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Account", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "To", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Reply-To", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Name", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Created", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE | flags, 20.0));
	AddColumn( new CLVColumn( "Tracker-Name", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "S", 100.0, CLV_SORT_KEYABLE, 40.0, "(S)tatus [Text]"));
	AddColumn( new CLVColumn( "A", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_NUMBER, 18.0, "(A)ttachments [Text]"));
	AddColumn( new CLVColumn( "P", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_NUMBER, 18.0, "(P)riority [Text]"));
	AddColumn( new CLVColumn( "Identity", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_DATE);
	SetSortMode( COL_DATE, Descending, false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::~BmMailRefView() { 
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMailRefView::CreateContainer( bool horizontal, bool vertical, 
												  				  bool scroll_view_corner, 
												  				  border_style border, 
																  uint32 ResizingMode, 
																  uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView, 
											 be_plain_font->StringWidth(" 99999 messages "));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailRefView::CreateListViewItem( BmListModelItem* item, 
																	BMessage*) {
	return new BmMailRefItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_TRASH_TARGET: {
				if (msg->IsReply()) {
					const BMessage* origMsg = msg->Previous();
					if (origMsg) {
						int32 count;
						type_code code;
						origMsg->GetInfo( "refs", &code, &count);
						entry_ref* refs = new entry_ref [count];
						int i=0;
						while( origMsg->FindRef( "refs", i, &refs[i]) == B_OK)
							++i;
						MoveToTrash( refs, i);
						delete [] refs;
					}
				}
				break;
			}
			case B_MOUSE_WHEEL_CHANGED: {
				if (modifiers() & (B_SHIFT_KEY | B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					bool passedOn = false;
					if (mPartnerMailView && !(passedOn = msg->FindBool("bm:passed_on"))) {
						BMessage msg2(*msg);
						msg2.AddBool("bm:passed_on", true);
						Looper()->PostMessage( &msg2, mPartnerMailView);
						return;
					}
				}
				inherited::MessageReceived( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::KeyDown(const char *bytes, int32 numBytes) { 
	if ( numBytes == 1 ) {
		switch( bytes[0]) {
			// implement remote navigation within mail-view (via cursor-keys with modifiers):
			case B_PAGE_UP:
			case B_PAGE_DOWN:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW: {
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					// remove modifiers so we don't ping-pong endlessly:
					Window()->CurrentMessage()->ReplaceInt32("modifiers", 0);
					if (mPartnerMailView)
						mPartnerMailView->KeyDown( bytes, numBytes);
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			case B_DELETE: {
				BMessage msg(BMM_TRASH);
				vector< BmIndexDesc> indexVect;
				vector< BListItem*> itemVect;
				int32 currIdx;
				int32 index=-1;
				for( int32 i=0; (currIdx=CurrentSelection( i))>=0; ++i) {
					if (index==-1 || indexVect[index].end != currIdx-1) {
						indexVect.push_back( BmIndexDesc( currIdx));
						index++;
					} else
						indexVect[index].end = currIdx;
					itemVect.push_back( ItemAt( currIdx));
				}
				if (indexVect.empty())
					break;
				AddSelectedRefsToMsg( &msg, BmApplication::MSG_MAILREF);
				// now move cursor onwards...
				if (indexVect.back().end < CountItems()-1)
					Select( indexVect.back().end+1);		// select next item that remains in list
				else if (indexVect.front().start > 0)
					Select( indexVect.front().start-1);		// select last item that remains in list
				else
					DeselectAll();
				// remove view-items immediately because that looks better and it 
				// avoids double deletions (which cause Tracker to complain):
				for( int32 i=indexVect.size()-1; i>=0; --i) {
					RemoveItems( indexVect[i].start, 1+indexVect[i].end-indexVect[i].start);
				}
				UpdateCaption();
				{	// scope for lock
					BmAutolockCheckGlobal lock( DataModel()->ModelLocker());
					lock.IsLocked()			|| BM_THROW_RUNTIME( BmString() << ControllerName() << "KeyDown(): Unable to lock model");
					for( uint32 i=0; i<itemVect.size(); ++i) {
						doRemoveModelItem( ((BmListViewItem*)itemVect[i])->ModelItem());
					}
				}
				// finally we instruct our app to remove the mail
				be_app_messenger.SendMessage( &msg);
				break;
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
	}
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::MouseDown(BPoint point) {
	inherited::MouseDown( point);
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons)==B_OK 
	&& buttons == B_SECONDARY_MOUSE_BUTTON) {
		int32 clickIndex = IndexOf( point);
		if (clickIndex >= 0) {
			if (!IsItemSelected( clickIndex))
				Select( clickIndex);
		} else 
			DeselectAll();
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	InitiateDrag()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefView::InitiateDrag( BPoint, int32 index, bool wasSelected) {
	if (!wasSelected)
		return false;
	BM_LOG2( BM_LogGui, "MailRefView::InitiateDrag() - enter");
	BMessage dragMsg( B_SIMPLE_DATA);
	dragMsg.AddString( "be:types", "text/x-email");
	dragMsg.AddString( "be:type_descriptions", "E-mail");
	dragMsg.AddInt32( "be:actions", B_MOVE_TARGET);
	dragMsg.AddInt32( "be:actions", B_TRASH_TARGET);
	BmMailRefItem* refItem = dynamic_cast<BmMailRefItem*>(ItemAt( index));
	BmMailRef* ref( refItem->ModelItem());
	dragMsg.AddString( "be:clip_name", ref->TrackerName());
	dragMsg.AddString( "be:originator", "Beam");
	int32 currIdx;
	// we count the number of selected items:
	int32 selCount;
	for( selCount=0; (currIdx=CurrentSelection( selCount))>=0; ++selCount)
		;
	BM_LOG2( BM_LogGui, BmString("MailRefView::InitiateDrag() - found ")<<selCount<<" selections");
	BFont font;
	GetFont( &font);
	float lineHeight = MAX(TheResources->FontLineHeight( &font),20.0);
	float baselineOffset = TheResources->FontBaselineOffset( &font);
	BRect dragRect( 0, 0, 200-1, MIN(selCount,4.0)*lineHeight-1);
	BView* dummyView = new BView( dragRect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* dragImage = new BBitmap( dragRect, B_RGBA32, true);
	dragImage->AddChild( dummyView);
	dragImage->Lock();
	dummyView->SetHighColor( B_TRANSPARENT_COLOR);
	dummyView->FillRect( dragRect);
	dummyView->SetDrawingMode( B_OP_ALPHA);
	dummyView->SetHighColor( 0, 0, 0, 128);
	dummyView->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	for( int32 i=0; (currIdx=CurrentSelection( i))>=0; ++i) {
		// now we add all selected items to drag-image and to drag-msg:
		refItem = dynamic_cast<BmMailRefItem*>(ItemAt( currIdx));
		BmMailRef* ref( refItem->ModelItem());
		dragMsg.AddRef( "refs", ref->EntryRefPtr());
		if (i<3) {
			// add only the first three selections to drag-image:
			const BBitmap* icon = refItem->GetColumnContentBitmap( 0);
			if (icon) {
				dummyView->DrawBitmapAsync( icon, BPoint(0,i*lineHeight));
			}
			dummyView->DrawString( ref->Subject().String(), 
										  BPoint( 20.0, i*lineHeight+baselineOffset));
		} else if (i==3) {
			// add an indicator that more items are being dragged than shown:
			BmString indicator = BmString("(...and ") << selCount-3 
				<< (selCount-3 == 1 ? " more item)" : " more items)");
			dummyView->DrawString( indicator.String(), 
										  BPoint( 20.0, i*lineHeight+baselineOffset));
		}
		if (i%100==0)
			BM_LOG2( BM_LogGui, BmString("MailRefView::InitiateDrag() - processing ")<<i<<"th selection");
	}
	dragImage->Unlock();
	DragMessage( &dragMsg, dragImage, B_OP_ALPHA, BPoint( 10.0, 10.0));
	DeselectAll();
	BM_LOG2( BM_LogGui, "MailRefView::InitiateDrag() - exit");
	return true;
}

/*------------------------------------------------------------------------------*\
	AcceptsDropOf( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefView::AcceptsDropOf( const BMessage* msg) {
	if (mCurrFolder && msg && msg->what == B_SIMPLE_DATA) {
		entry_ref eref;
		for( int32 i=0; msg->FindRef( "refs", i, &eref) == B_OK; ++i) {
			if (CheckMimeType( &eref, "text/x-email"))
				return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::HandleDrop( const BMessage* msg) {
	if (mCurrFolder && msg && msg->what == B_SIMPLE_DATA) {
		BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
		entry_ref eref;
		for( int i=0; msg->FindRef( BmMailMover::MSG_REFS, i, &eref)==B_OK; ++i) {
			tmpMsg.AddRef( BmMailMover::MSG_REFS, &eref);
		}
		tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, mCurrFolder->Name().String());
		tmpMsg.AddString( BmJobModel::MSG_MODEL, mCurrFolder->Key().String());
		TheJobStatusWin->PostMessage( &tmpMsg);
	}
	inherited::HandleDrop( msg);
}

/*------------------------------------------------------------------------------*\
	ShowFolder()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ShowFolder( BmMailFolder* folder) {
	try {
		StopJob();
		BmRef<BmMailRefList> refList( folder ? folder->MailRefList().Get() : NULL);
		if (mPartnerMailView)
			mPartnerMailView->ShowMail( static_cast< BmMailRef*>( NULL));
		if (refList)
			StartJob( refList.Get());
		else {
			DetachModel();
			MakeEmpty();
		}
		mCurrFolder = folder;
		SelectionChanged();
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmString BmMailRefView::StateInfoBasename()	{ 
	return "MailRefView";
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmMailRefView::DefaultLayout()		{ 
	return ThePrefs->GetMsg("MailRefLayout"); 
}

/*------------------------------------------------------------------------------*\
	AddSelectedRefsToMsg( msg, fieldName)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::AddSelectedRefsToMsg( BMessage* msg, BmString fieldName) {
	BmString selectedText;
	if (mPartnerMailView) {
		int32 start, finish;
		mPartnerMailView->GetSelection( &start, &finish);
		if (start < finish)
			selectedText.SetTo(mPartnerMailView->Text()+start, finish-start);
	}
	int32 selected = -1;
	int32 numSelected = 0;
	BMessenger msngr( this);
	while( (selected = CurrentSelection(numSelected)) >= 0) {
		BmMailRefItem* refItem;
		refItem = dynamic_cast<BmMailRefItem*>(ItemAt( selected));
		if (refItem) {
			BmMailRef* ref( refItem->ModelItem());
			msg->AddPointer( fieldName.String(), static_cast< void*>( ref));
			ref->AddRef();						// the message now refers to the mailRef, too
			if (selectedText.Length())
				msg->AddString( BmApplication::MSG_SELECTED_TEXT, selectedText.String());
			msg->AddMessenger( BmApplication::MSG_SENDING_REFVIEW, msngr);
		}
		numSelected++;
	}
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::SelectionChanged( void) {
	BM_LOG2( BM_LogGui, "MailRefView::SelectionChanged() - enter");
	int32 temp;
	int32 selection = -1;
	int32 numSelected = 0;
	while( numSelected < 2 && (temp = CurrentSelection(numSelected)) >= 0) {
		selection = temp;
		numSelected++;
	}
	if (mAvoidInvoke) {
		DeselectAll();
		return;
	}
	if (mPartnerMailView) {
		if (selection >= 0 && numSelected == 1) {
			BmMailRefItem* refItem;
			refItem = dynamic_cast<BmMailRefItem*>(ItemAt( selection));
			if (refItem) {
				BmMailRef* ref( refItem->ModelItem());
				if (ref)
					mPartnerMailView->ShowMail( ref);
			}
		} else
			mPartnerMailView->ShowMail( static_cast< BmMailRef*>( NULL));
	}
	
	SendNoticesIfNeeded( numSelected > 0);
	BM_LOG2( BM_LogGui, "MailRefView::SelectionChanged() - exit");
}

/*------------------------------------------------------------------------------*\
	SendNoticesIfNeeded()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::SendNoticesIfNeeded( bool haveSelectedRef) {
	if (haveSelectedRef != mHaveSelectedRef) {
		mHaveSelectedRef = haveSelectedRef;
		BMessage msg(BM_NTFY_MAILREF_SELECTION);
		msg.AddBool( MSG_MAILS_SELECTED, mHaveSelectedRef);
		SendNotices( BM_NTFY_MAILREF_SELECTION, &msg);
	}
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ItemInvoked( int32 index) {
	BmMailRefItem* refItem;
	refItem = dynamic_cast<BmMailRefItem*>(ItemAt( index));
	if (refItem && !mAvoidInvoke) {
		BmMailRef* ref( refItem->ModelItem());
		if (ref) {
			if (ref->Status() == BM_MAIL_STATUS_DRAFT
			|| ref->Status() == BM_MAIL_STATUS_PENDING) {
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance( ref);
				if (editWin)
					editWin->Show();
			} else {
				BmMailViewWin* viewWin = BmMailViewWin::CreateInstance( ref);
				if (viewWin)
					viewWin->Show();
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::AddMailRefMenu( BMenu* menu, BHandler* target, 
												bool isContextMenu) {
	if (!menu)
		return;
	AddItemToMenu( menu, CreateMenuItem( "Reply", BMM_REPLY), target);
	AddItemToMenu( menu, CreateMenuItem( "Reply To List", BMM_REPLY_LIST), target);
	AddItemToMenu( menu, CreateMenuItem( "Reply To Originator", BMM_REPLY_ORIGINATOR), target);
	AddItemToMenu( menu, CreateMenuItem( "Reply To All", BMM_REPLY_ALL), target);
	AddItemToMenu( menu, CreateMenuItem( "Forward As Attachment", BMM_FORWARD_ATTACHED), target);
	AddItemToMenu( menu, CreateMenuItem( "Forward Inline", BMM_FORWARD_INLINE), target);
	AddItemToMenu( menu, CreateMenuItem( "Forward Inline (With Attachments)", BMM_FORWARD_INLINE_ATTACH), target);
	AddItemToMenu( menu, CreateMenuItem( "Redirect", BMM_REDIRECT), target);
	menu->AddSeparatorItem();

	BMenu* statusMenu = new BMenu( MENU_MARK_AS);
	const char* stats[] = {
		BM_MAIL_STATUS_DRAFT, BM_MAIL_STATUS_FORWARDED, BM_MAIL_STATUS_NEW, 
		BM_MAIL_STATUS_PENDING, BM_MAIL_STATUS_READ, BM_MAIL_STATUS_REDIRECTED,
		BM_MAIL_STATUS_REPLIED, BM_MAIL_STATUS_SENT,	NULL
	};
	for( int i=0; stats[i]; ++i) {
		BMessage* msg = new BMessage( BMM_MARK_AS);
		msg->AddString( BmApplication::MSG_STATUS, stats[i]);
		AddItemToMenu( statusMenu, CreateMenuItem( stats[i], msg, (BmString("MarkAs")+stats[i]).String()), target);
	}
	BFont font( *be_plain_font);
	font.SetSize( 10);
	if (isContextMenu)
		statusMenu->SetFont( &font);
	menu->AddItem( statusMenu);
	menu->AddSeparatorItem();

	BMessage moveMsgTempl( BMM_MOVE);
	BmMenuController* moveMenu
		= new BmMenuController( MENU_MOVE, target, moveMsgTempl, TheMailFolderList.Get(), true);
	if (isContextMenu)
		moveMenu->SetFont( &font);
	menu->AddItem( moveMenu);
	menu->AddSeparatorItem();

	AddItemToMenu( menu, CreateMenuItem( "Filter (Applies Associated Chain)", new BMessage( BMM_FILTER), "Filter"), target);

	BMessage filterMsgTempl( BMM_FILTER);
	BmMenuController* filterMenu
		= new BmMenuController( MENU_FILTER, target,	filterMsgTempl, TheFilterList.Get());
	if (isContextMenu)
		filterMenu->SetFont( &font);
	menu->AddItem( filterMenu);
	AddItemToMenu( menu, CreateMenuItem( "Create Filter From Mail...", new BMessage( BMM_CREATE_FILTER)), target);

	menu->AddSeparatorItem();

	if (isContextMenu) {
		AddItemToMenu( menu, CreateMenuItem( "Print Message(s)...", BMM_PRINT, "Print Message..."), target);
		menu->AddSeparatorItem();
	}
	AddItemToMenu( menu, CreateMenuItem( "Move To Trash", BMM_TRASH), target);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "MailFolderViewMenu", false, false);

	BFont font( *be_plain_font);
	font.SetSize( 10);
	theMenu->SetFont( &font);

	AddMailRefMenu( theMenu, Window(), true);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
	theMenu->SetAsyncAutoDestruct( true);
  	theMenu->Go( point, true, false, openRect, true);
}
