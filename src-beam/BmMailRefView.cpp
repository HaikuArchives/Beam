/*
	BmMailRefView.cpp
		$Id$
*/

#include <Window.h>

#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

const int16 BmMailRefItem::nFirstTextCol = 3;

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
	COL_PRIORITY
};

/********************************************************************************\
	BmMailRefItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item)
{
	BmMailRef* ref = dynamic_cast<BmMailRef*>( _item);

	Bold( ref->IsNew());

	BString st = BString("Mail_") << ref->Status();
	BBitmap* icon = TheResources->IconByName(st);
	SetColumnContent( COL_STATUS_I, icon, 2.0, false);

	if (ref->HasAttachments()) {
		icon = TheResources->IconByName("Attachment");
		SetColumnContent( COL_ATTACHMENTS_I, icon, 2.0, false);
	}
	
	BString priority = BString("Priority_") << ref->Priority();
	if ((icon = TheResources->IconByName(priority))) {
		SetColumnContent( COL_PRIORITY_I, icon, 2.0, false);
	}

	BmListColumn cols[] = {
		{ ref->From().String(),						false },
		{ ref->Subject().String(),					false },
		{ ref->WhenString().String(),				false },
		{ ref->SizeString().String(),				true  },
		{ ref->Cc().String(),						false },
		{ ref->Account().String(),					false },
		{ ref->To().String(),						false },
		{ ref->ReplyTo().String(),					false },
		{ ref->Name().String(),						false },
		{ ref->CreatedString().String(),			false },
		{ ref->TrackerName(),						false },
		{ ref->Status().String(),					false },
		{ ref->HasAttachments() ? "*" : "",		false },
		{ ref->Priority().String(),				false },
		{ NULL, false }
	};
	SetTextCols( nFirstTextCol, cols, !ThePrefs->GetBool("StripedListView"));
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
	BmMailRef* ref = ModelItem();
	if (flags & BmMailRef::UPD_STATUS) {
		Bold( ref->IsNew());
		BString st = BString("Mail_") << ref->Status();
		BBitmap* icon = TheResources->IconByName(st);
		SetColumnContent( COL_STATUS_I, icon, 2.0, false);
		SetColumnContent( COL_STATUS, ref->Status().String(), false);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const int32 BmMailRefItem::GetNumValueForColumn( int32 column_index) const {
	BmMailRef* ref = ModelItem();
	if (column_index == 0 || column_index == 14) {
		// status
		BString st = ref->Status();
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
	const BmMailRef* ref = ModelItem();
	if (column_index == COL_DATE)
		return ref->When();
	else if (column_index == COL_CREATED)
		return ref->Created();
	else
		return 0;		// we don't know this date-column !?!
}



/********************************************************************************\
	BmMailRefView
\********************************************************************************/


