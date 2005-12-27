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
#include <Region.h>
#include <Window.h>

#include "BubbleHelper.h"
#include "CLVColumnLabelView.h"

#include "BeamApp.h"
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
#include "BmRosterBase.h"
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
	COL_WHEN_CREATED,
	COL_TRACKER_NAME,
	COL_STATUS,
	COL_ATTACHMENTS,
	COL_PRIORITY,
	COL_IDENTITY,
	COL_CLASSIFICATION,
	COL_RATIO_SPAM,
	COL_END
};

/********************************************************************************\
	BmDateWidthAdjuster
\********************************************************************************/
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmDateWidthAdjuster::BmDateWidthAdjuster(CLVEasyItem* i)
	:	cachedWidth(0)
	,	item(i)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const char* BmDateWidthAdjuster::operator() (int32 colIdx, time_t utc)
{
	CLVColumn* column = item->ColumnAt(colIdx);
	if (!column)
		return "";
	if (column->Width() != cachedWidth) {
		// TODO: replace these formats with localized versions!
		if (ThePrefs->GetBool( "UseSwatchTimeInRefView", false)) {
			const char* formats[] = {
				"%A, %Y-%m-%d @",
				"%a, %Y-%m-%d @",
				"%Y-%m-%d @",
				"%y-%m-%d @",
				"%y-%m-%d",
				NULL
			};
			for( const char** f = formats; *f; ++f) {
				dateStr = TimeToSwatchString( utc, *f);
				if (item->ColumnFitsText(colIdx, dateStr.String()))
					break;
			}
		} else {
			const char* formats[] = {
				"%A, %Y-%m-%d %H:%M:%S",
				"%a, %Y-%m-%d %H:%M:%S",
				"%a, %Y-%m-%d %H:%M",
				"%Y-%m-%d %H:%M",
				"%y-%m-%d %H:%M",
				"%y-%m-%d",
				NULL
			};
			for( const char** f = formats; *f; ++f) {
				dateStr = TimeToString( utc, *f);
				if (item->ColumnFitsText(colIdx, dateStr.String()))
					break;
			}
		}
		cachedWidth = column->Width();
	}
	return dateStr.String();
}

