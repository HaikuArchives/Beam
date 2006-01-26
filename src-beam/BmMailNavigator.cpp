/*
 * Copyright 2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "BmMail.h"
#include "BmMailNavigator.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
#include "BmMailView.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefSelector::operator() (BmMailRef* mailRef) const
{
	return mailRef && mailRef->IsValid();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmNewMailRefSelector::operator() (BmMailRef* mailRef) const
{
	return mailRef && mailRef->Status() == BM_MAIL_STATUS_NEW;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailNavigator::BmMailNavigator(BmMailRefView* refView, 
											BmMailView* mailView,
											const BmMailRefSelector& selector)
  :	mRefView(refView)
  ,	mMailView(mailView)
  ,	mMailRefSelector(selector)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailNavigator::MoveBackward()
{
	if (!_MoveMailView())
		_MoveRefView(true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailNavigator::MoveForward()
{
	if (!_MoveMailView())
		_MoveRefView(false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailNavigator::_MoveMailView()
{
	bool result = false;
	if (mMailView && mMailView->LockLooper()) {
		float currTop = mMailView->Bounds().top;
		char navKey = B_PAGE_DOWN;
		mMailView->KeyDown( &navKey, 1);
		result = mMailView->Bounds().top != currTop;
		mMailView->UnlockLooper();
	}
	return result;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailNavigator::_MoveRefView(bool backward)
{
	bool result = false;
	if (mRefView && mRefView->LockLooper()) {
		// if a date field is part of the sorting order, we check if
		// its sort mode is descending. If that is the case, we reverse
		// the navigation order, i.e. follow chronologic order:
		int32 sortKeys[100];
		CLVSortMode sortModes[100];
		int32 sortKeyCount = mRefView->GetSorting(sortKeys, sortModes);
		for(int32 i=0; i<sortKeyCount; ++i) {
			CLVColumn* column = mRefView->ColumnAt(sortKeys[i]);
			if (column->Flags() & (CLV_COLDATA_DATE | CLV_COLDATA_BIGTIME)) {
				if (sortModes[i] == Descending) {
					backward = !backward;
					break;
				}
			}
		}

		// now try to find next matching mail-ref:
		BmMailRefItem* refItem = NULL;
		int32 currSel = mRefView->CurrentSelection();
		if (backward) {
			int32 idx = currSel >= 0 ? currSel-1 : mRefView->CountItems()-1;
			for( ; idx >= 0; --idx) {
				refItem = dynamic_cast<BmMailRefItem*>(mRefView->ItemAt(idx));
				if (refItem && mMailRefSelector(refItem->ModelItem())) {
					mRefView->Select(idx);
					mRefView->ScrollToSelection();
					result = true;
					break;
				}
			}
		} else {
			int32 idx = currSel >= 0 ? currSel+1 : 0;
			for( ; idx < mRefView->CountItems(); ++idx) {
				refItem = dynamic_cast<BmMailRefItem*>(mRefView->ItemAt(idx));
				if (refItem && mMailRefSelector(refItem->ModelItem())) {
					mRefView->Select(idx);
					mRefView->ScrollToSelection();
					result = true;
					break;
				}
			}
		}
		mRefView->UnlockLooper();
	}
	return result;
}
