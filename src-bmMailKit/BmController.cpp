/*
	BmController.cpp
		$Id$
*/

#include <Autolock.h>

#include "BmApp.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

//#include <Profile.h>

/********************************************************************************\
	BmController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmController::BmController( BString name)
	:	mDataModel( NULL)
	,	mControllerName( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmController()
		-	destructor that ensures a detach from the model (if any)
\*------------------------------------------------------------------------------*/
BmController::~BmController() {
	DetachModel();
}

/*------------------------------------------------------------------------------*\
	AttachModel( model)
		-	tell a model that we are interested in its state
		-	if the "model"-argument is specified, it will become the new model
			of interest.
		-	safe to call with a NULL-model
\*------------------------------------------------------------------------------*/
void BmController::AttachModel( BmDataModel* model) {
	if (model) {
		if (mDataModel) {
			// detach current model, since we shall control a new one:
			DetachModel();
		}
		DataModel( model);
	}
	if (mDataModel) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> attaches to model " << ModelName());
		mDataModel->AddController( this);
	}
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	tell a model that we are no longer interested
		-	safe to call if there is no attached model
\*------------------------------------------------------------------------------*/
void BmController::DetachModel() {
	if (mDataModel) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> detaches from model " << ModelName());
		mDataModel->RemoveController( this);
		mDataModel = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	IsMsgFromCurrentModel( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmController::IsMsgFromCurrentModel( BMessage* msg) {
	BString msgModelName = FindMsgString( msg, BmDataModel::MSG_MODEL);
	return msgModelName == ModelName();
}



/********************************************************************************\
	BmJobController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmJobController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmJobController::BmJobController( BString name)
	:	BmController( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobController()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmJobController::~BmJobController() {
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobController::StartJob( BmJobModel* model, bool startInNewThread, 
										  bool deleteWhenDone) {
	if (model) {
		AttachModel( model);
	}
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> starts job " << ModelName());
		if (startInNewThread)
			DataModel()->StartJobInNewThread( deleteWhenDone);
		else
			DataModel()->StartJobInThisThread();
	}
}

/*------------------------------------------------------------------------------*\
	PauseJob( msg)
		-	hook-method that is called when the user wants this Job to pause
		-	parameter msg may contain any further attributes needed for update
		-	this default implementation simply tells its model to pause
\*------------------------------------------------------------------------------*/
void BmJobController::PauseJob( BMessage* msg) {
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> pauses job " << ModelName());
		DataModel()->PauseJob();
	}
}

/*------------------------------------------------------------------------------*\
	ContinueJob( msg)
		-	hook-method that is called when the user wants this Job to continue
		-	parameter msg may contain any further attributes
		-	this default implementation simply tells its model to continue
\*------------------------------------------------------------------------------*/
void BmJobController::ContinueJob( BMessage* msg) {
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> continues job " << ModelName());
		DataModel()->ContinueJob();
	}
}

/*------------------------------------------------------------------------------*\
	StopJob()
		-	this method is called when the user wants to stop a job
		-	this default implementation simply detaches from its model
\*------------------------------------------------------------------------------*/
void BmJobController::StopJob() {
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> stops job " << ModelName());
	BmJobModel* job = DataModel();
	if (job) {
		job->StopJob();
		DetachModel();
	}
}

/*------------------------------------------------------------------------------*\
	IsJobRunning()
		-	
\*------------------------------------------------------------------------------*/
bool BmJobController::IsJobRunning() {
	BmJobModel* model = DataModel();
	if (model) {
		return model->IsJobRunning();
	} else {
		return false;
	}
}