/********************************************************************************\
	BmMailRefItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem( ColumnListView* lv, 
										BmListModelItem* _item)
	:	inherited( lv, _item)
	,	mWhenStringAdjuster(this)
	,	mWhenCreatedStringAdjuster(this)
{
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
void BmMailRefItem::UpdateView( BmUpdFlags flags, bool redraw, 
										  uint32 updColBitmap) {
	BmMailRef* ref( ModelItem());
	if (!ref)
		return;
	BmBitmapHandle* icon = NULL;

	if (flags & BmMailRef::UPD_STATUS) {
		Bold( ref->IsSpecial());
		BmString st = BmString("Mail_") << ref->Status();
		icon = TheResources->IconByName(st);
		SetColumnContent( COL_STATUS_I, icon);
		if (redraw) {
			updColBitmap = 0xFFFFFFFF;
							// Bold() may have changed font, need to redraw everything!
			mWhenStringAdjuster.cachedWidth = 0;
			mWhenCreatedStringAdjuster.cachedWidth = 0;
							// trigger re-adjustment of date columns
		}
	}
	if (flags & BmMailRef::UPD_ATTACHMENTS) {
		if (ref->HasAttachments()) {
			icon = TheResources->IconByName("Attachment");
			SetColumnContent( COL_ATTACHMENTS_I, icon);
		}
		updColBitmap |= (1UL<<COL_ATTACHMENTS | 1UL<<COL_ATTACHMENTS_I);
	}
	if (flags & BmMailRef::UPD_PRIORITY) {
		BmString priority = BmString("Priority_") << ref->Priority();
		if ((icon = TheResources->IconByName(priority))!=NULL) {
			SetColumnContent( COL_PRIORITY_I, icon);
		}
		updColBitmap |= (1UL<<COL_PRIORITY | 1UL<<COL_PRIORITY_I);
	}
	if (flags & BmMailRef::UPD_ACCOUNT)
		updColBitmap |= (1UL << COL_ACCOUNT);
	if (flags & BmMailRef::UPD_CC)
		updColBitmap |= (1UL << COL_CC);
	if (flags & BmMailRef::UPD_FROM)
		updColBitmap |= (1UL << COL_FROM);
	if (flags & BmMailRef::UPD_NAME)
		updColBitmap |= (1UL << COL_NAME);
	if (flags & BmMailRef::UPD_WHEN_CREATED)
		updColBitmap |= (1UL << COL_WHEN_CREATED);
	if (flags & BmMailRef::UPD_REPLYTO)
		updColBitmap |= (1UL << COL_REPLY_TO);
	if (flags & BmMailRef::UPD_SIZE)
		updColBitmap |= (1UL << COL_SIZE);
	if (flags & BmMailRef::UPD_SUBJECT)
		updColBitmap |= (1UL << COL_SUBJECT);
	if (flags & BmMailRef::UPD_TO)
		updColBitmap |= (1UL << COL_TO);
	if (flags & BmMailRef::UPD_WHEN)
		updColBitmap |= (1UL << COL_DATE);
	if (flags & BmMailRef::UPD_IDENTITY)
		updColBitmap |= (1UL << COL_IDENTITY);
	if (flags & BmMailRef::UPD_TRACKERNAME)
		updColBitmap |= (1UL << COL_TRACKER_NAME);
	if (flags & BmMailRef::UPD_CLASSIFICATION)
		updColBitmap |= (1UL << COL_CLASSIFICATION);
	if (flags & BmMailRef::UPD_RATIO_SPAM)
		updColBitmap |= (1UL << COL_RATIO_SPAM);
	inherited::UpdateView( flags, redraw, updColBitmap);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const int32 BmMailRefItem::GetNumValueForColumn( int32 column_index) const {
	BmMailRef* ref( ModelItem());
	if (column_index == COL_STATUS_I || column_index == COL_STATUS) {
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
	} else if (column_index == COL_ATTACHMENTS_I 
	|| column_index == COL_ATTACHMENTS) {
		return ref->HasAttachments() ? 0 : 1;	
							// show mails with attachment at top
	} else if (column_index == COL_PRIORITY_I || column_index == COL_PRIORITY) {
		int16 prio = atol( ref->Priority().String());
		return (prio>=1 && prio<=5) ? prio : 3;
							// illdefined priority means medium priority (=3)
	} else if (column_index == COL_SIZE) {
		return ref->Size();
	} else if (column_index == COL_RATIO_SPAM) {
		return (int32)(ref->RatioSpam()*1000);	
							// errr, since we are returning int32!
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
	else
		return 0;		// we don't know this date-column !?!
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const bigtime_t BmMailRefItem::GetBigtimeValueForColumn( int32 column_index) const {
	BmMailRef* ref( ModelItem());
	if (column_index == COL_WHEN_CREATED)
		return ref->WhenCreated();
	else
		return 0;		// we don't know this bigtime-column !?!
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
		text = mWhenStringAdjuster( colIdx, ref->When());
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
	case COL_WHEN_CREATED: {
		text = mWhenCreatedStringAdjuster( colIdx, ref->WhenCreated()/(1000*1000));
		break;
	}
	case COL_TRACKER_NAME:
		text = ref->TrackerName();
		break;
	case COL_STATUS:
		text = ref->Status().String();
		break;
	case COL_ATTACHMENTS:
		text = ref->HasAttachments() ? "*" : "";
		break;
	case COL_PRIORITY:
		text = ref->Priority().String();
		break;
	case COL_IDENTITY:
		text = ref->Identity().String();
		break;
	case COL_CLASSIFICATION:
		if (ThePrefs->GetBool("MapClassificationGenuineToTofu", true)
		&& ref->Classification() == BM_MAIL_CLASS_TOFU)
			text = "Tofu";
		else
			text = ref->Classification().String();
		break;
	case COL_RATIO_SPAM:
		text = ref->RatioSpamString().String();
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


const char* const BmMailRefView::MSG_MAILS_SELECTED = "bm:msel";
const char* const BmMailRefView::MENU_MARK_AS =			"Set Status To";
const char* const BmMailRefView::MENU_FILTER = 			"Apply Specific Filter";
const char* const BmMailRefView::MENU_MOVE =				"Move To";

const BmString BmDragId = "beam/ref";

enum {
	BMM_CONNECT_LAYOUT = 'bmCL'
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( int32 width, int32 height) {
	return new BmMailRefView( width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( int32 width, int32 height)
	:	inherited( BRect(0,0,width-1,height-1), "Beam_MailRefView", 
					  B_MULTIPLE_SELECTION_LIST, false, true)
	,	mCurrFolder( NULL)
	,	mHaveSelectedRef( false)
	,	mStateInfoConnectedToParentFolder( true)
{
	int32 flags = CLV_SORT_KEYABLE;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	AddColumn( new CLVColumn( "S", 18.0, 
									  flags | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER
									  | CLV_COLTYPE_BITMAP, 
									  18.0, "(S)tatus [Icon]"));
	AddColumn( new CLVColumn( "A", 18.0, 
									  flags | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER
									  | CLV_COLTYPE_BITMAP, 
									  18.0, "(A)ttachments [Icon]"));
	AddColumn( new CLVColumn( "P", 18.0, 
									  flags | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER
									  | CLV_COLTYPE_BITMAP, 
									  18.0, "(P)riority [Icon]"));
	AddColumn( new CLVColumn( "From", 150.0, flags | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "Subject", 300.0, flags | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "Date-Sent", 100.0, 
									  flags | CLV_COLDATA_DATE | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "Size", 60.0, 
									  flags | CLV_COLDATA_NUMBER | CLV_RIGHT_JUSTIFIED
									  | CLV_COLTYPE_USERTEXT,
									  20.0));
	AddColumn( new CLVColumn( "Cc", 100.0, flags | CLV_COLTYPE_USERTEXT, 20.0));
	AddColumn( new CLVColumn( "Account", 120.0, flags | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "To", 150.0, flags | CLV_COLTYPE_USERTEXT, 20.0));
	AddColumn( new CLVColumn( "Reply-To", 150.0, flags | CLV_COLTYPE_USERTEXT,
									  20.0));
	AddColumn( new CLVColumn( "Name", 150.0, flags | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "Date-Received", 110.0, 
									  flags | CLV_COLDATA_BIGTIME | CLV_COLTYPE_USERTEXT,
									  20.0));
	AddColumn( new CLVColumn( "Tracker-Name", 150.0, 
									  flags | CLV_COLTYPE_USERTEXT, 
									  20.0));
	AddColumn( new CLVColumn( "S", 100.0, flags | CLV_COLTYPE_USERTEXT, 
									  40.0, "(S)tatus [Text]"));
	AddColumn( new CLVColumn( "A", 100.0, 
									  flags | CLV_COLDATA_NUMBER | CLV_COLTYPE_USERTEXT,
									  18.0, "(A)ttachments [Text]"));
	AddColumn( new CLVColumn( "P", 100.0, 
									  flags | CLV_COLDATA_NUMBER | CLV_COLTYPE_USERTEXT,
									  18.0, "(P)riority [Text]"));
	AddColumn( new CLVColumn( "Identity", 120.0, flags | CLV_COLTYPE_USERTEXT, 
									  40.0));
	AddColumn( new CLVColumn( "Class", 40.0, flags | CLV_COLTYPE_USERTEXT, 
									  40.0));
	AddColumn( new CLVColumn( "RatioSpam", 100.0, flags | CLV_COLDATA_NUMBER 
										| CLV_RIGHT_JUSTIFIED| CLV_COLTYPE_USERTEXT, 
									  40.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_WHEN_CREATED);
	SetSortMode( COL_WHEN_CREATED, Descending, false);
	int32 displayOrder[] = {
		COL_STATUS_I, COL_ATTACHMENTS_I, COL_NAME, COL_SUBJECT, COL_WHEN_CREATED, 
		COL_SIZE, COL_IDENTITY, COL_ACCOUNT,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};
	SetDisplayOrder(displayOrder);
	
	TheBubbleHelper->SetHelp( ColumnLabelView(), "Right-Click to show/hide columns");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::~BmMailRefView() { 
	TheBubbleHelper->SetHelp( ColumnLabelView(), NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailRefView::CreateListViewItem( BmListModelItem* item, 
																	BMessage*) {
	return new BmMailRefItem( this, item);
}

/*------------------------------------------------------------------------------*\
	ColumnWidthChanged()
		-	reformat date columns when the width changes
\*------------------------------------------------------------------------------*/
void BmMailRefView::ColumnWidthChanged(int32 colIdx, float NewWidth)
{
	inherited::ColumnWidthChanged(colIdx, NewWidth);
	CLVColumn* column = (CLVColumn*)ColumnAt(colIdx);
	if (!column || column->Flags()&(CLV_COLDATA_DATE|CLV_COLDATA_BIGTIME) == 0)
		return;
	BRect colBounds = Bounds();
	colBounds.left = column->ColumnBegin();
	colBounds.right = column->ColumnEnd();
	Invalidate( colBounds);
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
					if (origMsg && BmDragId==origMsg->FindString("be:originator")) {
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
				if (modifiers() 
				& (B_SHIFT_KEY | B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					bool passedOn = false;
					if (mPartnerMailView 
					&& !(passedOn = msg->FindBool("bm:passed_on"))) {
						BMessage msg2(*msg);
						msg2.AddBool("bm:passed_on", true);
						Looper()->PostMessage( &msg2, mPartnerMailView);
						return;
					}
				}
				inherited::MessageReceived( msg);
				break;
			}
			case B_MODIFIERS_CHANGED: {
				if (modifiers() & B_MENU_KEY) {
					ShowMenu( BPoint(100,50));
				}
				break;
			}
			case BMM_CONNECT_LAYOUT: {
				if (!mCurrFolder)
					return;
				bool connected = mCurrFolder->RefListStateInfoConnectedToParent();
				mCurrFolder->RefListStateInfoConnectedToParent( !connected);
				ReadStateInfo();
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
	TrashSelectedMessages()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::TrashSelectedMessages() {
	BMessage msg(BMM_TRASH);
	AddSelectedRefsToMsg( &msg);

	// note down selection...
	int32 firstIdx = CurrentSelection( 0);
	int32 currIdx;
	int32 lastIdx = firstIdx;
	for( int32 i=0; (currIdx=CurrentSelection( i))>=0; ++i)
		lastIdx = currIdx;

	// ...and move cursor onwards...
	bool selectNext 
		= ThePrefs->GetBool( "SelectNextMailAfterDelete", true);
	if (selectNext && lastIdx < CountItems()-1)
		Select( lastIdx+1);
				// select next item that remains in list
	else if (selectNext && firstIdx > 0)
		Select( firstIdx-1);
				// select last item that remains in list
	else
		DeselectAll();

	// ...finally tell app to delete mails:
	be_app_messenger.SendMessage( &msg);
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::KeyDown(const char *bytes, int32 numBytes) { 
	if ( numBytes == 1 ) {
		switch( bytes[0]) {
			// implement remote navigation within mail-view
			// (via cursor-keys with modifiers):
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
				TrashSelectedMessages();
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
	dragMsg.AddString( "be:originator", BmDragId.String());
	// now we add all selected items to drag-msg:
	int32 currIdx;
	for( int32 i=0; (currIdx=CurrentSelection( i))>=0; ++i) {
		refItem = dynamic_cast<BmMailRefItem*>(ItemAt( currIdx));
		BmMailRef* ref( refItem->ModelItem());
		dragMsg.AddRef( "refs", ref->EntryRefPtr());
		if (i%100==0)
			BM_LOG2( BM_LogGui, BmString("MailRefView::InitiateDrag() - processing ")<<i<<"th selection");
	}
	vector<int> cols;
	cols.push_back(0);
	cols.push_back(11);
	cols.push_back(4);
	BBitmap* dragImage = CreateDragImage(cols);
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
void BmMailRefView::HandleDrop( BMessage* msg) {
	type_code tc;
	int32 refCount;
	if (mCurrFolder && msg && msg->what == B_SIMPLE_DATA
	&& msg->GetInfo( "refs", &tc, &refCount) == B_OK) {
		BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
		entry_ref eref;
		entry_ref* refs = new entry_ref [refCount];
		int i=0;
		while(  msg->FindRef( "refs", i, &refs[i]) == B_OK)
			++i;
		tmpMsg.AddPointer( BmMailMover::MSG_REFS, (void*)refs);
		tmpMsg.AddInt32( BmMailMover::MSG_REF_COUNT, i);
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
		BmRef<BmMailRefList> refList( folder 
													? folder->MailRefList().Get() 
													: NULL);
		if (mPartnerMailView)
			mPartnerMailView->ShowMail( static_cast< BmMailRef*>( NULL));
		DetachModel();
		MakeEmpty();
		mCurrFolder = folder;
		if (refList)
			StartJob( refList.Get());
		SelectionChanged();
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::JobIsDone( bool completed) {
	if (IsHidden())
		// if view is hidden, we avoid adding all view-items:
		completed = false;
	inherited::JobIsDone( completed);
	if (completed && mCurrFolder) {
		BmRef<BmMailRefList> refList( mCurrFolder->MailRefList().Get());
		if (refList) {
			BmRef<BmListModelItem> refItem 
				= refList->FindItemByKey( mCurrFolder->SelectedRefKey());
			BmListViewItem* viewItem = FindViewItemFor( refItem.Get());
			int32 idx = IndexOf( viewItem);
			if (idx >= 0) {
				Select( idx);
				// show the selected item centered within the view (if possible):
				BRect frame = ItemFrame( idx);
				float newYPos = frame.top-(Bounds().Height()-frame.Height())/2.0;
				ScrollTo( BPoint( 0, MAX( 0, newYPos)));
			}
		}
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
	StateInfoFilename( )
		-	
\*------------------------------------------------------------------------------*/
BmString BmMailRefView::StateInfoFilename( bool forRead) {
	if (mCurrFolder) {
		BmRef<BmListModelItem> folderRef = mCurrFolder.Get();
		BmString stateInfoFilename;
		while( folderRef) {
			BmMailFolder* folder = dynamic_cast<BmMailFolder*>(folderRef.Get());
			if (folder) {
				stateInfoFilename
					= StateInfoBasename() 
						<< "_" << folder->Key() << " (" << folder->Name() << ")";
				if (!folder->RefListStateInfoConnectedToParent()) {
					BEntry stateInfoEntry( BeamRoster->StateInfoFolder(), 
												  stateInfoFilename.String());
					// if forRead==true, we want to return the first file that
					// exists, but if we are going to create it anyway, we want
					// to return the filename the info should be stored into (i.e.
					// the first item in the hierarchy that isn't connected to
					// its parent).
					if (!forRead || stateInfoEntry.Exists())
						break;
				}
			}
			folderRef = folder->Parent();
		}
		return stateInfoFilename;
	}
	return inherited::StateInfoFilename( forRead);
}

/*------------------------------------------------------------------------------*\
	AddSelectedRefsToMsg( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::AddSelectedRefsToMsg( BMessage* msg) {
	if (mPartnerMailView) {
		BmString selectedText;
		int32 start, finish;
		mPartnerMailView->GetSelection( &start, &finish);
		if (start < finish)
			selectedText.SetTo(mPartnerMailView->Text()+start, finish-start);
		if (selectedText.Length())
			msg->AddString( BeamApplication::MSG_SELECTED_TEXT, 
								 selectedText.String());
	}
	BMessenger msngr( this);
	msg->AddMessenger( BeamApplication::MSG_SENDING_REFVIEW, msngr);
	int32 selected = -1;
	if (CurrentSelection(0) >= 0) {
		BmMailRefVect* refs = new BmMailRefVect();
		BmMailRefItem* refItem;
		for( int32 idx=0; (selected = CurrentSelection(idx)) >= 0; ++idx) {
			refItem = dynamic_cast<BmMailRefItem*>( ItemAt( selected));
			if (refItem)
				refs->push_back( refItem->ModelItem());
		}
		msg->AddPointer( BeamApplication::MSG_MAILREF_VECT, 
						  	  static_cast< void*>( refs));
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
	BmRef<BmMailRef> ref;
	if (selection >= 0 && numSelected == 1) {
		BmMailRefItem* refItem;
		refItem = dynamic_cast<BmMailRefItem*>(ItemAt( selection));
		if (refItem) {
			ref = refItem->ModelItem();
		}
	}
	if (mPartnerMailView)
		mPartnerMailView->ShowMail( ref.Get());
	if (mCurrFolder && mCurrFolder->MailRefList()->InitCheck() == B_OK)
		mCurrFolder->SelectedRefKey( ref ? ref->Key() : BM_DEFAULT_STRING);
	
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
	if (refItem) {
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
	BFont font( *be_plain_font);
	BMenuItem* replyMenuItem = CreateSubMenuItem( "Reply", BMM_REPLY);
	BMenu* replyMenu = replyMenuItem->Submenu();
	if (isContextMenu)
		replyMenu->SetFont( &font);
	AddItemToMenu( menu, replyMenuItem, target);
	AddItemToMenu( replyMenu, 
						CreateMenuItem( "Reply (Auto)", BMM_REPLY, "Reply"), target);
	AddItemToMenu( replyMenu, 
						CreateMenuItem( "Reply To List", BMM_REPLY_LIST), target);
	AddItemToMenu( replyMenu, 
						CreateMenuItem( "Reply To Person", BMM_REPLY_ORIGINATOR), 
						target);
	AddItemToMenu( replyMenu, 
						CreateMenuItem( "Reply To All", BMM_REPLY_ALL), target);

	BMenuItem* fwdMenuItem = CreateSubMenuItem( "Forward", BMM_FORWARD_INLINE);
	BMenu* fwdMenu = fwdMenuItem->Submenu();
	if (isContextMenu)
		fwdMenu->SetFont( &font);
	AddItemToMenu( menu, fwdMenuItem, target);
	AddItemToMenu( fwdMenu, 
						CreateMenuItem( "Forward Inline", 
											 BMM_FORWARD_INLINE,
											 "Forward"), 
						target);
	AddItemToMenu( fwdMenu, 
						CreateMenuItem( "Forward As Attachment", 
											 BMM_FORWARD_ATTACHED), 
						target);
	AddItemToMenu( fwdMenu, 
						CreateMenuItem( "Forward Inline (With Attachments)", 
											 BMM_FORWARD_INLINE_ATTACH), 
						target);
	AddItemToMenu( menu, CreateMenuItem( "Redirect", BMM_REDIRECT), target);
	menu->AddSeparatorItem();

	BmMenuController* statusMenu
		= new BmMenuController( MENU_MARK_AS, target, new BMessage( BMM_MARK_AS), 
										&BmGuiRosterBase::RebuildStatusMenu, 
										BM_MC_MOVE_RIGHT);
	if (isContextMenu)
		statusMenu->SetFont( &font);
	menu->AddItem( statusMenu);
	menu->AddSeparatorItem();

	BmMenuController* moveMenu
		= new BmMenuController( MENU_MOVE, target, new BMessage( BMM_MOVE), 
										&BmGuiRosterBase::RebuildFolderMenu, 
										BM_MC_SKIP_FIRST_LEVEL | BM_MC_MOVE_RIGHT);
	if (isContextMenu)
		moveMenu->SetFont( &font);
	menu->AddItem( moveMenu);
	menu->AddSeparatorItem();

	AddItemToMenu( menu, CreateMenuItem( "Learn as Spam", BMM_LEARN_AS_SPAM), target);
	AddItemToMenu( menu, CreateMenuItem( "Learn as Tofu", BMM_LEARN_AS_TOFU), target);
	menu->AddSeparatorItem();

	AddItemToMenu( menu, 
						CreateMenuItem( "Filter (Applies Associated Chain)", 
											 new BMessage( BMM_FILTER), "Filter"), 
						target);

	BmMenuController* filterMenu
		= new BmMenuController( MENU_FILTER, target,	new BMessage( BMM_FILTER), 
										&BmGuiRosterBase::RebuildFilterMenu, 
										BM_MC_MOVE_RIGHT);
	if (isContextMenu)
		filterMenu->SetFont( &font);
	menu->AddItem( filterMenu);
	AddItemToMenu( menu, 
						CreateMenuItem( "Create Filter From Mail...", 
											 new BMessage( BMM_CREATE_FILTER)), 
						target);

	menu->AddSeparatorItem();

	AddItemToMenu( menu, CreateMenuItem( "Edit As New...", BMM_EDIT_AS_NEW), 
						target);
	menu->AddSeparatorItem();

	if (isContextMenu) {
		AddItemToMenu( menu, 
							CreateMenuItem( "Print...", 
												 BMM_PRINT, "Print Message..."), 
							target);
		menu->AddSeparatorItem();
	} else {
		AddItemToMenu( menu, 
							CreateMenuItem( "Previous Message", BMM_PREVIOUS_MESSAGE),
							target);
		AddItemToMenu( menu, 
							CreateMenuItem( "Next Message", BMM_NEXT_MESSAGE), 
							target);
		menu->AddSeparatorItem();
	}
	AddItemToMenu( menu, CreateMenuItem( "Move To Trash", BMM_TRASH), target);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::PopulateLabelViewMenu( BMenu* menu) {
	BMenuItem* item = new BMenuItem( "Connect Layout to Parent", 
												new BMessage( BMM_CONNECT_LAYOUT));
	if (mCurrFolder && mCurrFolder->RefListStateInfoConnectedToParent())
		item->SetMarked( true);
	item->SetTarget( this);
	menu->AddItem( item);
	menu->AddSeparatorItem();
	inherited::PopulateLabelViewMenu( menu);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "MailFolderViewMenu", false, false);

	BFont font( *be_plain_font);
	theMenu->SetFont( &font);

	AddMailRefMenu( theMenu, Window(), true);

   BRect assumedMenuRect( point.x, point.y, point.x+250, point.y+250);
   BPoint mousePoint;
   uint32 buttons;
	GetMouse( &mousePoint, &buttons);
	if (assumedMenuRect.Contains( mousePoint))
		point = mousePoint + BPoint(1,1);
   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
	theMenu->SetAsyncAutoDestruct( true);
  	theMenu->Go( point, true, true, openRect, true);
}