BmMailRefView* BmMailRefView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmMailRefView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_MailRefView", B_MULTIPLE_SELECTION_LIST, 
					  false, true, true, true)
	,	mCurrFolder( NULL)
	,	mMouseIsDown( false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;
	Initialize( BRect(0,0,width-1,height-1), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_ALL, true, true, true, B_FANCY_BORDER);

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
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( 3);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::~BmMailRefView() { 
	theInstance = NULL;
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
																	BMessage* archive) {
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
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailRefView: ") << err.what());
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
				if (mods & (B_CONTROL_KEY)) {
					// remove modifiers so we don't ping-pong endlessly:
					Window()->CurrentMessage()->ReplaceInt32("modifiers", 0);
					if (mPartnerMailView)
						mPartnerMailView->KeyDown( bytes, numBytes);
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			case B_DELETE: {
				int32 selCount;
				for( selCount=0; CurrentSelection( selCount)>=0; ++selCount)
					;
				if (!selCount)
					break;
				entry_ref* refs = new entry_ref [selCount];
				int32 currIdx;
				for( int32 i=0; i<selCount && (currIdx=CurrentSelection( i))>=0; ++i) {
					BmMailRefItem* refItem = dynamic_cast<BmMailRefItem*>(ItemAt( currIdx));
					refs[i] = dynamic_cast<BmMailRef*>(refItem->ModelItem())->EntryRef();
				}
				MoveToTrash( refs, selCount);
				delete [] refs;
				Select( currIdx+1);			// select next item that remains in list
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
	BMessage* message = Window()->CurrentMessage();
	if (message && message->FindInt32("buttons") == B_PRIMARY_MOUSE_BUTTON)
		mMouseIsDown = true;
	inherited::MouseDown( point); 
}

/*------------------------------------------------------------------------------*\
	MouseUp( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::MouseUp(BPoint point) { 
	inherited::MouseUp( point); 
	BMessage* message = Window()->CurrentMessage();
	if (mMouseIsDown && message && !(message->FindInt32("buttons") & B_PRIMARY_MOUSE_BUTTON)) {
		mMouseIsDown = false;
		BRect bounds = Bounds();
		if (bounds.Contains( point))
			SelectionChanged();
	}
}

/*------------------------------------------------------------------------------*\
	InitiateDrag()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefView::InitiateDrag( BPoint where, int32 index, bool wasSelected) {
	if (!wasSelected)
		return false;
	BMessage dragMsg( B_SIMPLE_DATA);
	dragMsg.AddString( "be:types", "text/x-email");
	dragMsg.AddString( "be:type_descriptions", "E-mail");
	dragMsg.AddInt32( "be:actions", B_MOVE_TARGET);
	dragMsg.AddInt32( "be:actions", B_TRASH_TARGET);
	BmMailRefItem* refItem = dynamic_cast<BmMailRefItem*>(ItemAt( index));
	BmMailRef* ref = dynamic_cast<BmMailRef*>(refItem->ModelItem());
	dragMsg.AddString( "be:clip_name", ref->TrackerName());
	dragMsg.AddString( "be:originator", "Beam");
	int32 currIdx;
	// we count the number of selected items:
	int32 selCount;
	for( selCount=0; (currIdx=CurrentSelection( selCount))>=0; ++selCount)
		;
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
		ref = dynamic_cast<BmMailRef*>(refItem->ModelItem());
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
			BString indicator = BString("(...and ") << selCount-3 << " more items)";
			dummyView->DrawString( indicator.String(), 
										  BPoint( 20.0, i*lineHeight+baselineOffset));
		}
	}
	dragImage->Unlock();
	DragMessage( &dragMsg, dragImage, B_OP_ALPHA, BPoint( 10.0, 10.0));
	return true;
}

/*------------------------------------------------------------------------------*\
	AcceptsDropOf( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefView::AcceptsDropOf( const BMessage* msg) {
	if (mCurrFolder && msg && msg->what == B_SIMPLE_DATA) {
		entry_ref eref;
		bool containsMails = false;
		for( int32 i=0; msg->FindRef( "refs", i, &eref) == B_OK; ++i) {
			if (CheckMimeType( &eref, "text/x-email"))
				containsMails = true;
		}
		return containsMails;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::HandleDrop( const BMessage* msg) {
	if (mCurrFolder && msg && msg->what == B_SIMPLE_DATA) {
		if (mCurrFolder) {
			BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
			entry_ref eref;
			for( int i=0; msg->FindRef( BmMailMoverView::MSG_REFS, i, &eref)==B_OK; ++i) {
				tmpMsg.AddRef( BmMailMoverView::MSG_REFS, &eref);
			}
			tmpMsg.AddString( BmJobStatusWin::MSG_JOB_NAME, mCurrFolder->Name());
			tmpMsg.AddPointer( BmMailMoverView::MSG_FOLDER, mCurrFolder.Get());
			TheJobStatusWin->PostMessage( &tmpMsg);
		}
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
		BmMailRefList* refList = folder ? folder->MailRefList() : NULL;
		if (mPartnerMailView)
			mPartnerMailView->ShowMail( static_cast< BmMailRef*>( NULL));
		if (refList)
			StartJob( refList, true);
		mCurrFolder = folder;
		SelectionChanged();
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BString BmMailRefView::StateInfoBasename()	{ 
	return "MailRefView";
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
const BMessage* BmMailRefView::DefaultLayout()		{ 
	return ThePrefs->GetMsg("MailRefLayout"); 
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::SelectionChanged( void) {
	int32 temp;
	int32 selection = -1;
	int32 numSelected = 0;
	if (mMouseIsDown)
		return;									// only react when mouse-button is up again
	while( (temp = CurrentSelection(numSelected)) >= 0) {
		selection = temp;
		numSelected++;
	}
	if (selection >= 0 && numSelected == 1) {
		BmMailRefItem* refItem;
		refItem = dynamic_cast<BmMailRefItem*>(ItemAt( selection));
		if (refItem) {
			BmMailRef* ref = dynamic_cast<BmMailRef*>(refItem->ModelItem());
			if (ref && mPartnerMailView)
				mPartnerMailView->ShowMail( ref);
		}
	} else
		if (mPartnerMailView)
			mPartnerMailView->ShowMail( static_cast< BmMailRef*>( NULL));
	
	BMessage msg(BM_NTFY_MAILREF_SELECTION);
	msg.AddInt32( MSG_MAILS_SELECTED, numSelected);
	SendNotices( BM_NTFY_MAILREF_SELECTION, &msg);
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ItemInvoked( int32 index) {
	BmMailRefItem* refItem;
	refItem = dynamic_cast<BmMailRefItem*>(ItemAt( index));
	if (refItem) {
		BmMailRef* ref = dynamic_cast<BmMailRef*>(refItem->ModelItem());
		if (ref) {
			if (ref->Status() == BM_MAIL_STATUS_DRAFT
			|| ref->Status() == BM_MAIL_STATUS_PENDING) {
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance();
				if (editWin) {
					editWin->EditMail( ref);
					editWin->Show();
				}
			} else {
				BmMailViewWin* viewWin = BmMailViewWin::CreateInstance();
				if (viewWin) {
					viewWin->ShowMail( ref);
					viewWin->Show();
				}
			}
		}
	}
}