/********************************************************************************\
	BmListViewItem
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::BmListViewItem( BString& key, BmListModelItem* modelItem, BBitmap* icon, 
										  bool hierarchical, uint32 level, 
										  bool superitem, bool expanded) 
	:	inherited( level, superitem, expanded, MAX(bmApp->FontLineHeight(), 18))
	,	mKey( key)
	,	mModelItem( modelItem)
	,	mOffs( hierarchical ? 1 : 0)
							// skip expander if listview is hierarchical
	,	mSubItemList( NULL)
{
	SetColumnContent( mOffs++, icon, 2.0, false);
							// no offset and no copy of our icon
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::~BmListViewItem() {
	if (mSubItemList) {
		while( !mSubItemList->IsEmpty()) {
			BmListViewItem* subItem = static_cast<BmListViewItem*>(mSubItemList->RemoveItem( (int32)0));
			delete subItem;
		}
		delete mSubItemList;
		mSubItemList = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::SetTextCols( BmListColumn* columnVec) {
	for( const BmListColumn* p = columnVec; p->text != NULL; ++p) {
		SetColumnContent( mOffs++, p->text, false, true, p->rightJustified);
	}
}

/*------------------------------------------------------------------------------*\
	AddSubItemsToMap()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::AddSubItemsToList( BmListViewController* view) {
	BmModelItemMap::const_iterator iter;
	if (!mModelItem->empty()) {
		mSubItemList = new BList( mModelItem->size());
		SetSuperItem( true);
		SetExpanded( true);
		for( iter = mModelItem->begin(); iter != mModelItem->end(); ++iter) {
			BmListModelItem* subItem = iter->second;
			BmListViewItem* viewItem = view->CreateListViewItem( subItem);
			mSubItemList->AddItem( viewItem);
			view->AddUnder( viewItem, this);
			viewItem->AddSubItemsToList( view);
		}
	}
}



/********************************************************************************\
	BmListViewController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmListViewController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmListViewController::BmListViewController( minimax minmax, BRect rect,
								 const char* Name, list_view_type Type, bool hierarchical, 
								 bool showLabelView)
	:	inherited( minmax, rect, Name, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, 
					  Type, hierarchical, showLabelView)
	,	inheritedController( Name)
	,	mItemList( NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmListViewController()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmListViewController::~BmListViewController() {
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	clears listview after detach
\*------------------------------------------------------------------------------*/
void BmListViewController::DetachModel() {
	inheritedController::DetachModel();
	MakeEmpty();		// clear display
	if (mItemList) {
		while( !mItemList->IsEmpty()) {
			BmListViewItem* subItem = static_cast<BmListViewItem*>(mItemList->RemoveItem( (int32)0));
			delete subItem;
		}		
		delete mItemList;
		mItemList = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_LISTMODEL_ADD: {
				if (!IsMsgFromCurrentModel( msg)) break;
				AddModelItem( msg);
				break;
			}
			case BM_LISTMODEL_REMOVE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				RemoveModelItem( msg);
				break;
			}
			case BM_LISTMODEL_UPDATE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				UpdateModelItem( msg);
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
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString(ControllerName()) << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddModelItem( msg)
		-	Hook function that is called whenever a new item has been added to the 
			listmodel
\*------------------------------------------------------------------------------*/
void BmListViewController::AddModelItem( BMessage* msg) {
/*
	BmListViewItem* newItem = NULL;
	BmListViewItem* parentItem = NULL;

	BmListModelItem* item = static_cast<BmListModelItem*>(
											FindMsgPointer( msg, BmListModel::MSG_MODELITEM));
	BString key = item->Key();

	// TODO: add mechanism for inserting at correct position
	if (!Hierarchical()) {
		newItem = CreateListViewItem( item);
		mItemMap[key] = newItem;
		AddItem( newItem);
	} else {
		BString pKey = item->ParentKey();
		parentItem = mItemMap[pKey];
		if (!parentItem) {
			(parentItem = TopHierarchyItem())
													|| BM_THROW_RUNTIME( BString("BmListViewController: No parent-item found that has key ") << pKey);
		}
		parentItem->SetSuperItem( true);
		parentItem->SetExpanded( true);
		
		newItem = CreateListViewItem( item, parentItem->OutlineLevel()+1);
		mItemMap[key] = newItem;
		AddUnder( newItem, parentItem);
	}
*/
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	Hook function that is called whenever an item has been deleted from the
			listmodel
		-	this implementation informs the model that we have noticed the removal 
			of the item. This is neccessary since the model is waiting for all 
			controllers to signal that they have understood that the deleted item
			is no longer valid. Otherwise, a controller might still access
			an item that has already been deleted by the model.
\*------------------------------------------------------------------------------*/
void BmListViewController::RemoveModelItem( BMessage* msg) {
	BmListModel *model = DataModel();
	if (model) {
		model->RemovalNoticed( this);
	}
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( msg)
		-	Hook function that is called whenever an item needs to be updated 
		-	default implementation does nothing
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateModelItem( BMessage* msg) {
}

/*------------------------------------------------------------------------------*\
	UpdateModelState( msg)
		-	Hook function that is called whenever the model as a whole (the list)
			has changed state
		-	default implementation does nothing
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateModelState( BMessage* msg) {
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	Hook function that is called whenever a jobmodel indicates that it is done
\*------------------------------------------------------------------------------*/
void BmListViewController::JobIsDone( bool completed) {
	if (completed) {
		AddModelItemsToList();
	}
}

/*------------------------------------------------------------------------------*\
	AddAllModelItemsToMap()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AddModelItemsToList() {
	BAutolock lock( DataModel()->ModelLocker());
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( BString() << ControllerName() << ":AddModelItemsToMap(): Unable to lock model");
	BmListModel *model = DataModel();
	mItemList = new BList( model->size());
	SetDisconnectScrollView( true);
	BmModelItemMap::const_iterator iter;
	for( iter = model->begin(); iter != model->end(); ++iter) {
		BmListModelItem* modelItem = iter->second;
		BmListViewItem* viewItem = CreateListViewItem( modelItem);
		mItemList->AddItem( viewItem);
		if (Hierarchical()) {
			// immediately add item and its subitems:
			AddItem( viewItem);
			viewItem->AddSubItemsToList( this);
		}
	}
	if (!Hierarchical()) {
		// add complete item-list for efficiency:
		AddList( mItemList);
	}
	SortItems();
	SetDisconnectScrollView( false);
	UpdateColumnSizesDataRectSizeScrollBars( true);
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	override that automatically grabs the keyboard-focus after a click 
			inside the listview
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseDown(BPoint point) { 
	inherited::MouseDown( point); 
	MakeFocus( true);
}

