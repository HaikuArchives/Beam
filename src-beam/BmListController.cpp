/*
	BmListController.cpp
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


#include <memory.h>

#include <Autolock.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "CLVColumnLabelView.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBusyView.h"
#include "BmCaption.h"
#include "BmDataModel.h"
#include "BmListController.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmListViewItem
\********************************************************************************/


const char* const BmListViewItem::MSG_EXPANDED = 	"bm:expnd";
const char* const BmListViewItem::MSG_CHILDNAMES = "bm:chldnm";
const char* const BmListViewItem::MSG_CHILDREN = 	"bm:chldrn";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::BmListViewItem( const BmString& key, BmListModelItem* modelItem,
										  bool, BMessage* archive)
	:	inherited( 0, !modelItem->empty(), false, MAX( TheResources->FontLineHeight(), 18))
	,	mKey( key)
	,	mModelItem( modelItem)
{
	SetExpanded( archive ? archive->FindBool( MSG_EXPANDED) : false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::~BmListViewItem() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewItem::Archive( BMessage* archive, bool) const {
	// first we archive the item's attributes into a separate message...
	BMessage msg;
	msg.AddBool( MSG_EXPANDED, IsExpanded());

	// ...now we add this message with a corresponding key to the archive:
	status_t ret = archive->AddMessage( MSG_CHILDREN, &msg)
						|| archive->AddString( MSG_CHILDNAMES, Key().String());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::UpdateView( BmUpdFlags flags) {
	BmListModelItem* item = ModelItem();
	if (item && flags & (UPD_EXPANDER)) {
		SetSuperItem( item->size() != 0);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::SetTextCols( int16 firstTextCol, BmListColumn* columnVec) {
	int16 offs = firstTextCol;
	for( const BmListColumn* p = columnVec; p->text != NULL; ++p) {
		SetColumnContent( offs++, p->text, p->rightJustified);
	}
}

#ifdef BM_LOGGING
/*------------------------------------------------------------------------------*\
	ObjectSize()
		-	
\*------------------------------------------------------------------------------*/
int32 BmListViewItem::ObjectSize( bool addSizeofThis) const {
	return 	(addSizeofThis ? sizeof( *this) : 0)
		+		mKey.Length()+1;
}
#endif



/********************************************************************************\
	BmListViewController
\********************************************************************************/


const char* const BmListViewController::MSG_COLUMN_NO = 	"bm:colno";
const char* const BmListViewController::MSG_COLUMN_POS = "bm:colps";

const char* const BmListViewController::MSG_HIGHITEM = 	"hitem";
const char* const BmListViewController::MSG_EXPAND = 		"expnd";
const char* const BmListViewController::MSG_SCROLL_STEP= "step";

/*------------------------------------------------------------------------------*\
	BmListViewController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmListViewController::BmListViewController( minimax minmax, BRect rect,
								 const char* Name, list_view_type Type, bool hierarchical, 
								 bool showLabelView, bool showCaption, bool showBusyView)
	:	inherited( minmax, rect, Name, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, 
					  Type, hierarchical, showLabelView)
	,	inheritedController( Name)
	,	mInitialStateInfo( NULL)
	,	mShowCaption( showCaption)
	,	mShowBusyView( showBusyView)
	,	mUseStateCache( true)
	,	mCurrHighlightItem( NULL)
	,	mSittingOnExpander( false)
	,	mExpandCollapseRunner( NULL)
	,	mPulsedScrollRunner( NULL)
	,	mPulsedScrollStep( 0)
	,	mDragBetweenItems( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmListViewController()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmListViewController::~BmListViewController() {
	BList tempList;
		int32 count = FullListCountItems();
		for( int i=0; i<count; ++i)
			tempList.AddItem( FullListItemAt( i));
		while( !tempList.IsEmpty()) {
			BmListViewItem* subItem = static_cast<BmListViewItem*>(tempList.RemoveItem( (int32)0));
			delete subItem;
		}
	delete mExpandCollapseRunner;
	delete mPulsedScrollRunner;
	delete mInitialStateInfo;
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmListViewController::CreateContainer( bool horizontal, bool vertical, 
												  							bool scroll_view_corner, 
												  							border_style border, 
																		   uint32 ResizingMode, 
																		   uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView);
}

/*------------------------------------------------------------------------------*\
	AttachedToWindow()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AttachedToWindow() {
	inherited::AttachedToWindow();
	if (SetTarget( this) != B_OK)
		BM_THROW_RUNTIME( "ListViewController: Could not set invocation-target.");
	SetInvocationMessage( new BMessage( B_CONTROL_INVOKED));
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	override that automatically grabs the keyboard-focus after a click 
			inside the listview
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseDown(BPoint point) { 
	inherited::MouseDown( point); 
	// BView::MakeFocus( true);
	// [zooey]: use the following line to draw blue frame indicating key-focus
	MakeFocus( true);
}

/*------------------------------------------------------------------------------*\
	MouseUp( point)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseUp(BPoint point) { 
	inherited::MouseUp( point); 
}

/*------------------------------------------------------------------------------*\
	MouseMoved()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseMoved( BPoint point, uint32 transit, const BMessage *msg) {
	inherited::MouseMoved( point, transit, msg);
	if (msg && AcceptsDropOf( msg)) {
		if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
			const int32 scrollZone = 12;
			int32 scrollStep = 0;
			BRect b( Bounds());
			if (point.y > b.bottom-scrollZone)
				scrollStep = 1;
			else if (point.y < b.top+scrollZone)
				scrollStep = -1;
			if (scrollStep != mPulsedScrollStep) {
				if (mPulsedScrollRunner) {
					delete mPulsedScrollRunner;
					mPulsedScrollRunner = NULL;
				}
				mPulsedScrollStep = scrollStep;
				if (mPulsedScrollStep) {
					int32 pulsedScrollDelay = ThePrefs->GetInt( "PulsedScrollDelay", 100);
					if (pulsedScrollDelay>0) {
						BMessage* msg = new BMessage( BM_PULSED_SCROLL);
						msg->AddInt32( MSG_SCROLL_STEP, mPulsedScrollStep);
						BMessenger msgr( this);
						mPulsedScrollRunner 
							= new BMessageRunner( msgr, msg, pulsedScrollDelay*1000, -1);
					}
				}
			}
			HighlightItemAt( point);
			if (Hierarchical() && mCurrHighlightItem 
			&& mCurrHighlightItem->ExpanderRectContains( point)) {
				if (!mSittingOnExpander) {
					if (!mCurrHighlightItem->IsExpanded()) {
						// expand superitem if mouse is over expander (so that user can 
						// navigate through the subitems that were previously hidden):
						Expand( mCurrHighlightItem);
					} else {
						// collapse superitem if mouse is over expander (so that user can 
						// hide subitems that were previously shown):
						Collapse( mCurrHighlightItem);
					}
					mSittingOnExpander = true;
				}
			} else 
				mSittingOnExpander = false;
		} else if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW) {
			if (mCurrHighlightItem) {
				mCurrHighlightItem->Highlight( false);
				mCurrHighlightItem->HighlightTop( false);
				mCurrHighlightItem->HighlightBottom( false);
				InvalidateItem( IndexOf( mCurrHighlightItem));
				mCurrHighlightItem = NULL;
			}
			if (mPulsedScrollRunner) {
				delete mPulsedScrollRunner;
				mPulsedScrollRunner = NULL;
				mPulsedScrollStep = 0;
			}
		}
	} else {
		if (mPulsedScrollRunner) {
			delete mPulsedScrollRunner;
			mPulsedScrollRunner = NULL;
			mPulsedScrollStep = 0;
		}
	}
}

/*------------------------------------------------------------------------------*\
	HighligtItemAt( point)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::HighlightItemAt( const BPoint& point) {
	int32 index;
	bool needToHighlightTop = false;
	bool forceUpdate = false;
	if (mDragBetweenItems) {
		BRect firstItemRect = ItemFrame( 0);
		BPoint highPt( point.x, point.y-firstItemRect.Height()/2.0);
		index = IndexOf( highPt);
		if (index < 0) {
			if (highPt.y < firstItemRect.top && point.y >= firstItemRect.top) {
				index = 0;
				needToHighlightTop = true;
			}
		}
		if (index == 0)
			forceUpdate = true;
	} else
		index = IndexOf( point);
	if (forceUpdate || IndexOf( mCurrHighlightItem) != index) {
		if (mCurrHighlightItem) {
			// remove old highlight:
			mCurrHighlightItem->Highlight( false);
			mCurrHighlightItem->HighlightTop( false);
			mCurrHighlightItem->HighlightBottom( false);
			InvalidateItem( IndexOf( mCurrHighlightItem));
		}
		if (index >= 0) {
			// highlight current destination:
			mCurrHighlightItem = dynamic_cast<BmListViewItem*>( ItemAt( index));
			if (mDragBetweenItems) {
				mCurrHighlightItem->HighlightTop( needToHighlightTop);
				mCurrHighlightItem->HighlightBottom( !needToHighlightTop);
			} else
				mCurrHighlightItem->Highlight( true);
			InvalidateItem( index);
			if (Hierarchical() && mCurrHighlightItem->IsSuperItem()) {
				if (mExpandCollapseRunner) {
					delete mExpandCollapseRunner;
					mExpandCollapseRunner = NULL;
				}
				int32 expandCollapseDelay = ThePrefs->GetInt( "ExpandCollapseDelay", 1000);
				if (expandCollapseDelay>0) {
					BMessage* msg = new BMessage( BM_EXPAND_OR_COLLAPSE);
					msg->AddPointer( MSG_HIGHITEM, (void*)mCurrHighlightItem);
					msg->AddBool( MSG_EXPAND, !mCurrHighlightItem->IsExpanded());
					BMessenger msgr( this);
					mExpandCollapseRunner 
						= new BMessageRunner( msgr, msg, expandCollapseDelay*1000, 1);
				}
			}
		} else
			mCurrHighlightItem = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::HandleDrop( const BMessage*) {
	// remove the drag-highlight, if neccessary:
	if (mCurrHighlightItem) {
		mCurrHighlightItem->Highlight( false);
		mCurrHighlightItem->HighlightTop( false);
		mCurrHighlightItem->HighlightBottom( false);
		InvalidateItem( IndexOf( mCurrHighlightItem));
		mCurrHighlightItem = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MessageReceived( BMessage* msg) {
	try {
		BmRef<BmDataModel> dataModel( DataModel());
		switch( msg->what) {
			case BM_LISTMODEL_ADD: {
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				if (IsMsgFromCurrentModel( msg)) {
					AddModelItem( item);
					UpdateCaption();
				}
				item->RemoveRef();			// the msg is no longer referencing the item
				break;
			}
			case BM_LISTMODEL_REMOVE: {
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				if (IsMsgFromCurrentModel( msg)) {
					RemoveModelItem( item);
					UpdateCaption();
				}
				item->RemoveRef();			// the msg is no longer referencing the item
				break;
			}
			case BM_LISTMODEL_UPDATE: {
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				if (IsMsgFromCurrentModel( msg)) {
					BmUpdFlags flags = UPD_ALL;
					msg->FindInt32( BmListModel::MSG_UPD_FLAGS, (int32*)&flags);
					UpdateModelItem( item, flags);
				}
				item->RemoveRef();			// the msg is no longer referencing the item
				break;
			}
			case BM_LISTVIEW_SHOW_COLUMN:
			case BM_LISTVIEW_HIDE_COLUMN: {
				ShowOrHideColumn( msg);
				break;
			}
			case BM_JOB_UPDATE_STATE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				UpdateModelState( msg);
				break;
			}
			case BM_JOB_DONE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			case BMM_SET_BUSY: {
				ScrollView()->SetBusy();
				break;
			}
			case BMM_UNSET_BUSY: {
				ScrollView()->UnsetBusy();
				break;
			}
			case B_CONTROL_INVOKED: {
				// an item has been double-clicked:
				ItemInvoked( FindMsgInt32( msg, "index"));
				break;
			}
			case BM_EXPAND_OR_COLLAPSE: {
				BmListViewItem* item = NULL;
				msg->FindPointer( MSG_HIGHITEM, (void**)&item); 
				if (item == mCurrHighlightItem) {
					bool expand = msg->FindBool( MSG_EXPAND);
					if (expand) {
						// expand superitem (so that user can 
						// navigate through the subitems that were previously hidden):
						Expand( mCurrHighlightItem);
					} else {
						// collapse superitem (so that user can 
						// hide subitems that were previously shown):
						Collapse( mCurrHighlightItem);
					}
				}
				delete mExpandCollapseRunner;
				mExpandCollapseRunner = NULL;
				break;
			}
			case BM_PULSED_SCROLL: {
				int32 step;
				if (msg->FindInt32( MSG_SCROLL_STEP, &step) == B_OK) {
					BRect bounds = Bounds();
					if (step>0) {
						int32 lastVisibleIndex = IndexOf( BPoint( 0, bounds.bottom-5));
						if (lastVisibleIndex>=0 && lastVisibleIndex+step < CountItems()) {
							BRect frame = ItemFrame( lastVisibleIndex+step);
							float newYPos = frame.bottom-Bounds().Height();
							ScrollTo( BPoint( 0, MAX( 0, newYPos)));
						}
					} else if (step<0) {
						int32 firstVisibleIndex = IndexOf( BPoint( 0, bounds.top+5));
						if (firstVisibleIndex+step >= 0) {
							BRect frame = ItemFrame( firstVisibleIndex+step);
							float newYPos = frame.top;
							ScrollTo( BPoint( 0, MAX( 0, newYPos)));
						}
					}
					BPoint point;
					uint32 buttons;
					GetMouse( &point, &buttons);
					HighlightItemAt( point);
				}
				break;
			}
			default:
				if (msg->WasDropped())
					HandleDrop( msg);
				else
					inherited::MessageReceived( msg);
		}
		if (MsgNeedsAck( msg) && dataModel)
			dataModel->ControllerAck( this);
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString(ControllerName()) << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	FindViewItemFor( modelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::FindViewItemFor( BmListModelItem* modelItem) const {
	BmViewModelMap::const_iterator iter = mViewModelMap.find( modelItem);
	if (iter == mViewModelMap.end())
		return NULL;
	else
		return iter->second;
}

/*------------------------------------------------------------------------------*\
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AddAllModelItems() {
	BM_LOG2( BM_LogModelController, BmString(ControllerName())<<": adding items to listview");
	BmAutolockCheckGlobal lock( DataModel()->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			BmString() << ControllerName() 
				<< ":AddAllModelItems(): Unable to lock model"
		);
	MakeEmpty();
	BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
	BM_ASSERT( model);
	BList* tempList = NULL;
	if (!Hierarchical())
		tempList = new BList( model->size());
	SetDisconnectScrollView( true);
	SetInsertAtSortedPos( false);
	BmModelItemMap::const_iterator iter;
	BmModelItemMap::const_iterator endIter = model->end();
	int32 count=1;
	for( iter = model->begin(); iter != endIter; ++iter, ++count) {
		BmListModelItem* modelItem = iter->second.Get();
		BmListViewItem* viewItem;
		if (Hierarchical()) {
			// add item and its subitems to the listview:
			doAddModelItem( NULL, modelItem);
		} else {
			viewItem = CreateListViewItem( modelItem);
			if (viewItem)
				tempList->AddItem( viewItem);
			mViewModelMap[modelItem] = viewItem;
			BM_LOG2( BM_LogMailTracking, BmString("ListView <") << ModelName() << "> added view-item " << viewItem->Key());
		}
		if (count%100==0) {
			ScrollView()->PulseBusyView();
			BmString caption = BmString()<<count<<" "<<ItemNameForCaption()<<(count>1?"s":"");
			UpdateCaption( caption.String());
		}
	}
	if (!Hierarchical()) {
		// add complete item-list for efficiency:
#ifdef BM_LOGGING_MEM
		int32 objSize = 0;
		for( int32 i=0; i<tempList->CountItems(); ++i) {
			BmListViewItem* item = (BmListViewItem*)tempList->ItemAt(i);
			objSize += item->ObjectSize() + sizeof( BmString) + item->Key().Length();
		}
		BM_LOG( BM_LogMailTracking, BmString("ListView <") << ModelName() << "> has (estimated) size of " << objSize << " bytes");
#endif
		AddList( tempList);
		delete tempList;
	}

	SortItems();
	SetInsertAtSortedPos( true);
	SetDisconnectScrollView( false);
	UpdateColumnSizesDataRectSizeScrollBars( true);
	UpdateCaption();
	BM_LOG3( BM_LogModelController, BmString(ControllerName())<<": finished with adding items to listview");
}

/*------------------------------------------------------------------------------*\
	AddModelItem( item)
		-	Hook function that is called whenever a new item has been added to the 
			listmodel
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::AddModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, BmString(ControllerName())<<": adding one item to listview");
	BmListViewItem* newItem;
	if (!Hierarchical()) {
		newItem = doAddModelItem( NULL, item);
	} else {
		BmRef<BmListModelItem> parent( item->Parent());
		BmListViewItem* parentItem = FindViewItemFor( parent.Get());
		newItem = doAddModelItem( parentItem, item);
	}
	BMessage msg( BM_NTFY_LISTCONTROLLER_MODIFIED);
	SendNotices( BM_NTFY_LISTCONTROLLER_MODIFIED, &msg);
	return newItem;
}

/*------------------------------------------------------------------------------*\
	doAddModelItem( BmListViewItem* parent, BmListModelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::doAddModelItem( BmListViewItem* parent, BmListModelItem* item) {
	auto_ptr<BMessage> archive( Hierarchical() ? GetArchiveForItemKey( item->Key()) : NULL);
	BmListViewItem* newItem = CreateListViewItem( item, archive.get());
	if (newItem) {
		if (Hierarchical() && !item->empty())
			newItem->SetSuperItem( true);
		if (parent)
			AddUnder( newItem, parent);
		else
			AddItem( newItem);
		mViewModelMap[item] = newItem;
		BM_LOG2( BM_LogMailTracking, BmString("ListView <") << ModelName() << "> added view-item " << newItem->Key());
	}
	
	// add all sub-items of current item to the view as well:
	BmModelItemMap::const_iterator iter;
	BmModelItemMap::const_iterator endIter = item->end();
	for( iter = item->begin(); iter != endIter; ++iter) {
		BmListModelItem* subItem = iter->second.Get();
		doAddModelItem( newItem, subItem);
	}
	return newItem;
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	Hook function that is called whenever an item has been deleted from the
			listmodel
\*------------------------------------------------------------------------------*/
void BmListViewController::RemoveModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, BmString(ControllerName())<<": removing one item from listview");
	if (item) {
		doRemoveModelItem( item);
		UpdateCaption();
		BMessage msg( BM_NTFY_LISTCONTROLLER_MODIFIED);
		SendNotices( BM_NTFY_LISTCONTROLLER_MODIFIED, &msg);
	}
}

/*------------------------------------------------------------------------------*\
	doRemoveModelItem( msg)
		-
\*------------------------------------------------------------------------------*/
void BmListViewController::doRemoveModelItem( BmListModelItem* item) {
	BmListViewItem* viewItem = NULL;
	if (item) {
		BmViewModelMap::iterator pos = mViewModelMap.find( item);
		if (pos != mViewModelMap.end()) {
			viewItem = pos->second;
			mViewModelMap.erase( pos);
		}
		if (viewItem) {
			RemoveItem( viewItem);
			BM_LOG2( BM_LogMailTracking, BmString("ListView <") << ModelName() << "> removed view-item " << viewItem->Key());
			// remove all sub-items of current item from the view as well:
			BmModelItemMap::const_iterator iter;
			BmModelItemMap::const_iterator endIter = item->end();
			for( iter = item->begin(); iter != endIter; ++iter) {
				BmListModelItem* subItem = iter->second.Get();
				doRemoveModelItem( subItem);
			}
			delete viewItem;
		}
	}
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( msg)
		-	Hook function that is called whenever an item needs to be updated 
		-	default implementation redraws item completely
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::UpdateModelItem( BmListModelItem* item, 
																		 BmUpdFlags updFlags) {
	BmListViewItem* viewItem = FindViewItemFor( item);
	if (viewItem) {
		viewItem->UpdateView( updFlags);
		int32 idx = IndexOf(viewItem);
		if (idx >= 0)
			InvalidateItem( idx);
	} else
		BM_LOG2( BM_LogModelController, BmString(ControllerName())<<": requested to update an unknown item <"<<item->Key()<<">");
	if (updFlags & UPD_SORT) {
		SetDisconnectScrollView( true);
		SortItems();
		SetDisconnectScrollView( false);
	}
	return viewItem;
}

/*------------------------------------------------------------------------------*\
	UpdateModelState( msg)
		-	Hook function that is called whenever the model as a whole (the list)
			has changed state
		-	default implementation does nothing
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateModelState( BMessage*) {
}

/*------------------------------------------------------------------------------*\
	UpdateCaption()
		-
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateCaption( const char* text) {
	if (mShowCaption && fScrollView) {
		if (text) {
			ScrollView()->SetCaptionText( text);
		} else {
			int32 numItems = FullListCountItems();
			BmString caption;
			if (!numItems)
				caption = BmString("no ")<<ItemNameForCaption()<<"s";
			else
				caption = BmString("")<<numItems<<" "<<ItemNameForCaption()<<(numItems>1 ? "s" : "");
			ScrollView()->SetCaptionText( caption.String());
		}
		Window()->UpdateIfNeeded();
	}
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ItemInvoked( int32) {
}

/*------------------------------------------------------------------------------*\
	ShowOrHideColumn( msg)
		-	Hook function that is called whenever the user asked to show or hide a
			specific listview-column
\*------------------------------------------------------------------------------*/
void BmListViewController::ShowOrHideColumn( BMessage* msg) {
	if (msg->what == BM_LISTVIEW_SHOW_COLUMN) {
		ShowColumn( msg->FindInt32( MSG_COLUMN_NO));
	} else {
		HideColumn( msg->FindInt32( MSG_COLUMN_NO));
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ShowLabelViewMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "LabelViewMenu", false, false);

	int32 numCols = fColumnList.CountItems();
	int32 numVisibleCols = fColumnDisplayList.CountItems();
	for( int32 i=0; i<numCols; ++i) {
		CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( i);
		if (!column) continue;
		uint32 flags = column->Flags();
		if (flags & (CLV_EXPANDER | CLV_LOCK_WITH_RIGHT | CLV_MERGE_WITH_RIGHT | CLV_NOT_MOVABLE)) continue;
		bool shown = fColumnDisplayList.HasItem( column);
		BMessage* msg = new BMessage( shown ? BM_LISTVIEW_HIDE_COLUMN : BM_LISTVIEW_SHOW_COLUMN);
		msg->AddInt32( MSG_COLUMN_NO, i);
		BmString label = column->GetLabelName();
		BMenuItem* item = new BMenuItem( label.String(), msg);
		item->SetMarked( shown);
		if (shown && numVisibleCols == 1) {
			// don't allow user to remove last visible column:
			item->SetEnabled( false);
		}
		item->SetTarget( this);
		theMenu->AddItem( item);
	}

	if (theMenu->CountItems() > 0) {
	   ColumnLabelView()->ConvertToScreen(&point);
		BRect openRect;
		openRect.top = point.y - 5;
		openRect.bottom = point.y + 5;
		openRect.left = point.x - 5;
		openRect.right = point.x + 5;
		theMenu->SetAsyncAutoDestruct( true);
  		theMenu->Go( point, true, false, openRect, true);
	} else
	  	delete theMenu;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ExpansionChanged( CLVListItem* _item, bool) {
	BmListViewItem* item = dynamic_cast< BmListViewItem*>( _item);
	UpdateItem( item, UPD_EXPANDER);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateItem( BmListViewItem* item, BmUpdFlags flags) {
	if (!item)
		return;
	if (LockLooper()) {
		item->UpdateView( flags);
		InvalidateItem( IndexOf( item));
		UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	AttachModel()
		-	reads appropriate state-info after attaching to data-model
\*------------------------------------------------------------------------------*/
void BmListViewController::AttachModel( BmDataModel* model) {
	inheritedController::AttachModel( model);
	ReadStateInfo();
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	writes state-info for current data-model before detaching
		-	clears listview after detach
\*------------------------------------------------------------------------------*/
void BmListViewController::DetachModel() {
	WriteStateInfo();
	inheritedController::DetachModel();
	ScrollView()->UnsetBusy();
	MakeEmpty();
	UpdateCaption();
}

/*------------------------------------------------------------------------------*\
	MakeEmpty()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MakeEmpty() {
	BList tempList;
	if (LockLooper()) {
		int32 count = FullListCountItems();
		for( int i=0; i<count; ++i)
			tempList.AddItem( FullListItemAt( i));
		inherited::MakeEmpty();					// clear display
		int c;
		while( !tempList.IsEmpty()) {
			BmListViewItem* subItem = static_cast<BmListViewItem*>(tempList.RemoveItem( (int32)0));
			if ((c = mViewModelMap.erase( subItem->ModelItem())) != 1)
				BM_LOGERR( BmString("unable to erase subItem ") << subItem->Key() << "from ViewModelMap.\nResult of erase() is: "<<c);
			delete subItem;
		}
		UpdateCaption();
		UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::StartJob( BmJobModel* model, bool startInNewThread,
											    int32 jobSpecifier) {
	AttachModel( model);
	ScrollView()->SetBusy();
	UpdateCaption( "tracking...");
	inheritedController::StartJob( model, startInNewThread, jobSpecifier);
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	Hook function that is called whenever the jobmodel associated to this 
			controller indicates that it is done (meaning: the list has been fetched
			and is now ready to be displayed).
\*------------------------------------------------------------------------------*/
void BmListViewController::JobIsDone( bool completed) {
	if (completed) {
		AddAllModelItems();
	} else {
		UpdateCaption( "");
	}
	ScrollView()->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::WriteStateInfo() {
	status_t err;
	BmString stateInfoFilename;
	BFile stateInfoFile;
	BMessage archive;
	
	if (!DataModel() || !mUseStateCache)
		return;

	try {
		stateInfoFilename = StateInfoBasename() << "_" << ModelName();
		if (this->Archive( &archive, Hierarchical()) != B_OK)
			BM_THROW_RUNTIME( BmString("Unable to archive State-Info for ")
										<< Name());
		if ((err = stateInfoFile.SetTo( 
			TheResources->StateInfoFolder(), 
			stateInfoFilename.String(), 
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE
		)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not create state-info file\n\t<") 
										<< stateInfoFilename << ">\n\n Result: " 
										<< strerror(err)
			);
		if ((err = archive.Flatten( &stateInfoFile)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not store state-info into file\n\t<") 
										<< stateInfoFilename << ">\n\n Result: " 
										<< strerror(err)
			);
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	Archive()
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewController::Archive(BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (deep && ret == B_OK) {
		int32 numItems = FullListCountItems();
		for( int32 i=0; i<numItems && ret==B_OK; ++i) {
			BmListViewItem* item = static_cast< BmListViewItem*>( FullListItemAt( i));
			ret = item->Archive( archive, deep);
		}
	}
	if (ret != B_OK) {
		BM_SHOWERR( BmString("Could not archive State-Info for ") << ModelName() << "\n\tError: "<< strerror( ret));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetArchiveForItemKey( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmListViewController::GetArchiveForItemKey( const BmString& key, BMessage* msg) {
	if (mInitialStateInfo) {
		status_t ret = B_OK;
		for( int i=0; ret==B_OK; ++i) {
			const char* name;
			ret = mInitialStateInfo->FindString( BmListViewItem::MSG_CHILDNAMES, i, &name);
			if (key == name) {
				return FindMsgMsg( mInitialStateInfo, BmListViewItem::MSG_CHILDREN, msg, i);
			}
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	ReadStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ReadStateInfo() {
	status_t err;
	BmString stateInfoFilename;
	BFile stateInfoFile;

	// try to open state-info-file...
	stateInfoFilename = StateInfoBasename() << "_" << ModelName();
	if (mUseStateCache
	&& (err = stateInfoFile.SetTo( TheResources->StateInfoFolder(), 
											 stateInfoFilename.String(), B_READ_ONLY)) != B_OK) {
		// state-file not found, but we have changed names of statefiles in Feb 2003,
		// so we check if a state-file according to the old name exists (and rename
		// it to match our new regulations):
		Regexx rx( stateInfoFilename, "^(.+?_.+?_\\d+)_\\d+(.+?)$", "$1$2");
		BmString oldStateInfoFilename = rx;
		BEntry entry( TheResources->StateInfoFolder(), oldStateInfoFilename.String());
		err = (entry.InitCheck() || entry.Rename( stateInfoFilename.String()));
	}
	if (mUseStateCache
	&& (err = stateInfoFile.SetTo( TheResources->StateInfoFolder(), 
											 stateInfoFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, file found, we fetch our state(s) from it:
		try {
			delete mInitialStateInfo;
			mInitialStateInfo = new BMessage;
			if ((err = mInitialStateInfo->Unflatten( &stateInfoFile)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not fetch state-info from file\n\t<") 
						<< stateInfoFilename << ">\n\n Result: " << strerror(err)
				);
			if (mInitialStateInfo)
				Unarchive( mInitialStateInfo);
		} catch (BM_error &e) {
			delete mInitialStateInfo;
			mInitialStateInfo = NULL;
			BM_SHOWERR( e.what());
		}
		return;
	}
	delete mInitialStateInfo;
	mInitialStateInfo = DefaultLayout();
	if (mInitialStateInfo)
		Unarchive( mInitialStateInfo);
}



/********************************************************************************\
	BmCLVContainerView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCLVContainerView::BmCLVContainerView( minimax minmax, ColumnListView* target, 
													 uint32 resizingMode, uint32 flags, 
													 bool horizontal, bool vertical,
													 bool scroll_view_corner, border_style border, 
													 bool showCaption, bool showBusyView,
													 float captionWidth)
	:	inherited( minmax, target, resizingMode, flags, horizontal, vertical,
					  scroll_view_corner, border)
	,	mCaption( NULL)
	,	mCaptionWidth( captionWidth)
	,	mBusyView( NULL)
{
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR));
	BRect frame;
	BPoint LT;
	BScrollBar* hScroller = horizontal ? ScrollBar( B_HORIZONTAL) : NULL;
	if (hScroller) {
		frame = hScroller->Frame();
		if (showCaption && !mCaptionWidth) {
//			mCaptionWidth = frame.Width();
			hScroller->Hide();
		}
	} else {
		frame = Bounds();
		frame.left += 2;
		frame.right -= 2;
		frame.top = frame.bottom - 1 - B_H_SCROLL_BAR_HEIGHT;
	}
	if (showBusyView) {
		LT = frame.LeftTop();
		float bvSize = frame.Height();
		if (hScroller) {
			// a horizontal scrollbar exists, we shrink it to make room for the busyview:
			hScroller->ResizeBy( -bvSize, 0.0);
			hScroller->MoveBy( bvSize, 0.0);
		}
		mBusyView = new BmBusyView( BRect( LT.x, LT.y, LT.x+bvSize, LT.y+bvSize));
		AddChild( mBusyView);
		frame.left += bvSize;
	}
	if (showCaption) {
		LT = frame.LeftTop();
		if (hScroller) {
			// a horizontal scrollbar exists, we shrink it to make room for the caption:
			hScroller->ResizeBy( -mCaptionWidth, 0.0);
			hScroller->MoveBy( mCaptionWidth, 0.0);
		} else {
			// no horizontal scrollbar, so the caption occupies all the remaining space:
			mCaptionWidth = frame.Width();
		}
		mCaption = new BmCaption( BRect( LT.x, LT.y, LT.x+mCaptionWidth, LT.y+frame.Height()), "");
		AddChild( mCaption);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCLVContainerView::~BmCLVContainerView() {
}
	
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::SetCaptionText( const char* text) {
	if (mCaption)
		mCaption->SetText( text);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::SetBusy() {
	if (mBusyView) mBusyView->SetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::UnsetBusy() {
	if (mBusyView) mBusyView->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::PulseBusyView() {
	if (mBusyView) mBusyView->Pulse();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmCLVContainerView::layout( BRect rect) {
	BRect r = inherited::layout( rect);
	float fullCaptionWidth = r.Width();
	fullCaptionWidth -= 2.0;
	if (mBusyView) {
		BRect bvFrame = mBusyView->Frame();
		mBusyView->MoveTo( bvFrame.left, rect.bottom-1-bvFrame.Height());
		fullCaptionWidth -= bvFrame.Width();
	}
	if (mCaption) {
		BRect cpFrame = mCaption->Frame();
		mCaption->MoveTo( cpFrame.left, rect.bottom-1-cpFrame.Height());
		BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
		if (!mCaptionWidth && (!hScroller || hScroller->IsHidden())) {
			if (ScrollBar( B_VERTICAL))
				fullCaptionWidth -= B_V_SCROLL_BAR_WIDTH + 1;
			mCaption->ResizeTo( fullCaptionWidth, cpFrame.Height());
			mCaption->Invalidate();
		}
	}
	return r;
}
