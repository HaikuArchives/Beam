/*
	BmPrefsLoggingView.cpp
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
#include "BmMenuControl.h"
#include "BmPrefs.h"
#include "BmPrefsLoggingView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsLoggingView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsLoggingView::BmPrefsLoggingView() 
	:	inherited( "Logging Messages")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Levels of Logging",
					new VGroup(
						mLogPopControl = new BmMenuControl( "Receiving Mail (POP3):", 
																		new BPopUpMenu("")),
						mLogSmtpControl = new BmMenuControl( "Sending Mail (SMTP):", 
																		 new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogFilterControl = new BmMenuControl( "Filtering Mails:", 
																			new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogAppControl = new BmMenuControl( "Application State:", 
																		new BPopUpMenu("")),
						new Space(),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Levels of Debug-Logging",
					new VGroup(
						mLogMailTrackingControl 
							= new BmMenuControl( "Tracking Mails:", 
														new BPopUpMenu("")),
						mLogMailParseControl = new BmMenuControl( "Parsing Mails:", 
																				new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogJobWinControl = new BmMenuControl( "Status Window:", 
																			new BPopUpMenu("")),
						mLogGuiControl = new BmMenuControl( "General GUI:", 
																		new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogModelControllerControl 
							= new BmMenuControl( "Info-Transport:", 
														new BPopUpMenu("")),
						mLogRefCountControl = new BmMenuControl( "Memory:", 
																			  new BPopUpMenu("")),
						0
					)
				),
				new Space(),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Logfile Sizes",
					new VGroup(
						mMaxLogfileSizeControl 
							= new BmTextControl( "Maximum size for logfile (KB):", 
														false, 8),
						mMinLogfileSizeControl 
							= new BmTextControl( "Minimum size for logfile (KB):", 
														false, 8),
						0
					)
				),
				new Space(),
				0
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mLogPopControl->Divider();
	divider = MAX( divider, mLogSmtpControl->Divider());
	divider = MAX( divider, mLogAppControl->Divider());
	divider = MAX( divider, mLogFilterControl->Divider());
	mLogPopControl->SetDivider( divider);
	mLogSmtpControl->SetDivider( divider);
	mLogAppControl->SetDivider( divider);
	mLogFilterControl->SetDivider( divider);

	divider = mLogMailParseControl->Divider();
	divider = MAX( divider, mLogMailTrackingControl->Divider());
	divider = MAX( divider, mLogRefCountControl->Divider());
	divider = MAX( divider, mLogJobWinControl->Divider());
	divider = MAX( divider, mLogGuiControl->Divider());
	divider = MAX( divider, mLogModelControllerControl->Divider());
	mLogMailParseControl->SetDivider( divider);
	mLogMailTrackingControl->SetDivider( divider);
	mLogRefCountControl->SetDivider( divider);
	mLogJobWinControl->SetDivider( divider);
	mLogGuiControl->SetDivider( divider);
	mLogModelControllerControl->SetDivider( divider);

	divider = mMaxLogfileSizeControl->Divider();
	divider = MAX( divider, mMinLogfileSizeControl->Divider());
	mMaxLogfileSizeControl->SetDivider( divider);
	mMinLogfileSizeControl->SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsLoggingView::~BmPrefsLoggingView() {
	TheBubbleHelper->SetHelp( mLogPopControl, NULL);
	TheBubbleHelper->SetHelp( mLogSmtpControl, NULL);
	TheBubbleHelper->SetHelp( mLogAppControl, NULL);
	TheBubbleHelper->SetHelp( mLogFilterControl, NULL);
	TheBubbleHelper->SetHelp( mLogMailParseControl, NULL);
	TheBubbleHelper->SetHelp( mLogMailTrackingControl, NULL);
	TheBubbleHelper->SetHelp( mLogGuiControl, NULL);
	TheBubbleHelper->SetHelp( mLogJobWinControl, NULL);
	TheBubbleHelper->SetHelp( mLogModelControllerControl, NULL);
	TheBubbleHelper->SetHelp( mLogRefCountControl, NULL);
	TheBubbleHelper->SetHelp( mMinLogfileSizeControl, NULL);
	TheBubbleHelper->SetHelp( mMaxLogfileSizeControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsLoggingView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mLogPopControl, 
		"Here you can select how much shall be logged when receiving mails.\n"
		"If you encounter problems with one of your POP-accounts,\n"
		"you can increase this setting in order to get more info.\n"
		"Please note that logging everything here means that complete\n"
		"e-mails will be logged, so this can produce a lot of data."
	);
	TheBubbleHelper->SetHelp( 
		mLogSmtpControl, 
		"Here you can select how much shall be logged when sending mails.\n"
		"If you encounter problems with one of your SMTP-accounts,\n"
		"you can increase this setting in order to get more info.\n"
		"Please note that logging everything here means that complete\n"
		"e-mails will be logged, so this can produce a lot of data."
	);
	TheBubbleHelper->SetHelp( 
		mLogAppControl, 
		"This logging-terrain concerns the general state of\n"
		"the application.\n"
		"Startup- and shutdown-times are shown in 'Log'-mode."
	);
	TheBubbleHelper->SetHelp( 
		mLogFilterControl, 
		"Here you can select how much will be logged\n"
		"when filtering mails.\n"
		"'Log'-mode will log executed actions only, while\n"
		"'Log More'-mode will give additional info about\n"
		"which filters are being executed.\n"
		"If you have problems getting a specific filter to\n"
		"work properly, increasing this setting might help."
	);
	TheBubbleHelper->SetHelp( 
		mLogMailParseControl, 
		"This is for debugging only!\n"
		"If you think Beam is displaying a specific mail\n"
		"incorrectly, increasing this setting may help to\n"
		"find out what is going wrong."
	);
	TheBubbleHelper->SetHelp( 
		mLogMailTrackingControl, 
		"This is for debugging only!\n"
		"If you encounter a problem when tracking mails,\n"
		"increasing this setting may help to find out\n"
		"what is going wrong."
	);
	TheBubbleHelper->SetHelp( 
		mLogGuiControl, 
		"This is for debugging only!\n"
		"If there seems to be a problem with the window-layout\n"
		"or any other GUI-related issue, increasing this setting\n"
		"may help to find out what is going wrong."
	);
	TheBubbleHelper->SetHelp( 
		mLogJobWinControl, 
		"This is for debugging only!\n"
		"If there seems to be a problem related to\n"
		"the status-window, increasing this setting\n"
		"may help to find out what is going wrong."
	);
	TheBubbleHelper->SetHelp( 
		mLogModelControllerControl, 
		"This is for debugging only!\n"
		"This setting concerns the internal passing of information.\n"
		"So if you think Beam is not reacting on state changes properly\n"
		"(like when you move mails or switch to another folder),\n"
		"increasing this setting may help to find out what is going wrong."
	);
	TheBubbleHelper->SetHelp( 
		mLogRefCountControl, 
		"This is for debugging only!\n"
		"Increasing this setting will log more details about\n"
		"the memory allocation/deallocation that is going on.\n"
		"This may help if Beam seems to be leaking memory or\n"
		"is crashing even (gasp!)."
	);
	const char* t 
		= "With minimum and maximum size, you can control the size of logfiles.\n"
		  "When Beam starts, every logfile is checked against the maximum size\n"
		  "and if it exceeds that size, the file is cut back (from the front)\n"
		  "to the given minium size.";
	TheBubbleHelper->SetHelp( mMaxLogfileSizeControl, t);
	TheBubbleHelper->SetHelp( mMinLogfileSizeControl, t);

	mMaxLogfileSizeControl->SetTarget( this);
	mMinLogfileSizeControl->SetTarget( this);

	// add log-levels:
	const char* logModes[] = {
		BmPrefs::LOG_LVL_0,
		BmPrefs::LOG_LVL_1,
		BmPrefs::LOG_LVL_2,
		BmPrefs::LOG_LVL_3,
		NULL
	};
	for( int32 i=0; logModes[i]; ++i) {
		AddItemToMenu( mLogPopControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_POP_SELECTED)),
							this);
		AddItemToMenu( mLogSmtpControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_SMTP_SELECTED)),
							this);
		AddItemToMenu( mLogAppControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_APP_SELECTED)),
							this);
		AddItemToMenu( mLogFilterControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_FILTER_SELECTED)),
							this);
		AddItemToMenu( mLogMailParseControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_MAILPARSE_SELECTED)),
							this);
		AddItemToMenu( 
			mLogMailTrackingControl->Menu(), 
				new BMenuItem( logModes[i], 
									new BMessage( BM_LOG_MAILTRACKING_SELECTED)),
				this
		);
		AddItemToMenu( mLogGuiControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_GUI_SELECTED)),
							this);
		AddItemToMenu( mLogJobWinControl->Menu(), 
							new BMenuItem( logModes[i],
												new BMessage( BM_LOG_JOBWIN_SELECTED)),
							this);
		AddItemToMenu( 
			mLogModelControllerControl->Menu(), 
				new BMenuItem( logModes[i], 
									new BMessage( BM_LOG_MODELCONTROLLER_SELECTED)),
				this
			);
		AddItemToMenu( mLogRefCountControl->Menu(), 
							new BMenuItem( logModes[i], 
												new BMessage( BM_LOG_REFCOUNT_SELECTED)),
							this);
	}

	Update();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsLoggingView::Update() {
	BmString val;
	val << static_cast<int>( ThePrefs->GetInt("MinLogfileSize",50*1024)/1024);
	mMinLogfileSizeControl->SetTextSilently( val.String());
	val = "";
	val << static_cast<int>( ThePrefs->GetInt("MaxLogfileSize",200*1024)/1024);
	mMaxLogfileSizeControl->SetTextSilently( val.String());
	mLogPopControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogPop));
	mLogSmtpControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogSmtp));
	mLogFilterControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogFilter));
	mLogAppControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogApp));
	mLogMailParseControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogMailParse));
	mLogMailTrackingControl->MarkItem( 
		ThePrefs->GetLogLevelFor( BM_LogMailTracking));
	mLogJobWinControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogJobWin));
	mLogGuiControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogGui));
	mLogModelControllerControl->MarkItem( 
		ThePrefs->GetLogLevelFor( BM_LogModelController));
	mLogRefCountControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogRefCount));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsLoggingView::SaveData() {
	// prefs are already stored by General View
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsLoggingView::UndoChanges() {
	// prefs are already undone by General View
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsLoggingView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( source == mMinLogfileSizeControl)
					ThePrefs->SetInt( "MinLogfileSize", 
										   atoi(mMinLogfileSizeControl->Text())*1024);
				else if ( source == mMaxLogfileSizeControl)
					ThePrefs->SetInt( "MaxLogfileSize", 
										   atoi(mMaxLogfileSizeControl->Text())*1024);
				NoticeChange();
				break;
			}
			case BM_LOG_POP_SELECTED: {
				BMenuItem* item = mLogPopControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogPop, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_SMTP_SELECTED: {
				BMenuItem* item = mLogSmtpControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogSmtp, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_FILTER_SELECTED: {
				BMenuItem* item = mLogFilterControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogFilter, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_APP_SELECTED: {
				BMenuItem* item = mLogAppControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogApp, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_MAILPARSE_SELECTED: {
				BMenuItem* item = mLogMailParseControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogMailParse, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_MAILTRACKING_SELECTED: {
				BMenuItem* item = mLogMailTrackingControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogMailTracking, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_JOBWIN_SELECTED: {
				BMenuItem* item = mLogJobWinControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogJobWin, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_GUI_SELECTED: {
				BMenuItem* item = mLogGuiControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogGui, item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_MODELCONTROLLER_SELECTED: {
				BMenuItem* item = mLogModelControllerControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogModelController, 
														 item->Label());
					NoticeChange();
				}
			}
			case BM_LOG_REFCOUNT_SELECTED: {
				BMenuItem* item = mLogRefCountControl->Menu()->FindMarked();
				if (item) {
					ThePrefs->SetLogLevelForTo( BM_LogRefCount, item->Label());
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
