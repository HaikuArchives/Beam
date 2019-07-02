/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <memory.h>

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <StringView.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "CLVColumnLabelView.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmBusyView.h"
#include "BmCaption.h"
#include "BmDataModel.h"
#include "BmListController.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmViewItemFilter
\********************************************************************************/

BmViewItemFilter::BmViewItemFilter()
{
}

BmViewItemFilter::~BmViewItemFilter()
{
}



/********************************************************************************\
	BmListViewItem
\********************************************************************************/

enum Columns {
	COL_EXPANDER = 0
};

const char* const BmListViewItem::MSG_EXPANDED = 	"bm:expnd";
const char* const BmListViewItem::MSG_CHILDNAMES = "bm:chldnm";
const char* const BmListViewItem::MSG_CHILDREN = 	"bm:chldrn";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::BmListViewItem( ColumnListView* lv, BmListModelItem* modelItem,
										  bool, BMessage* archive)
	:	inherited( modelItem->OutlineLevel(), !modelItem->empty(), false, lv)
	,	mModelItem( modelItem)
	,	mShouldBeHidden( false)
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
void BmListViewItem::UpdateView( BmUpdFlags flags, bool redraw, 
											uint32 updColBitmap) {
	if (flags & UPD_EXPANDER) {
		BmListModelItem* item = ModelItem();
		if (item)
			SetSuperItem( item->size() != 0);
		updColBitmap |= (1UL<<COL_EXPANDER);
	}
	BRegion updateRegion;
	bool needReSort = false;
	uint32 colCount = fOwner->CountColumns();
	for( uint32 c=0; c<colCount; ++c) {
		if (updColBitmap & (1UL<<c)) {
			if (fOwner->IsSortKey( c))
				needReSort = true;
			if (redraw)
				updateRegion.Include( ItemColumnFrame( c));
		}
	}
	if (needReSort)
		fOwner->ReSortItem( this);
	if (redraw) {
		BRect updateRect = updateRegion.Frame();
		if (updateRect.IsValid())
			fOwner->Invalidate( updateRect);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::SetTextCols( int16 firstTextCol, const char** content) {
	int16 offs = firstTextCol;
	for( ; *content != NULL; ++content) {
		SetColumnContent( offs++, *content);
	}
}



/********************************************************************************\
	BmViewItemManager
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmViewItemManager::BmViewItemManager()
	:	mLocker("ViewItemManagerLock")
	,	mFilter(NULL)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmViewItemManager::~BmViewItemManager()
{
	delete mFilter;
	MakeEmpty();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmViewItemManager::Add(const BmListModelItem* modelItem,
									 BmListViewItem* viewItem)
{
	if (!viewItem)
		return;
	BAutolock lock(mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME("BmViewItemManager::Add(): Unable to get lock");
	mViewModelMap[modelItem] = viewItem;
	if (mFilter && !mFilter->Matches(viewItem))
		viewItem->ShouldBeHidden(true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmViewItemManager::Remove(const BmListModelItem* modelItem)
{
	BmListViewItem* viewItem = NULL;
	if (modelItem) {
		BAutolock lock(mLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME("BmViewItemManager::Remove(): Unable to get lock");
		BmViewModelMap::iterator pos = mViewModelMap.find(modelItem);
		if (pos != mViewModelMap.end()) {
			viewItem = pos->second;
			mViewModelMap.erase(pos);
		}
	}
	return viewItem;
}
	
/*------------------------------------------------------------------------------*\
	MakeEmpty()
		-	
\*------------------------------------------------------------------------------*/
void BmViewItemManager::MakeEmpty()
{
	BAutolock lock(mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME("BmViewItemManager::MakeEmpty(): Unable to get lock");

	BmViewModelMap::iterator iter;
	for(iter = mViewModelMap.begin(); iter != mViewModelMap.end(); ++iter)
		delete iter->second;
	mViewModelMap.clear();
}

/*------------------------------------------------------------------------------*\
	FindViewItemFor( modelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* 
BmViewItemManager::FindViewItemFor(const BmListModelItem* modelItem) const
{
	BAutolock lock(mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME(
			"BmViewItemManager::FindViewItemFor(): Unable to get lock"
		);

	BmViewModelMap::const_iterator iter = mViewModelMap.find(modelItem);
	if (iter == mViewModelMap.end())
		return NULL;
	else
		return iter->second;
}

/*------------------------------------------------------------------------------*\
	ApplyFilter(filter, continueCallback)
		-	sets new filter (which may be NULL) and applies it to all view items
		-	this object takes possession of the given filter and will delete it
			whenever a new filter is set or the object is destroyed
		-	the given callback will be checked to determine if the process 
			should be stopped
		-	returns true if filter has been applied to all view items
			and false if the process was stopped inbetween or an error occurred
		-	since this method may take a considerable amount of time, it is
			being invoked in a separate job thread
\*------------------------------------------------------------------------------*/
bool BmViewItemManager::ApplyFilter(BmViewItemFilter* filter, 
	ContinueCallback& continueCallback, BmListViewController* listView)
{
	BM_LOG2( BM_LogGui, BmString("starting to apply list-item filter"));
	BmListViewItem* viewItem;
	BmViewModelMap::const_iterator iter;
	{	// scope for lock
		BAutolock lock(mLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME(
				"BmViewItemManager::ApplyFilter(): Unable to get lock"
			);
	
		if (filter != mFilter) {
			delete mFilter;
			mFilter = filter;
		}
	
		for(iter = mViewModelMap.begin(); iter != mViewModelMap.end(); ++iter) {
			viewItem = iter->second;
			if (!viewItem || !continueCallback())
				return false;
			viewItem->ShouldBeHidden(mFilter && !mFilter->Matches(viewItem));
		}
	}
	
	/* The result of filter application will be changes to the shouldBeHidden
		attribute of the view items - every matching view item will have
		shouldBeHidden == false, while for all others shouldBeHidden == true.
		So now we have to adjust the visibility of the items accordingly:
	*/
	BM_LOG2( BM_LogGui, BmString("starting to adjust listview"));

	// always lock the looper first, then the manager lock, as otherwise
	// we might deadlock
	BAutolock listLock(listView->Looper());
	if (!listLock.IsLocked())
		BM_THROW_RUNTIME(
			"BmViewItemManager::ApplyFilter(): Unable to get looper lock!"
		);
	BAutolock lock(mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME(
			"BmViewItemManager::ApplyFilter(): Unable to get lock"
		);

	size_t mapSize = mViewModelMap.size();
	BList visibleItems((int32)min_c(INT32_MAX, mapSize));
	for(iter = mViewModelMap.begin(); iter != mViewModelMap.end(); ++iter) {
		if (!iter->second->ShouldBeHidden())
			visibleItems.AddItem(iter->second);
	}
	listView->ReplaceItemsWith(&visibleItems);
	BM_LOG2( BM_LogGui, BmString("finished with adjusting listview"));
	
	return true;
}


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
BmListViewController::BmListViewController( BRect rect,
								 const char* Name, list_view_type Type, 
								 bool hierarchical, bool showLabelView)
	:	inherited( rect, Name, 
					  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					  Type, hierarchical, showLabelView)
	,	inheritedController( Name)
	,	mInitialStateInfo( NULL)
	,	mUseStateCache( true)
	,	mCurrHighlightItem( NULL)
	,	mSittingOnExpander( false)
	,	mExpandCollapseRunner( NULL)
	,	mPulsedScrollRunner( NULL)
	,	mPulsedScrollStep( 0)
	,	mDragBetweenItems( false)
{
	BmString family = ThePrefs->GetString( "ListviewFont", "");
	int size = ThePrefs->GetInt( "ListviewFontSize", 0);
	BFont font(be_plain_font);
	if (family.Length())
		font.SetFamilyAndStyle( family.String(), NULL);
	if (size > 0)
		font.SetSize( float(size));
	SetFont(&font);
	float minHeight
		= hierarchical
			? float(ThePrefs->GetInt( "ListviewHierarchicalMinItemHeight", 16))
			: float(ThePrefs->GetInt( "ListviewFlatMinItemHeight", 16));
	SetMinItemHeight( MAX( TheResources->FontLineHeight(&font), minHeight));
}

/*------------------------------------------------------------------------------*\
	~BmListViewController()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmListViewController::~BmListViewController() {
	delete mExpandCollapseRunner;
	delete mPulsedScrollRunner;
	delete mInitialStateInfo;
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
void BmListViewController::MouseMoved( BPoint point, uint32 transit, 
													const BMessage *msg) {
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
					int32 pulsedScrollDelay 
						= ThePrefs->GetInt( "PulsedScrollDelay", 100);
					if (pulsedScrollDelay>0) {
						BMessage msg( BM_PULSED_SCROLL);
						msg.AddInt32( MSG_SCROLL_STEP, mPulsedScrollStep);
						BMessenger msgr( this);
						mPulsedScrollRunner = new BMessageRunner( 
							msgr, &msg, pulsedScrollDelay*1000, -1
						);
					}
				}
			}
			HighlightItemAt( point);
			if (Hierarchical() && mCurrHighlightItem 
			&& mCurrHighlightItem->ExpanderRectContains( point)) {
				if (!mSittingOnExpander) {
					if (!mCurrHighlightItem->IsExpanded()) {
						// expand superitem if mouse is over expander (so that user 
						// can navigate through the subitems that were previously 
						// hidden):
						Expand( mCurrHighlightItem);
					} else {
						// collapse superitem if mouse is over expander (so that 
						// user can hide subitems that were previously shown):
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
		BPoint highPt( point.x, point.y-firstItemRect.Height()/2.0f);
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
				int32 expandCollapseDelay 
					= ThePrefs->GetInt( "ExpandCollapseDelay", 1000);
				if (expandCollapseDelay>0) {
					BMessage msg( BM_EXPAND_OR_COLLAPSE);
					msg.AddPointer( MSG_HIGHITEM, (void*)mCurrHighlightItem);
					msg.AddBool( MSG_EXPAND, !mCurrHighlightItem->IsExpanded());
					BMessenger msgr( this);
					mExpandCollapseRunner = new BMessageRunner( 
						msgr, &msg, expandCollapseDelay*1000, 1
					);
				}
			}
		} else
			mCurrHighlightItem = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	CreateDragImage()
		-	
\*------------------------------------------------------------------------------*/
BBitmap* BmListViewController::CreateDragImage(const vector<int>& cols, 
															  int32 max) {
	int32 currIdx;
	// we count the number of selected items:
	int32 selCount;
	for( selCount=0; (currIdx=CurrentSelection( selCount))>=0; ++selCount)
		;
	BM_LOG2( BM_LogGui, BmString("CreateDragImage() - found ")
								<<selCount<<" selections");
	const float v = 5;
	const float h = 5;

	float width = 0;
	for( uint32 c=0; c<cols.size(); ++c) {
		CLVColumn* col = ColumnAt(cols[c]);
		if (!col)
			continue;
		width += col->Width();
	}

	BFont font;
	GetFont( &font);
	float lineHeight = MAX(TheResources->FontLineHeight( &font), 20.0f);
	float baselineOffset = TheResources->FontBaselineOffset( &font);
	BRect dragRect( 0, 0, 2*h+width-1, float(MIN(selCount,max))*lineHeight-1+v);
	BView* dummyView = new BView( dragRect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* dragImage = new BBitmap( dragRect, B_RGBA32, true);
	dragImage->AddChild( dummyView);
	dragImage->Lock();
	dummyView->SetHighColor( B_TRANSPARENT_COLOR);
	dummyView->FillRect( dragRect);
	dummyView->SetDrawingMode( B_OP_ALPHA);
	dummyView->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	rgb_color tcol = ui_color( B_PANEL_BACKGROUND_COLOR);
	tcol.alpha = 192;
	dummyView->SetHighColor( tcol);
	dummyView->FillRoundRect( dragRect, 5, 5);
	tcol = BmWeakenColor( B_PANEL_BACKGROUND_COLOR, 3);
	tcol.alpha = 192;
	dummyView->SetHighColor( tcol);
	dummyView->SetPenSize( 2);
	dummyView->StrokeRoundRect( dragRect, 5, 5);
	dummyView->SetPenSize( 1);
	dummyView->SetHighColor( ui_color( B_PANEL_TEXT_COLOR));
	dragRect.InsetBy(h,v);
	BRegion region;
	// now we add all selected items to drag-image and to drag-msg:
	for( int32 i=0; (currIdx=CurrentSelection( i))>=0; ++i) {
		BmListViewItem* item = dynamic_cast<BmListViewItem*>(ItemAt( currIdx));
		if (i==max-1 && selCount>max) {
			// add an indicator that more items are being dragged than shown:
			region.Set(dragRect);
			dummyView->ConstrainClippingRegion(&region);
			BmString indicator = BmString("(...and ") << selCount-max 
				<< (selCount-max == 1 ? " more item)" : " more items)");
			dummyView->DrawString( indicator.String(), 
										  BPoint( h, v+float(i)*lineHeight+baselineOffset));
			dummyView->ConstrainClippingRegion(NULL);
		} else if (i<max) {
			// add only the first couple of selections to drag-image:
			float hOffs = 0;
			for( uint32 c=0; c<cols.size(); ++c) {
				CLVColumn* col = ColumnAt(cols[c]);
				if (!col)
					continue;
				if ((col->Type() & CLV_COLTYPE_MASK) == CLV_COLTYPE_BITMAP) {
					const BmBitmapHandle* icon 
						= item->GetColumnContentBitmap( cols[c]);
					if (icon && icon->bitmap) {
						dummyView->DrawBitmapAsync( 
							icon->bitmap, BPoint(h+hOffs,v+float(i)*lineHeight));
					}
				} else {
					const char* text = item->GetColumnContentText( cols[c]);
					if (text)
						dummyView->DrawString( 
							text, 
							BPoint( h+hOffs, 
									  v+float(i)*lineHeight+baselineOffset)
						);
				}
				hOffs += col->Width();
			}
		}
	}
	dummyView->Sync();
	dragImage->Unlock();
	return dragImage;
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::HandleDrop( BMessage*) {
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
				item->RemoveRef();
							// the msg is no longer referencing the item
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
				item->RemoveRef();
							// the msg is no longer referencing the item
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
				item->RemoveRef();
							// the msg is no longer referencing the item
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
				if (fScrollView)
					fScrollView->SetBusy();
				break;
			}
			case BMM_UNSET_BUSY: {
				if (fScrollView)
					fScrollView->UnsetBusy();
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
						int32 lastVisibleIndex 
							= IndexOf( BPoint( 0, bounds.bottom-5));
						if (lastVisibleIndex>=0 
						&& lastVisibleIndex+step < CountItems()) {
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
BmListViewItem* 
BmListViewController::FindViewItemFor( BmListModelItem* modelItem) const {
	return mViewItemManager.FindViewItemFor(modelItem);
}

/*------------------------------------------------------------------------------*\
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AddAllModelItems() {
	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())<<": adding items to listview");
	MakeEmpty();
	BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
	BM_ASSERT( model);
	BmAutolockCheckGlobal lock( model->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			model->ModelNameNC() << ": Unable to get lock"
		);
	BList* tempList = NULL;
	if (!Hierarchical())
		tempList = new BList((int32)min_c(INT32_MAX, model->size()));
	SetDisconnectScrollView( true);
	SetInsertAtSortedPos( false);
	BmModelItemMap::const_iterator iter;
	BmModelItemMap::const_iterator end = model->end();
	int32 count=1;
	for( iter = model->begin(); iter != end; ++iter) {
		BmListModelItem* modelItem = iter->second.Get();
		if (!modelItem->IsValid())
			continue;
		BmListViewItem* viewItem;
		if (Hierarchical()) {
			// add item and its subitems to the listview:
			doAddModelItem( NULL, modelItem, false);
		} else {
			viewItem = CreateListViewItem( modelItem);
			if (viewItem) {
				mViewItemManager.Add(modelItem, viewItem);
				if (!viewItem->ShouldBeHidden()) {
					viewItem->UpdateView( UPD_ALL, false);
					tempList->AddItem( viewItem);
				}
				BM_LOG2( BM_LogMailTracking, 
							BmString("ListView <") << ModelName() << "> added view-item "
								<< viewItem->Key());
			} else
				BM_LOG( BM_LogMailTracking, 
						  BmString("ListView <") << ModelName() 
								<< "> could not create view-item for modelItem "
								<< modelItem->Key());
		}
		if (count%100==0) {
			if (fScrollView)
				fScrollView->PulseBusyView();
			BmString caption = BmString() << count << " " << ItemNameForCaption()
										<< (count>1?"s":"");
			UpdateCaption( caption.String());
		}
		count++;
	}
	if (!Hierarchical()) {
		// add complete item-list for efficiency:
		AddList( tempList);
		delete tempList;
	}

	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())
					<< ": added all items to listview, now sorting");
	SortItems();
	SetInsertAtSortedPos( true);
	SetDisconnectScrollView( false);
	UpdateDataRect( true);
	UpdateCaption();
	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())
					<< ": finished with sorting added items");
}

/*------------------------------------------------------------------------------*\
	AddModelItem( item)
		-	Hook function that is called whenever a new item has been added to the 
			listmodel
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::AddModelItem( BmListModelItem* item) {
	BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
	BM_ASSERT( model);
	BmAutolockCheckGlobal lock( model->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			model->ModelNameNC() << ": Unable to get lock"
		);
	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())<<": adding one item to listview");
	BmListViewItem* newItem = FindViewItemFor( item);
	if (!newItem) {
		if (!Hierarchical()) {
			newItem = doAddModelItem( NULL, item, true);
		} else {
			BmRef<BmListModelItem> parent( item->Parent());
			BmListViewItem* parentItem = FindViewItemFor( parent.Get());
			newItem = doAddModelItem( parentItem, item, true);
		}
		BMessage msg( BM_NTFY_LISTCONTROLLER_MODIFIED);
		SendNotices( BM_NTFY_LISTCONTROLLER_MODIFIED, &msg);
	}
	return newItem;
}

/*------------------------------------------------------------------------------*\
	doAddModelItem( BmListViewItem* parent, BmListModelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::doAddModelItem( BmListViewItem* parent, 
																		BmListModelItem* item,
																		bool redraw) {
	std::auto_ptr<BMessage> archive( 
		Hierarchical() 
			? GetArchiveForItemKey( item->Key()) 
			: NULL
	);
	BmListViewItem* newItem = CreateListViewItem( item, archive.get());
	if (newItem) {
		mViewItemManager.Add(item, newItem);
		if (!newItem->ShouldBeHidden()) {
			if (parent)
				AddUnder( newItem, parent);
			else
				AddItem( newItem);
			newItem->UpdateView( UPD_ALL, redraw);
		}
		BM_LOG2( BM_LogMailTracking, 
					BmString("ListView <") << ModelName() << "> added view-item " 
						<< newItem->Key());
	}
	
	// add all sub-items of current item to the view as well:
	BmModelItemMap::const_iterator iter;
	BmModelItemMap::const_iterator endIter = item->end();
	for( iter = item->begin(); iter != endIter; ++iter) {
		BmListModelItem* subItem = iter->second.Get();
		doAddModelItem( newItem, subItem, redraw);
	}
	return newItem;
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	Hook function that is called whenever an item has been deleted from the
			listmodel
\*------------------------------------------------------------------------------*/
void BmListViewController::RemoveModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, 
				BmString(ControllerName())<<": removing one item from listview");
	if (item) {
		BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
		BM_ASSERT( model);
		BmAutolockCheckGlobal lock( model->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				model->ModelNameNC() << ": Unable to get lock"
			);
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
	BmListViewItem* viewItem = mViewItemManager.Remove(item);
	if (viewItem) {
		RemoveItem( viewItem);
		BM_LOG2( BM_LogMailTracking, 
					BmString("ListView <") << ModelName() 
						<< "> removed view-item " << viewItem->Key());
		// remove all sub-items of current item from the view as well:
		BmModelItemMap::const_iterator iter;
		for( iter = item->begin(); iter != item->end(); ++iter) {
			BmListModelItem* subItem = iter->second.Get();
			doRemoveModelItem( subItem);
		}
		delete viewItem;
	}
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( item, updFlags)
		-	Hook function that is called whenever an item needs to be updated 
		-	updFlags may contain info on which parts (columns) of the item should 
			be updated.
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::UpdateModelItem( BmListModelItem* item, 
																		 BmUpdFlags updFlags) {
	BmListViewItem* viewItem = FindViewItemFor( item);
	if (viewItem) {
		int32 itemIndex = IndexOf( viewItem);
		bool needRedraw = false; 
		if (itemIndex >= 0) {
			BRect b = Bounds();
			int32 visibleTopIndex = IndexOf( b.LeftTop());
			int32 visibleBottomIndex = IndexOf( b.LeftBottom());
			if (visibleBottomIndex <= 0)
				visibleBottomIndex = CountItems()-1;
			if (itemIndex >= visibleTopIndex && itemIndex <= visibleBottomIndex)
				needRedraw = true;
		}
		BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
		BM_ASSERT( model);
		BmAutolockCheckGlobal lock( model->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				model->ModelNameNC() << ": Unable to get lock"
			);
		viewItem->UpdateView( updFlags, needRedraw);
	} else
		BM_LOG2( BM_LogModelController, 
					BmString(ControllerName())
						<< ": requested to update an unknown item <"
						<< item->Key()<<">");
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
	if (fScrollView) {
		if (text) {
			fScrollView->SetCaptionText( text);
		} else {
			int32 numItems = FullListCountItems();
			BmString caption;
			if (!numItems)
				caption = BmString("no ")<<ItemNameForCaption()<<"s";
			else
				caption = BmString("")<<numItems<<" "<<ItemNameForCaption()
								<<(numItems>1 ? "s" : "");
			fScrollView->SetCaptionText( caption.String());
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
void BmListViewController::PopulateLabelViewMenu( BMenu* menu) {
	int32 numCols = fColumnList.CountItems();
	int32 numVisibleCols = fColumnDisplayList.CountItems();
	for( int32 i=0; i<numCols; ++i) {
		CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( i);
		if (!column) continue;
		uint32 flags = column->Flags();
		if (flags & (CLV_EXPANDER | CLV_LOCK_WITH_RIGHT | CLV_MERGE_WITH_RIGHT 
							| CLV_NOT_MOVABLE)) continue;
		bool shown = fColumnDisplayList.HasItem( column);
		BMessage* msg = new BMessage( 
			shown 
				? BM_LISTVIEW_HIDE_COLUMN 
				: BM_LISTVIEW_SHOW_COLUMN
		);
		msg->AddInt32( MSG_COLUMN_NO, i);
		BmString label = column->GetLabelName();
		BMenuItem* item = new BMenuItem( label.String(), msg);
		item->SetMarked( shown);
		if (shown && numVisibleCols == 1) {
			// don't allow user to remove last visible column:
			item->SetEnabled( false);
		}
		item->SetTarget( this);
		menu->AddItem( item);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ShowLabelViewMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "LabelViewMenu", false, false);
	BFont font( *be_plain_font);
	theMenu->SetFont( &font);

	PopulateLabelViewMenu( theMenu);

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
		BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
		BM_ASSERT( model);
		BmAutolockCheckGlobal lock( model->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				model->ModelNameNC() << ": Unable to get lock"
			);
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
	if (fScrollView)
		fScrollView->UnsetBusy();
	MakeEmpty();
	UpdateCaption();
}

/*------------------------------------------------------------------------------*\
	MakeEmpty()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MakeEmpty() {
	if (LockLooper()) {
		inherited::MakeEmpty();					// clear display
		mViewItemManager.MakeEmpty();
		UpdateCaption();
		UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	ApplyViewItemFilter( )
	-	
\*------------------------------------------------------------------------------*/
bool BmListViewController::ApplyViewItemFilter(BmViewItemFilter* filter, 
	BmViewItemManager::ContinueCallback& continueCallback)
{
	bool result = mViewItemManager.ApplyFilter(filter, continueCallback, this);
	if (LockLooper()) {
		UpdateCaption();
		UnlockLooper();
	}
	return result;
}

/*------------------------------------------------------------------------------*\
	ApplyModelItemFilter( )
	-	
\*------------------------------------------------------------------------------*/
bool BmListViewController::ApplyModelItemFilter(BmListModelItemFilter* filter)
{
	bool result = false;
	if (LockLooper()) {
		BmListModel *model = dynamic_cast<BmListModel*>(DataModel().Get());
		BM_ASSERT(model);
		model->MarkCacheAsDirty();
			// we want to bypass the cache, as it may no longer contain all mails
		model->SetFilter(filter);
		MakeEmpty();
		StartJob();
		UnlockLooper();
		result = true;
	}
	return result;
}

/*------------------------------------------------------------------------------*\
	StartJob( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::StartJob( BmJobModel* model, bool startInNewThread,
											    int32 jobSpecifier) {
	if (fScrollView)
		fScrollView->SetBusy();
	UpdateCaption( "tracking...");
	inheritedController::StartJob( model, startInNewThread, jobSpecifier);
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	Hook function that is called whenever the jobmodel associated to this 
			controller indicates that it is done (meaning: the list has been 
			fetched and is now ready to be displayed).
\*------------------------------------------------------------------------------*/
void BmListViewController::JobIsDone( bool completed) {
	if (completed) {
		AddAllModelItems();
	} else {
		UpdateCaption( "");
	}
	if (fScrollView)
		fScrollView->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::WriteStateInfo() {
	status_t err;
	BFile stateInfoFile;
	BMessage archive;
	
	BmListModel* model = dynamic_cast<BmListModel*>(DataModel().Get());
	if (!model || !model->InitCheck()==B_OK || !mUseStateCache)
		return;

	try {
		BmString stateInfoFilename = StateInfoFilename( false);
		if (this->Archive( &archive, Hierarchical()) != B_OK)
			BM_THROW_RUNTIME( BmString("Unable to archive State-Info for ")
										<< Name());
		if ((err = stateInfoFile.SetTo( 
			BeamRoster->StateInfoFolder(), 
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
			BmListViewItem* item 
				= static_cast< BmListViewItem*>( FullListItemAt( i));
			ret = item->Archive( archive, deep);
		}
	}
	if (ret != B_OK) {
		BM_SHOWERR( BmString("Could not archive State-Info for ") 
							<< ModelName() << "\n\tError: "<< strerror( ret));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetArchiveForItemKey( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmListViewController::GetArchiveForItemKey( const BmString& key, 
																		BMessage* msg) {
	if (mInitialStateInfo) {
		status_t ret = B_OK;
		for( int i=0; ret==B_OK; ++i) {
			const char* name;
			ret = mInitialStateInfo->FindString( 
				BmListViewItem::MSG_CHILDNAMES, i, &name
			);
			if (key == name) {
				return FindMsgMsg( 
					mInitialStateInfo, BmListViewItem::MSG_CHILDREN, msg, i
				);
			}
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	StateInfoFilename( )
		-	
\*------------------------------------------------------------------------------*/
BmString BmListViewController::StateInfoFilename( bool forRead) {
	return StateInfoBasename() << "_" << ModelName();
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
	stateInfoFilename = StateInfoFilename(true);
	if (mUseStateCache
	&& (err = stateInfoFile.SetTo( 
		BeamRoster->StateInfoFolder(), 
		stateInfoFilename.String(), 
		B_READ_ONLY
	)) != B_OK) {
		// state-file not found, but we have changed names of statefiles 
		// in Feb 2003, so we check if a state-file according to the old name 
		// exists (and rename it to match our new regulations):
		Regexx rx( stateInfoFilename, "^(.+?_.+?_\\d+)_\\d+(.+?)$", "$1$2");
		BmString oldStateInfoFilename = rx;
		BEntry entry( 
			BeamRoster->StateInfoFolder(), oldStateInfoFilename.String()
		);
		err = (entry.InitCheck() || entry.Rename( stateInfoFilename.String()));
	}
	if (mUseStateCache
	&& (err = stateInfoFile.SetTo( 
		BeamRoster->StateInfoFolder(), 
		stateInfoFilename.String(), 
		B_READ_ONLY
	)) == B_OK) {
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
