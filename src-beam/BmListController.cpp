/*
	BmListController.cpp
		$Id$
*/

#include <memory.h>

#include <Autolock.h>

#include "BmApp.h"
#include "BmListController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmUtil.h"

//#include <Profile.h>


/********************************************************************************\
	BmListViewItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::BmListViewItem( BString& key, BmListModelItem* modelItem,
										  bool hierarchical, BMessage* archive)
	:	inherited( 0, !modelItem->empty(), false, MAX(bmApp->FontLineHeight(), 18))
	,	mKey( key)
	,	mModelItem( modelItem)
	,	mSubItemList( NULL)
{
	SetExpanded( archive ? archive->FindBool( MSG_EXPANDED) : false);
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
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewItem::Archive( BMessage* archive, bool deep) const {
	// first we archive the item's attributes into a separate message...
	BMessage msg;
	msg.AddBool( MSG_EXPANDED, IsExpanded());

	// ...now we add this message with a corresponding key to the archive:
	status_t ret = archive->AddMessage( MSG_CHILDREN, &msg)
						|| archive->AddString( MSG_CHILDNAMES, Key());

	if (deep && ret==B_OK && mSubItemList) {
		// we repeat the procedure for all our subitems:
		int32 numItems = mSubItemList->CountItems();
		for( int32 i=0; i<numItems && ret==B_OK; ++i) {
			BmListViewItem* item = static_cast< BmListViewItem*>( mSubItemList->ItemAt( i));
			ret = item->Archive( archive, deep);
		}
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::UpdateView( BmUpdFlags flags) {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::SetTextCols( int16 firstTextCol, BmListColumn* columnVec,
											 bool truncate) {
	int16 offs = firstTextCol;
	for( const BmListColumn* p = columnVec; p->text != NULL; ++p) {
		SetColumnContent( offs++, p->text, truncate, p->rightJustified);
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
		for( iter = mModelItem->begin(); iter != mModelItem->end(); ++iter) {
			BmListModelItem* subItem = iter->second;
			BMessage* archive = view->GetArchiveForItemKey( subItem->Key());
			BmListViewItem* viewItem = view->CreateListViewItem( subItem, archive);
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
	,	mInitialStateInfo( NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmListViewController()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmListViewController::~BmListViewController() {
	delete mInitialStateInfo;
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
		BM_SHOWERR( BString(ControllerName()) << ":\n\t" << err.what());
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
	AddAllModelItemsToList()
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
		BmListViewItem* viewItem;
		if (Hierarchical()) {
			BMessage* archive = GetArchiveForItemKey( modelItem->Key());
			viewItem = CreateListViewItem( modelItem, archive);
			mItemList->AddItem( viewItem);
			// immediately add item and its subitems to the listview:
			AddItem( viewItem);
			viewItem->AddSubItemsToList( this);
		} else {
			viewItem = CreateListViewItem( modelItem);
			mItemList->AddItem( viewItem);
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
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ExpansionChanged( CLVListItem* _item, bool expanded) {
	BmListViewItem* item = dynamic_cast< BmListViewItem*>( _item);
	UpdateItem( item, BmListViewItem::UPD_EXPANDER);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateItem( BmListViewItem* item, BmUpdFlags flags) {
	if (!item)
		return;
	LockLooper();
	item->UpdateView( flags);
	InvalidateItem( IndexOf( item));
	UnlockLooper();
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
	MakeEmpty();								// clear display
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
	JobIsDone( completed)
		-	Hook function that is called whenever the jobmodel associated to this 
			controller indicates that it is done (meaning: the list has been fetched
			and is now ready to be displayed).
\*------------------------------------------------------------------------------*/
void BmListViewController::JobIsDone( bool completed) {
	if (completed) {
		AddModelItemsToList();
	}
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::WriteStateInfo() {
	status_t err;
	BString stateInfoFilename;
	BFile stateInfoFile;
	auto_ptr< BMessage> archive( new BMessage);
	
	if (!DataModel())
		return;

	try {
		stateInfoFilename = BString(bmApp->SettingsPath.Path()) << "/StateInfo/" 
								  << StateInfoBasename() << "_" << ModelName();
		this->Archive( archive.get(), Hierarchical()) == B_OK
													|| BM_THROW_RUNTIME( BString("Unable to archive State-Info for ")<<Name());
		(err = stateInfoFile.SetTo( stateInfoFilename.String(), 
											 B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create state-info file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
		(err = archive->Flatten( &stateInfoFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store state-info into file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	Archive()
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewController::Archive(BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (deep && ret == B_OK && mItemList) {
		int32 numItems = mItemList->CountItems();
		for( int32 i=0; i<numItems && ret==B_OK; ++i) {
			BmListViewItem* item = static_cast< BmListViewItem*>( mItemList->ItemAt( i));
			ret = item->Archive( archive, deep);
		}
	}
	if (ret != B_OK) {
		ShowAlert( BString("Could not archive State-Info for ") << ModelName() << " errortext: "<< strerror( ret));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetArchiveForItemKey( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmListViewController::GetArchiveForItemKey( BString key) {
	if (mInitialStateInfo) {
		status_t ret = B_OK;
		for( int i=0; ret==B_OK; ++i) {
			const char* name;
			ret = mInitialStateInfo->FindString( BmListViewItem::MSG_CHILDNAMES, i, &name);
			if (key == name) {
				return FindMsgMsg( mInitialStateInfo, BmListViewItem::MSG_CHILDREN, NULL, i);
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
	BString stateInfoFilename;
	BFile stateInfoFile;

	// try to open state-info-file...
	stateInfoFilename = BString(bmApp->SettingsPath.Path()) << "/StateInfo/" 
							  << StateInfoBasename() << "_" << ModelName();
	if ((err = stateInfoFile.SetTo( stateInfoFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, file found, we fetch our state(s) from it:
		try {
			mInitialStateInfo = new BMessage;
			(err = mInitialStateInfo->Unflatten( &stateInfoFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch state-info from file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
			inherited::UnArchive( mInitialStateInfo);
		} catch (exception &e) {
			delete mInitialStateInfo;
			mInitialStateInfo = NULL;
			BM_SHOWERR( e.what());
		}
	} else 
		inherited::UnArchive( bmApp->Prefs->MailRefLayout());
}
