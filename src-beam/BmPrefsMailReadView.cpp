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
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <LayeredGroup.h>
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>


#include "Colors.h"
#include "BubbleHelper.h"

#include "BmBasics.h"
#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
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
			new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Display Options",
				new VGroup(
					mMarkAsReadDelayControl = new BmTextControl( 
						"Delay (in ms) before marking mails as read:"
					),
					new Space( minimax(0,5,0,5)),
					mHeaderListSmallControl = new BmTextControl( 
						"Fields in 'small'-header mode:"
					),
					mHeaderListLargeControl = new BmTextControl( 
						"Fields in 'large'-header mode:"
					),
					new Space( minimax(0,5,0,5)),
					new HGroup(
						mTimeModeInHeaderViewControl = new BmMenuControl( 
							"Time-Mode used in headerview:", new BPopUpMenu("")
						),
						new Space(),
						0
					),
					new Space( minimax(0,5,0,5)),
					mSelectNextOnDeleteControl = new BmCheckControl( 
						"Select next mail after deleting the current", 
						new BMessage(BM_SELECT_NEXT_ON_DELETE_CHANGED), 
						this, 
						ThePrefs->GetBool("SelectNextMailAfterDelete",true)
					),
					mUseSwatchTimeInRefViewControl = new BmCheckControl( 
						"Use Swatch Time in mail-listview", 
						new BMessage(BM_USE_SWATCHTIME_CHANGED), 
						this, 
						ThePrefs->GetBool("UseSwatchTimeInRefView",false)
					),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Attachment Options",
				new VGroup(
					mShowDecodedLengthControl = new BmCheckControl( 
						"Show Real (Decoded) Length", 
						new BMessage(BM_SHOW_DECODED_LENGTH_CHANGED), 
						this, ThePrefs->GetBool("ShowDecodedLength",true)
					),
					0
				)
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	DivideSame( 
		mHeaderListSmallControl,
		mHeaderListLargeControl,
		mMarkAsReadDelayControl,
		mTimeModeInHeaderViewControl,
		NULL
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailReadView::~BmPrefsMailReadView() {
	TheBubbleHelper->SetHelp( mHeaderListSmallControl, NULL);
	TheBubbleHelper->SetHelp( mHeaderListLargeControl, NULL);
	TheBubbleHelper->SetHelp( mMarkAsReadDelayControl, NULL);
	TheBubbleHelper->SetHelp( mUseSwatchTimeInRefViewControl, NULL);
	TheBubbleHelper->SetHelp( mSelectNextOnDeleteControl, NULL);
	TheBubbleHelper->SetHelp( mTimeModeInHeaderViewControl, NULL);
	TheBubbleHelper->SetHelp( mShowDecodedLengthControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mHeaderListSmallControl, 
		"Here you can enter the list of header-fields that will be displayed\n"
		"in the 'Small'-mode of the mailheader-view.\n"
		"Just enter the header-fields in the order you wish them to appear\n"
		"and separate them by a ',' (comma).\n"
		"If put a '/' (slash) between two fieldnames, the first field will\n"
		"be displayed, but if that is empty, the second will be used."
	);
	TheBubbleHelper->SetHelp( 
		mHeaderListLargeControl, 
		"Here you can enter the list of header-fields that will be displayed\n"
		"in the 'Large'-mode of the mailheader-view.\n"
		"Just enter the header-fields in the order you wish them to appear\n"
		"and separate them by a ',' (comma).\n"
		"If put a '/' (slash) between two fieldnames, the first field will\n"
		"be displayed, but if that is empty, the second will be used."
	);
	TheBubbleHelper->SetHelp( 
		mMarkAsReadDelayControl, 
		"When you select a new mail and it is displayed in the mail-view\n"
		"it will be marked as 'read', after a certain delay.\n"
		"You can enter the delay into this field."
	);
	TheBubbleHelper->SetHelp( 
		mSelectNextOnDeleteControl, 
		"If you check this, Beam will automatically select the next mail\n"
		"when you delete (trash) a mail.\n"
		"If you leave this unchecked, no mail will be selected when you\n"
		"delete the current mail."
	);
	TheBubbleHelper->SetHelp( 
		mUseSwatchTimeInRefViewControl, 
		"If you check this, the datetime-columns in the list of mails\n"
		"will be displayed in Swatch Time (0-999).\n"
		"If you leave this unchecked, time-columns will be\n"
		"displayed in local time."
	);
	TheBubbleHelper->SetHelp( 
		mTimeModeInHeaderViewControl, 
		"Here you can select the time-base that will be used\n"
		"for the 'Date:'-field when displaying the mail-header:\n"
		"    'Local':  means that the time will be converted into\n"
		"              the time-base that is used on your machine\n"
		"              (i.e. it will indicate the time from your\n"
		"              perspective)\n"
		"    'Native': means that the time will be displayed\n"
		"              just as it is contained in the mail-text\n"
		"              (i.e. it will indicate the time from the \n"
		"              perspective of the mail's sender).\n"
		"    'Swatch': means that the time will be converted into\n"
		"              Swatch Internet Time. This is a geeky time-base\n"
		"              measured in .beats (ranging from 0 to 999)."
	);
	TheBubbleHelper->SetHelp( 
		mShowDecodedLengthControl, 
		"If checked, Beam will display the real (decoded)\n"
		"length of each attachment, not the encoded length.\n"
		"Showing the real length takes slightly more time, but\n"
		"it avoids confusion about file-sizes."
	);

	mHeaderListSmallControl->SetTarget( this);
	mHeaderListLargeControl->SetTarget( this);
	mMarkAsReadDelayControl->SetTarget( this);

	// add time-modes:
	const char* timeModes[] = {
		"Local",
		"Native",
		"Swatch",
		NULL
	};
	for( int32 i=0; timeModes[i]; ++i) {
		BMessage* msg = new BMessage( BM_TIMEMODE_IN_HEADERVIEW_SELECTED);
		AddItemToMenu( 
			mTimeModeInHeaderViewControl->Menu(), 
			new BMenuItem( timeModes[i], msg), 
			this
		);
	}

	Update();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailReadView::Update() {
	mUseSwatchTimeInRefViewControl->SetValueSilently( 
		ThePrefs->GetBool("UseSwatchTimeInRefView",false)
	);
	mSelectNextOnDeleteControl->SetValueSilently( 
		ThePrefs->GetBool("SelectNextMailAfterDelete",true)
	);
	mShowDecodedLengthControl->SetValueSilently( 
		ThePrefs->GetBool("ShowDecodedLength",true)
	);
	BmString val;
	val << ThePrefs->GetInt("MarkAsReadDelay");
	mMarkAsReadDelayControl->SetTextSilently( val.String());
	mHeaderListLargeControl->SetTextSilently( 
		ThePrefs->GetString("HeaderListLarge").String()
	);
	mHeaderListSmallControl->SetTextSilently( 
		ThePrefs->GetString("HeaderListSmall").String()
	);
	mTimeModeInHeaderViewControl->MarkItem( 
		ThePrefs->GetString("TimeModeInHeaderView","local").String()
	);
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
					ThePrefs->SetString( 
						"HeaderListSmall", 
						mHeaderListSmallControl->Text()
					);
				else if ( source == mHeaderListLargeControl)
					ThePrefs->SetString(
						"HeaderListLarge", 
						mHeaderListLargeControl->Text()
					);
				else if ( source == mMarkAsReadDelayControl)
					ThePrefs->SetInt(
						"MarkAsReadDelay", 
						atoi( mMarkAsReadDelayControl->Text())
					);
				NoticeChange();
				break;
			}
			case BM_USE_SWATCHTIME_CHANGED: {
				ThePrefs->SetBool(
					"UseSwatchTimeInRefView", 
					mUseSwatchTimeInRefViewControl->Value()
				);
				NoticeChange();
				break;
			}
			case BM_SELECT_NEXT_ON_DELETE_CHANGED: {
				ThePrefs->SetBool(
					"SelectNextMailAfterDelete", 
					mSelectNextOnDeleteControl->Value()
				);
				NoticeChange();
				break;
			}
			case BM_SHOW_DECODED_LENGTH_CHANGED: {
				ThePrefs->SetBool(
					"ShowDecodedLength", 
					mShowDecodedLengthControl->Value()
				);
				NoticeChange();
				break;
			}
			case BM_TIMEMODE_IN_HEADERVIEW_SELECTED: {
				BMenuItem* item 
					= mTimeModeInHeaderViewControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetString( "TimeModeInHeaderView", item->Label());
					NoticeChange();
				}
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
