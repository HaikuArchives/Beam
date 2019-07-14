/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
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
	:	inherited( "Logging messages")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Levels of logging",
					new VGroup(
						mLogPopControl = new BmMenuControl( "Receiving mail:", 
																		new BPopUpMenu(""), 
																		1.0, 0,
																		BmPrefs::LOG_LVL_3),
						mLogSmtpControl = new BmMenuControl( "Sending mail:", 
																		 new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogFilterControl = new BmMenuControl( "Filtering mails:", 
																			new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogAppControl = new BmMenuControl( "Application state:", 
																		new BPopUpMenu("")),
						new Space(),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Levels of debug logging",
					new VGroup(
						mLogMailTrackingControl 
							= new BmMenuControl( "Tracking mails:", 
														new BPopUpMenu(""),
														1.0, 0,
														BmPrefs::LOG_LVL_3),
						mLogMailParseControl = new BmMenuControl( "Parsing mails:", 
																				new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogJobWinControl = new BmMenuControl( "Status window:", 
																			new BPopUpMenu("")),
						mLogGuiControl = new BmMenuControl( "General GUI:", 
																		new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mLogModelControllerControl 
							= new BmMenuControl( "Info transport:", 
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
				new MBorder( M_LABELED_BORDER, 10, (char*)"Logfile sizes",
					new VGroup(
						mMaxLogfileSizeControl 
							= new BmTextControl( "Maximum size of logfile (KB):", 
														false, 8),
						mMinLogfileSizeControl 
							= new BmTextControl( "Minimum size of logfile (KB):", 
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
	
	BmDividable::DivideSame(
		mLogPopControl,
		mLogSmtpControl,
		mLogAppControl,
		mLogFilterControl,
		NULL
	);

	BmDividable::DivideSame(
		mLogMailParseControl,
		mLogMailTrackingControl,
		mLogRefCountControl,
		mLogJobWinControl,
		mLogGuiControl,
		mLogModelControllerControl,
		NULL
	);

	BmDividable::DivideSame(
		mMaxLogfileSizeControl,
		mMinLogfileSizeControl,
		NULL
	);
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
		"If you encounter problems with one of your POP/IMAP-accounts,\n"
		"you can increase this setting in order to get more info.\n"
		"Please note that logging everything here means that complete\n"
		"e-mails will be logged, so this can produce a lot of data."
	);
	TheBubbleHelper->SetHelp( 
		mLogSmtpControl, 
		"Here you can select how much shall be logged when sending mails.\n"
		"If you encounter problems with one of your SMTP accounts,\n"
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
		"'Log more'-mode will give additional info about\n"
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
	mLogPopControl->MarkItem( ThePrefs->GetLogLevelFor( BM_LogRecv));
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
					ThePrefs->SetLogLevelForTo( BM_LogRecv, item->Label());
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
