/*
	BmPrefsMailReadView.cpp
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
#include <layout-all.h>

#include "Colors.h"

#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmPrefsMailReadView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsMailReadView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailReadView::BmPrefsMailReadView() 
	:	inherited( "Reading Mail")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Header Display",
				new VGroup(
					mHeaderListSmallControl = new BmTextControl( "Fields in 'small'-mode:"),
					mHeaderListLargeControl = new BmTextControl( "Fields in 'large'-mode:"),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Trusted Mimetypes (when opening attachments)",
				new VGroup(
					mMimeTypeTrustInfoControl = new BmTextControl( "Mimetype trust info:"),
					0
				)
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mHeaderListSmallControl->Divider();
	divider = MAX( divider, mHeaderListLargeControl->Divider());
	mHeaderListSmallControl->SetDivider( divider);
	mHeaderListLargeControl->SetDivider( divider);

	mHeaderListLargeControl->SetText( ThePrefs->GetString("HeaderListLarge").String());
	mHeaderListSmallControl->SetText( ThePrefs->GetString("HeaderListSmall").String());
	mMimeTypeTrustInfoControl->SetText( ThePrefs->GetString("MimeTypeTrustInfo").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailReadView::~BmPrefsMailReadView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::Initialize() {
	inherited::Initialize();

	mHeaderListSmallControl->SetTarget( this);
	mHeaderListLargeControl->SetTarget( this);
	mMimeTypeTrustInfoControl->SetTarget( this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::SaveData() {
	// prefs are already stored by General View
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::UndoChanges() {
	// prefs are already undone by General View
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( source == mHeaderListSmallControl)
					ThePrefs->SetString("HeaderListSmall", mHeaderListSmallControl->Text());
				else if ( source == mHeaderListLargeControl)
					ThePrefs->SetString("HeaderListLarge", mHeaderListLargeControl->Text());
				else if ( source == mMimeTypeTrustInfoControl)
					ThePrefs->SetString("MimeTypeTrustInfo", mMimeTypeTrustInfoControl->Text());
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}
