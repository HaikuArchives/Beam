/*
	BmBodyPartView.cpp
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
#include <FilePanel.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmEncoding.h"
#include "BmLogHandler.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmMailView.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmBodyPartItem
\********************************************************************************/

class BmBodyPartItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	BmBodyPartItem( ColumnListView* lv, BmListModelItem* item);
	~BmBodyPartItem();

	BmBodyPart* ModelItem() const	{ return dynamic_cast<BmBodyPart*>(mModelItem.Get()); }

	// Hide copy-constructor and assignment:
	BmBodyPartItem( const BmBodyPartItem&);
	BmBodyPartItem operator=( const BmBodyPartItem&);
};

enum Columns {
	COL_EXPANDER = 0,
	COL_ICON,
	COL_NAME,
	COL_MIMETYPE,
	COL_SIZE,
	COL_DESCRIPTION,
	COL_TRANSFER_ENC,
	COL_CHARSET,
	COL_LANGUAGE
};

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

const BmString BmDragId = "beam/att";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartItem::BmBodyPartItem( ColumnListView* lv, 
										  BmListModelItem* _item)
	:	inherited( lv, _item)
{
	BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( _item);

	BmBitmapHandle* icon = TheResources->IconByName( bodyPart->MimeType());
	SetColumnContent( COL_ICON, icon, 2.0);

	BmString sizeString = bodyPart->IsMultiPart() 
								? BM_DEFAULT_STRING 
								: ThePrefs->GetBool( "ShowDecodedLength", true) 
									? BytesToString( bodyPart->DecodedLength(), true)
									: BytesToString( bodyPart->BodyLength(), true);

	// set column-values:
	const char* cols[] = {
		bodyPart->FileName().String(),
		bodyPart->MimeType().String(),
		sizeString.String(),
		bodyPart->Description().String(),
		bodyPart->TransferEncoding().String(),
		bodyPart->IsText() 
			? bodyPart->SuggestedCharset().String() 
			: "",
		bodyPart->Language().String(),
		NULL
	};
	SetTextCols( BmBodyPartView::nFirstTextCol, cols);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartItem::~BmBodyPartItem() { 
}



/********************************************************************************\
	BmBodyPartView
\********************************************************************************/
const int16 BmBodyPartView::nFirstTextCol = 2;
float BmBodyPartView::nColWidths[10] = {10,20,20,20,20,20,20,20,20,0};

const char* const BmBodyPartView::MSG_SHOWALL = "bm:showall";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::BmBodyPartView( minimax minmax, int32 width, int32 height, 
										  bool editable)
	:	inherited( BRect(0,0,width-1,height-1), "Beam_BodyPartView", 
					  B_MULTIPLE_SELECTION_LIST, true, true)
	,	mShowAllParts( false)
	,	mEditable( editable)
	,	mSavePanel( NULL)
	,	mIsUsedForPrinting( false)
	,	mInUpdate( false)
{
	SetViewColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
	fLightColumnCol = ui_color( B_UI_PANEL_BACKGROUND_COLOR);
	fSelectedItemColorWindowActive = 
	fSelectedItemColorWindowInactive = BmWeakenColor( B_UI_SHADOW_COLOR, 3);

	SetResizingMode( B_FOLLOW_NONE);
	int32 flags = Flags();
	flags &= (B_NAVIGABLE^0xFFFFFFFF);
	SetFlags( flags);

	UseStateCache( false);

	AddColumn( new CLVColumn( NULL, nColWidths[0], 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING 
									  | CLV_NOT_MOVABLE | CLV_COLTYPE_BITMAP, 10.0));
	AddColumn( new CLVColumn( "Icon", nColWidths[1], 
									  CLV_PUSH_PASS | CLV_COLTYPE_BITMAP, 18.0));
	AddColumn( new CLVColumn( "Name", nColWidths[2], 0, 20.0));
	AddColumn( new CLVColumn( "Mimetype", nColWidths[3], 0, 20.0));
	AddColumn( new CLVColumn( "Size", nColWidths[4], CLV_RIGHT_JUSTIFIED, 20.0));
	AddColumn( new CLVColumn( "Description", nColWidths[5], 0, 20.0));
	AddColumn( new CLVColumn( "Transfer", nColWidths[6], 0, 20.0));
	AddColumn( new CLVColumn( "", nColWidths[7], 0, 20.0));
	AddColumn( new CLVColumn( "", nColWidths[8], 0, 20.0));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::~BmBodyPartView() { 
	delete mSavePanel;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmBodyPartView::Archive( BMessage* archive, bool) const {
	status_t ret = archive->AddBool( MSG_SHOWALL, mShowAllParts);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmBodyPartView::Unarchive( const BMessage* archive, bool) {
	status_t ret = archive->FindBool( MSG_SHOWALL, &mShowAllParts);
	fAvoidColPushing = !mShowAllParts;
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmBodyPartView::CreateListViewItem( BmListModelItem* item, 
																	 BMessage*) {
	BmBodyPart* modelItem = dynamic_cast<BmBodyPart*>( item);
	if (mShowAllParts || !modelItem->IsMultiPart() 
	&& !modelItem->ShouldBeShownInline()) {
		return new BmBodyPartItem( this, item);
	} else {
		return NULL;
	}
}

/*------------------------------------------------------------------------------*\
	ShowBody()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ShowBody( BmBodyPartList* body) {
	try {
		StopJob();
		if (!body) {
			ResizeTo( Bounds().Width(), 0);
			MoveTo( 0, 0);
			SetViewColor( ui_color( B_UI_DOCUMENT_BACKGROUND_COLOR));
		} else {
			SetViewColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
			StartJob( body, !mIsUsedForPrinting);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("BodyPartView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddAttachment()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAttachment( const BMessage* msg) {
	if (!mEditable)
		return;
	entry_ref ref;
	BmRef<BmDataModel> modelRef( DataModel());
	BmBodyPartList* bodyPartList 
		= dynamic_cast<BmBodyPartList*>( modelRef.Get());
	if (!bodyPartList)
		return;
	for( int i=0; msg->FindRef( "refs", i, &ref) == B_OK; ++i) {
		bodyPartList->AddAttachmentFromRef( &ref, mDefaultCharset);
	}
	BmBodyPart* droppedBodyPart = NULL;
	if (msg->FindPointer( "bm:bodypart", (void**)&droppedBodyPart) == B_OK) {
		// we take a shortcut: if dragging from one mail-view to the other,
		// we just copy the body-part:
		BmRef<BmBodyPart> newBodyPart( new BmBodyPart(*droppedBodyPart));
		bodyPartList->AddItemToList( newBodyPart.Get());
	}
	ShowBody( bodyPartList);
}

/*------------------------------------------------------------------------------*\
	AddAttachment()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAttachment( const char* path) {
	if (!mEditable)
		return;
	BmRef<BmDataModel> modelRef( DataModel());
	BmBodyPartList* bodyPartList 
		= dynamic_cast<BmBodyPartList*>( modelRef.Get());
	if (!bodyPartList)
		return;
	BEntry entry(path);
	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK)
		bodyPartList->AddAttachmentFromRef( &ref, mDefaultCharset);
	ShowBody( bodyPartList);
}

/*------------------------------------------------------------------------------*\
	AdjustVerticalSize()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AdjustVerticalSize() {
	float width = Bounds().Width();
	int32 count = CountItems();
	float itemHeight = count ? ItemAt(0)->Height()+1 : 0;
							// makes this view disappear if no BodyPart is shown
	ResizeTo( width, count*itemHeight);
	Invalidate();
	BmMailView* mailView = (BmMailView*)Parent();
	if (mailView) {
		mailView->ScrollTo( 0, 0);
		mailView->CalculateVerticalOffset();
		mailView->UpdateParsingStatus();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ExpansionChanged( CLVListItem* _item, bool expanded) {
	inherited::ExpansionChanged( _item, expanded);
	if (!mInUpdate)
		AdjustVerticalSize();
}

/*------------------------------------------------------------------------------*\
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAllModelItems() {
	// initializations:
	int i;
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		mColWidths[i] = nColWidths[i];
	}
	// do add all items:
	inherited::AddAllModelItems();
	// update info about maximum column widths:
	int32 count = FullListCountItems();
	mInUpdate = true;
	for( int i=0; i<count; ++i) {
		BmListViewItem* viewItem 
			= dynamic_cast<BmListViewItem*>( FullListItemAt( i));
		for( int c=nFirstTextCol; c<CountColumns(); ++c) {
			float textWidth = StringWidth( viewItem->GetColumnContentText(c));
			mColWidths[c] 
				= MAX( 
					mColWidths[c], 
					textWidth+10+EXPANDER_SHIFT*viewItem->OutlineLevel()
				);
		}
		Expand(FullListItemAt(i));
	}
	// adjust column widths, if neccessary:
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		ColumnAt( i)->SetWidth( mColWidths[i]);
	}
	mInUpdate = false;
	AdjustVerticalSize();
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	we 
\*------------------------------------------------------------------------------*/
void BmBodyPartView::RemoveModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())<<": removing one item from listview");
	inherited::RemoveModelItem( item);
	BmRef<BmDataModel> modelRef( DataModel());
	BmBodyPartList* bodyPartList 
		= dynamic_cast<BmBodyPartList*>( modelRef.Get());
	ShowBody( bodyPartList);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmString BmBodyPartView::StateInfoBasename()	{ 
	return "BodyPartView";
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::KeyDown(const char *bytes, int32 numBytes) { 
	if ( numBytes == 1 && mEditable) {
		switch( bytes[0]) {
			case B_DELETE: {
				BmRef<BmDataModel> modelRef( DataModel());
				BmBodyPartList* body 
					= dynamic_cast<BmBodyPartList*>( modelRef.Get());
				int32 idx;
				for( int32 i=0; (idx = CurrentSelection(i)) >= 0; ++i) {
					BmBodyPartItem* bodyItem 
						= dynamic_cast<BmBodyPartItem*>(ItemAt( idx));
					body->RemoveItemFromList( bodyItem->ModelItem());
				}				
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
	} else 
		inherited::KeyDown( bytes, numBytes);
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	if (!mEditable && Parent())
		Parent()->MakeFocus( true);
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
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_BODYPARTVIEW_SHOWALL: {
				mShowAllParts = true;
				fAvoidColPushing = false;
				BmRef<BmDataModel> modelRef( DataModel());
				ShowBody( dynamic_cast<BmBodyPartList*>( modelRef.Get()));
				break;
			}
			case BM_BODYPARTVIEW_SHOWATTACHMENTS: {
				mShowAllParts = false;
				fAvoidColPushing = true;
				BmRef<BmDataModel> modelRef( DataModel());
				ShowBody( dynamic_cast<BmBodyPartList*>( modelRef.Get()));
				break;
			}
			case BM_BODYPARTVIEW_SAVE_ATTACHMENT: {
				int32 index = CurrentSelection( 0);
				if (index < 0)
					break;
				BmBodyPartItem* bodyPartItem 
					= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
				if (!bodyPartItem)
					break;
				BmBodyPart* bodyPart( bodyPartItem->ModelItem());
				if (!bodyPart)
					break;
				// first step, let user select folder to save stuff into:
				if (!mSavePanel) {
					mSavePanel = new BFilePanel( 
						B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false
					);
				}
				mSavePanel->SetSaveText( bodyPart->FileName().String());
				mSavePanel->Show();
				break;
			}
			case BM_BODYPARTVIEW_SRC_CHARSET: {
				// change the source charset, i.e. the charset this
				// bodypart originally came from. This usually is UTF8 on BeOS,
				// but sometimes one has to handle other text-attachments, too.
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				int32 index = CurrentSelection( 0);
				if (index < 0 || !item)
					break;
				BmBodyPartItem* bodyPartItem 
					= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
				if (!bodyPartItem)
					break;
				BmBodyPart* bodyPart( bodyPartItem->ModelItem());
				if (!bodyPart)
					break;
				BmString charset(item->Label());
				BmStringIBuf srcBuf( bodyPart->DecodedData());
				BmString utf8Text;
				const uint32 blockSize 
					= max( (int32)128, bodyPart->DecodedLength());
				BmStringOBuf destBuf( blockSize);
				BmUtf8Encoder encoder( &srcBuf, charset, blockSize);
				destBuf.Write( &encoder, blockSize);
				utf8Text.Adopt( destBuf.TheString());
				if (encoder.HadError() || encoder.HadToDiscardChars()) {
					BAlert* alert = new BAlert( 
						"Wrong Source Charset", 
						"The selected source charset is probably incorrect, "
							"as some of the characters couldn't be converted to UTF8.",
						"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage('xxxx'), BMessenger( this)));
				} else {
					bodyPart->SetBodyText( utf8Text, charset);
					BmRef<BmDataModel> modelRef( DataModel());
					ShowBody( dynamic_cast<BmBodyPartList*>( modelRef.Get()));
				}
				break;
			}
			case BM_BODYPARTVIEW_DEST_CHARSET: {
				// change the destination charset, i.e. the charset this
				// bodypart will have in the mail:
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				int32 index = CurrentSelection( 0);
				if (index < 0 || !item)
					break;
				BmBodyPartItem* bodyPartItem 
					= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
				if (!bodyPartItem)
					break;
				BmBodyPart* bodyPart( bodyPartItem->ModelItem());
				if (!bodyPart)
					break;
				bodyPart->SuggestCharset( item->Label());
				BmRef<BmDataModel> modelRef( DataModel());
				ShowBody( dynamic_cast<BmBodyPartList*>( modelRef.Get()));
				break;
			}
			case BM_BODYPARTVIEW_DELETE_ATTACHMENT: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BMessage* newMsg = NULL;
					int32 index;
					for( int32 i=0; (index = CurrentSelection(i)) >= 0; ++i) {
						BmBodyPartItem* bodyPartItem 
							= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
						if (!bodyPartItem)
							break;
						BmBodyPart* bodyPart( bodyPartItem->ModelItem());
						if (!bodyPart)
							break;
						if (!newMsg)
							newMsg = new BMessage(BM_BODYPARTVIEW_DELETE_ATTACHMENT);
						newMsg->AddInt32( "sel_index", index);
					}
					BAlert* alert = new BAlert( 
						"Remove Mail-Attachment", 
						"Are you sure about removing the selected attachment(s)"
							" from the mail?",
						"Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( newMsg, BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					BmRef<BmDataModel> modelRef( DataModel());
					BmBodyPartList* bodyPartList 
						= dynamic_cast<BmBodyPartList*>( modelRef.Get());
					if (bodyPartList && bodyPartList->Mail() 
					&& buttonPressed == 0) {
						int32 index;
						for(
							int32 i=0; 
							msg->FindInt32( "sel_index", i, &index) == B_OK;
							++i
						) {
							if (index < 0)
								break;
							BmBodyPartItem* bodyPartItem 
								= dynamic_cast<BmBodyPartItem*>( 
									FullListItemAt( index)
								);
							if (!bodyPartItem)
								break;
							BmBodyPart* bodyPart( bodyPartItem->ModelItem());
							if (!bodyPart)
								break;
							bodyPartList->RemoveItemFromList( bodyPart);
							bodyPartList->Mail()->ConstructAndStore();
						}
					}
				}
				break;
			}
			case B_CANCEL: {
				// since a SavePanel seems to avoid quitting, thus stopping Beam 
				// from proper exit, we detroy the panel:
				delete mSavePanel;
				mSavePanel = NULL;
				break;
			}
			case B_SAVE_REQUESTED: {
				// since a SavePanel seems to avoid quitting, thus stopping Beam 
				// from proper exit, we detroy the panel:
				delete mSavePanel;
				mSavePanel = NULL;
				int32 index;
				for( int32 i=0; (index = CurrentSelection(i)) >= 0; ++i) {
					BmBodyPartItem* bodyPartItem 
						= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
					if (!bodyPartItem)
						break;
					BmRef<BmListModelItem> modelItemRef( );
					BmBodyPart* bodyPart( bodyPartItem->ModelItem());
					if (!bodyPart)
						break;
					entry_ref destDirRef;
					if (msg->FindRef( "directory", 0, &destDirRef) == B_OK) {
						BmString name = msg->FindString( "name");
						bodyPart->SaveAs( destDirRef, name);
					}
				}
				break;
			}
			case B_TRASH_TARGET:
			case B_COPY_TARGET: {
				if (msg->IsReply()) {
					const BMessage* dragMsg = msg->Previous();
					BmRef<BmDataModel> modelRef( DataModel());
					BmBodyPartList* bodyPartList 
						= dynamic_cast<BmBodyPartList*>( modelRef.Get());
					if (!dragMsg || !bodyPartList
					|| BmDragId!=dragMsg->FindString("be:originator"))
						break;
					int32 index;
					for( int32 i=0; (index = CurrentSelection(i)) >= 0; ++i) {
						BmBodyPartItem* bodyPartItem 
							= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
						BmBodyPart* bodyPart 
							= dynamic_cast<BmBodyPart*>( bodyPartItem->ModelItem());
						if (bodyPart) {
							if (msg->what == B_COPY_TARGET) {
								entry_ref dref;
								const char* name;
								if (msg->FindRef( "directory", &dref) != B_OK
								||	msg->FindString( "name", &name) != B_OK)
									return;
								BDirectory dir( &dref);
								BFile file( &dir, name, B_WRITE_ONLY | B_CREATE_FILE);
								if (file.InitCheck() == B_OK)
									bodyPart->WriteToFile( file);
							} else if (msg->what == B_TRASH_TARGET && mEditable) {
								bodyPartList->RemoveItemFromList( bodyPart);
							}
						}
					}
				}
				break;
			}
			case B_SIMPLE_DATA: {
				HandleDrop(msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("BodyPartView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::HandleDrop( BMessage* msg) {
	entry_ref ref;
	if (msg->FindRef( "refs", &ref) == B_OK)
		AddAttachment( msg);
	else {
		BPath tempPath;
		entry_ref tempPathRef;
		if (find_directory( B_COMMON_TEMP_DIRECTORY, &tempPath, true) == B_OK 
		&& get_ref_for_path( tempPath.Path(), &tempPathRef) == B_OK) {

			// build the reply message
			BMessage reply(B_COPY_TARGET);
			reply.AddInt32("be:actions", B_COPY_TARGET);
			reply.AddString("be:types", "application/octet-stream");
			reply.AddRef("directory", &tempPathRef);
			const char* filename;
			if (msg->FindString("be:clip_name", &filename) != B_OK)
				filename = "beam_dropped_file";
			reply.AddString("name", filename);
			BmString dropFileName = tempPath.Path();
			dropFileName << "/" << filename;
			{
				BFile file( dropFileName.String(), 
								B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
			}
	
			// Attach any data the originator may have tagged on
			BMessage data;
			if (msg->FindMessage("be:originator-data", &data) == B_OK)
				reply.AddMessage("be:originator-data", &data);
	
			// copy over all the file types the drag initiator claimed to
			// support
			for (int32 index = 0; ; index++) {
				const char *type;
				if (msg->FindString("be:filetypes", index, &type) != B_OK)
					break;
				reply.AddString("be:filetypes", type);
			}
	
			BMessage dataMsg;
			if (msg->SendReply(&reply, (BHandler*)NULL, 100000) == B_OK)
				AddAttachment(dropFileName.String());
		}
	}
	inherited::HandleDrop(msg);
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ItemInvoked( int32 index) {
	BmBodyPartItem* bodyPartItem 
		= dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
	if (bodyPartItem) {
		BmBodyPart* bodyPart( bodyPartItem->ModelItem());
		if (!bodyPart)
			return;
		bodyPartItem->Highlight( true);
		InvalidateItem( index);
		Window()->UpdateIfNeeded();		// for immediate highlight
		// we write the attachment into a temporary file...
		entry_ref eref = bodyPart->WriteToTempFile();
		// ...and verify that the real mimetype corresponds to the one indicated
		// by the attachment-info:
		BmString realMT = DetermineMimeType( &eref, true);
		if (realMT.ICompare( bodyPart->MimeType())!=0) {
			int32 choice = 0;
			if (bodyPart->MimeType().ICompare( "message/rfc822") == 0
			&& realMT.ICompare( "text/",5) == 0) {
				// skip complaining about emails being text/plain which comes
				// up quite regularly due to the limited success of the corresp.
				// BeOS-mimetype-sniffer-rules (they require a mail to start with 
				// "Return-Path: ", which is not always the case):
				choice = 1;		// revert to declared type
			} else {
				BmString s("ATTENTION!\n\n");
				s << "The attachment has been declared to be of type \n\n\t"
				  << bodyPart->MimeType()
				  << "\n\nbut BeOS thinks it is in fact of type\n\n\t" << realMT
				  << "\n\nWhat would you like Beam to do?";
				BmString openReal = BmString("Open as ")<<realMT;
				BmString openDeclared = BmString("Open as ")<<bodyPart->MimeType();
				BAlert* alert = new BAlert( "", s.String(), openReal.String(), 
													 openDeclared.String(), "Cancel");
				alert->SetShortcut( 2, B_ESCAPE);
				choice = alert->Go();
			}
			if (choice == 2)
				return;
			if (choice == 1) {
				// revert to declared mimetype:
				realMT = bodyPart->MimeType();
				BNode node( &eref);
				BNodeInfo nodeInfo( &node);
				status_t err = nodeInfo.SetType( realMT.String());
				if (err != B_OK)
					return;
			}
		}
		if (beamApp->HandlesMimetype( realMT)) {
			// take care of text/x-email and message/rfc822 ourselves:
			BMessage msg( B_REFS_RECEIVED);
			msg.AddRef( "refs", &eref);
			beamApp->RefsReceived( &msg);
		} else {
			bool doIt = true;
			if (BmBodyPart::MimeTypeIsPotentiallyHarmful( realMT)) {
				BmString s("The attachment may contain harmful content, "
							  "are you sure to open it?");
				BAlert* alert = new BAlert( "", s.String(), "Yes", "No");
				alert->SetShortcut( 1, B_ESCAPE);
				doIt = (alert->Go() == 0);
			}
			if (doIt) {
				BEntry entry( &eref);
				BPath path;
				entry.GetPath( &path);
				status_t res=entry.InitCheck();
				if (res==B_OK && path.InitCheck()==B_OK && path.Path()) {
					update_mime_info( path.Path(), false, true, false);
					// using BRoster::Launch(type, args, ...) seems to be more
					// reliable than Launch(eref), as for instance Firefox
					// doesn't work with the latter...
					char* urlStr = const_cast<char*>(path.Path());
					res = be_roster->Launch(realMT.String(), 1, &urlStr);
					if (res != B_OK && res != B_ALREADY_RUNNING) {
						ShowAlert( 
							BmString("Sorry, could not launch application for "
										"this attachment (unknown mimetype perhaps?)\n\n"
										"Error: ") << strerror(res)
						);
					}
				} else
					ShowAlert( 
						BmString("Sorry, could not open this attachment.\n\n"
									"Error: ") << strerror(res)
					);
			}
		}
		bodyPartItem->Highlight( false);
		InvalidateItem( index);
	}
}

/*------------------------------------------------------------------------------*\
	InitiateDrag()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartView::InitiateDrag( BPoint, int32 index, bool wasSelected) {
	if (!wasSelected)
		return false;
	BMessage dragMsg( B_SIMPLE_DATA);
	BmBodyPartItem* bodyPartItem = dynamic_cast<BmBodyPartItem*>(ItemAt( index));
	BmBodyPart* bodyPart( bodyPartItem->ModelItem());
	dragMsg.AddInt32( "be:actions", B_COPY_TARGET);
	dragMsg.AddInt32( "be:actions", B_TRASH_TARGET);
	dragMsg.AddString( "be:types", B_FILE_MIME_TYPE);
//	dragMsg.AddString( "be:filetypes", bodyPart->MimeType().String());
	dragMsg.AddString( "be:originator", BmDragId.String());
	dragMsg.AddString( "be:clip_name", bodyPart->FileName().String());
	dragMsg.AddPointer( "bm:bodypart", bodyPart);
	vector<int> cols;
	cols.push_back(1);
	cols.push_back(2);
	cols.push_back(3);
	BBitmap* dragImage = CreateDragImage(cols);
	DragMessage( &dragMsg, dragImage, B_OP_ALPHA, BPoint( 10.0, 10.0));
	return true;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "BodyPartViewMenu", false, false);
	BFont font( *be_plain_font);
	theMenu->SetFont( &font);

	BMenuItem* item; 
	if (!mEditable) {
		item = new BMenuItem( 
			"Show All MIME-Bodies", 
			new BMessage( mShowAllParts
								  ? BM_BODYPARTVIEW_SHOWATTACHMENTS
								  : BM_BODYPARTVIEW_SHOWALL)
		);
		item->SetTarget( this);
		item->SetMarked( mShowAllParts);
		theMenu->AddItem( item);
	
		theMenu->AddSeparatorItem();
		item = new BMenuItem( "Save Attachment As...", 
									 new BMessage( BM_BODYPARTVIEW_SAVE_ATTACHMENT));
		item->SetTarget( this);
		theMenu->AddItem( item);
	
		if (!mEditable) {
			theMenu->AddSeparatorItem();
			item = new BMenuItem( "Remove Attachment from Mail...", 
										 new BMessage( BM_BODYPARTVIEW_DELETE_ATTACHMENT));
			item->SetTarget( this);
			theMenu->AddItem( item);
		}
	} else {
		BMenu* menu = new BMenu( "Specify Source Charset");
		menu->SetFont( &font);
		BeamGuiRoster->AddCharsetMenu( menu, this, BM_BODYPARTVIEW_SRC_CHARSET);
		theMenu->AddItem( menu);
		theMenu->AddSeparatorItem();
		menu = new BMenu( "Select Destination Charset");
		menu->SetFont( &font);
		BeamGuiRoster->AddCharsetMenu( menu, this, BM_BODYPARTVIEW_DEST_CHARSET);
		theMenu->AddItem( menu);
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

