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
#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>


#include "Colors.h"
#include "BubbleHelper.h"

#include "BmBasics.h"
#include "BmCheckControl.h"
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
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Reading Mail Options",
				new VGroup(
					mMarkAsReadDelayControl = new BmTextControl( "Delay (in ms) before marking mails as read:"),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Network Options",
				new VGroup(
					mAutoCheckIfPppUpControl = new BmCheckControl( "Automatically check for mails only if PPP is up", 
																				  new BMessage(BM_CHECK_IF_PPP_UP_CHANGED), 
																				  this, ThePrefs->GetBool("AutoCheckOnlyIfPPPRunning")),
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
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailReadView::~BmPrefsMailReadView() {
	TheBubbleHelper->SetHelp( mHeaderListSmallControl, NULL);
	TheBubbleHelper->SetHelp( mHeaderListLargeControl, NULL);
	TheBubbleHelper->SetHelp( mMimeTypeTrustInfoControl, NULL);
	TheBubbleHelper->SetHelp( mMarkAsReadDelayControl, NULL);
	TheBubbleHelper->SetHelp( mAutoCheckIfPppUpControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( mHeaderListSmallControl, "Here you can enter the list of header-fields that will be displayed\nin the 'Small'-mode of the mailheader-view.\nJust enter the header-fields in the order you wish them to appear\nand separate them by a ',' (comma).");
	TheBubbleHelper->SetHelp( mHeaderListLargeControl, "Here you can enter the list of header-fields that will be displayed\nin the 'Large'-mode of the mailheader-view.\nJust enter the header-fields in the order you wish them to appear\nand separate them by a ',' (comma).");
	TheBubbleHelper->SetHelp( mMimeTypeTrustInfoControl, "When you double-click an attachment, Beam checks \n\
if the mimetype of the attachment can be trusted.\n\
Here you can define how Beam should treat different mimetypes.\n\
Each single entry is of the form: <mtSubString:action>\n\
where mtSubString is compared against the beginning of the \n\
attachment's mimetype. If the mimetype matches, the action given\n\
in that entry is executed:\n\
	T means 'trust', i.e. the attachment is immediately opened.\n\
	W means 'warn', i.e. Beam asks the user before opening the attachment.\n\
The order of the entries is important, so that in the default\n\
settings, the mimetype application/pdf has to be given trust before\n\
the mimetype application can be set to warn-mode.\n\n\
N.B.: I know this is clumsy and I promise that there will be\n\
something better in one of the next versions of Beam.");
	TheBubbleHelper->SetHelp( mMarkAsReadDelayControl, "When you select a new mail and it is displayed in the mail-view\n\
it will be marked as 'read', after a certain delay.\n\
You can enter this delay into this field.");
	TheBubbleHelper->SetHelp( mAutoCheckIfPppUpControl, "If you check this, automatical checks take place only if you\n\
have a running dialup-connection.\n\
If you have a permanent connection to the internet, you MUST\n\
uncheck this, otherwise no automatic checks will happen!");

	mHeaderListSmallControl->SetTarget( this);
	mHeaderListLargeControl->SetTarget( this);
	mMarkAsReadDelayControl->SetTarget( this);
	mMimeTypeTrustInfoControl->SetTarget( this);
	Update();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::Update() {
	mAutoCheckIfPppUpControl->SetValueSilently( ThePrefs->GetBool("AutoCheckOnlyIfPPPRunning"));
	BmString val;
	val << ThePrefs->GetInt("MarkAsReadDelay");
	mMarkAsReadDelayControl->SetTextSilently( val.String());
	mHeaderListLargeControl->SetTextSilently( ThePrefs->GetString("HeaderListLarge").String());
	mHeaderListSmallControl->SetTextSilently( ThePrefs->GetString("HeaderListSmall").String());
	mMimeTypeTrustInfoControl->SetTextSilently( ThePrefs->GetString("MimeTypeTrustInfo").String());
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
				else if ( source == mMarkAsReadDelayControl)
					ThePrefs->SetInt("MarkAsReadDelay", atoi( mMarkAsReadDelayControl->Text()));
				NoticeChange();
				break;
			}
			case BM_CHECK_IF_PPP_UP_CHANGED: {
				ThePrefs->SetBool("AutoCheckOnlyIfPPPRunning", mAutoCheckIfPppUpControl->Value());
				NoticeChange();
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}
