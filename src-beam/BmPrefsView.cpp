/*
	BmPrefsView.cpp
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

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif

#include <liblayout/LayeredGroup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "Colors.h"

#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmPrefsView.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPrefsView
\********************************************************************************/

const char* const BmPrefsView::MSG_ACCOUNT = 	"bm:acc";
const char* const BmPrefsView::MSG_FIELD_NAME = "bm:fname";
const char* const BmPrefsView::MSG_COMPLAINT = 	"bm:compl";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsView::BmPrefsView( BmString label) 
	:	inherited( M_NO_BORDER, 0, (char*)label.String())
	,	mInitDone( false)
	,	mLabelView( NULL)
{
	MView* view;
	if (label.Length()) {
		view = 
			new VGroup( 
				minimax(100,100),
				new MBorder( M_DEPRESSED_BORDER, 2, NULL, 
					mLabelView = new MStringView( label.String(), B_ALIGN_CENTER)
				),
				new Space( minimax(0,2,0,2)),
				0
			);
	} else {
		view = new Space();
	}
	mGroupView = dynamic_cast<BView*>( view);
	AddChild( mGroupView);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsView::~BmPrefsView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsView::Activated() {
	if (!mInitDone)
		Initialize();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsView::Initialize() {
	if (mLabelView) {
		BFont font = *be_bold_font;
		font.SetSize(12);
		mLabelView->SetFont( &font);
		mLabelView->SetViewColor( MedMetallicBlue);
		mLabelView->SetLowColor( MedMetallicBlue);
		mLabelView->SetHighColor( White);
	}
	layoutprefs();
	layout( Bounds());
	mInitDone = true;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << mLabelView->Text() << ":\n\t" << err.what());
	}
}



/********************************************************************************\
	BmPrefsViewContainer
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsViewContainer::BmPrefsViewContainer( LayeredGroup* group)
	:	MBorder( M_NO_BORDER, 0, (char*)"PrefsViewContainer", group)
	,	mLayeredGroup( group)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsViewContainer::ShowPrefs( int index) {
	if (!mLayeredGroup || index < 0 || index >= mLayeredGroup->CountChildren())
		return;
	mLayeredGroup->ActivateLayer( index);
	BmPrefsView* pv = dynamic_cast<BmPrefsView*>( mLayeredGroup->ChildAt( index));
	if (pv)
		pv->Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsViewContainer::WriteStateInfo() {
	if (!mLayeredGroup)
		return;
	for( int i=0; i<mLayeredGroup->CountChildren(); ++i) {
		BmPrefsView* pv = dynamic_cast<BmPrefsView*>( mLayeredGroup->ChildAt( i));
		if (pv)
			pv->WriteStateInfo();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsViewContainer::SaveData() {
	if (!mLayeredGroup)
		return false;
	if (!ThePrefs->GetBool( "AvoidPrefsSanityChecks", false)) {
		for( int i=0; i<mLayeredGroup->CountChildren(); ++i) {
			BmPrefsView* pv = dynamic_cast<BmPrefsView*>( mLayeredGroup->ChildAt( i));
			if (pv && !pv->SanityCheck()) {
				ShowPrefs( i);
				return false;
			}
		}
	}
	for( int i=0; i<mLayeredGroup->CountChildren(); ++i) {
		BmPrefsView* pv = dynamic_cast<BmPrefsView*>( mLayeredGroup->ChildAt( i));
		if (pv)
			pv->SaveData();
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsViewContainer::Finish() {
	if (!mLayeredGroup)
		return;
	for( int i=0; i<mLayeredGroup->CountChildren(); ++i) {
		BmPrefsView* pv = dynamic_cast<BmPrefsView*>( mLayeredGroup->ChildAt( i));
		if (pv)
			pv->Finish();
	}
}
