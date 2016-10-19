/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <algorithm>

#include <Alert.h>
#include <Beep.h>
#include <Deskbar.h>
#include <Roster.h>
#include <Screen.h>

#include "regexx.hh"
using namespace regexx;

#include "BubbleHelper.h"
#include "ImageAboutWindow.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmBusyView.h"
#include "BmDataModel.h"
#include "BmDeskbarView.h"
#include "BmEncoding.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmGuiRoster.h"
#include "BmIdentity.h"
#include "BmImapAccount.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFactory.h"
#include "BmMailFolderList.h"
#include "BmMailMonitor.h"
#include "BmMailMover.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmMainWindow.h"
#include "BmMsgTypes.h"
#include "BmNetUtil.h"
#include "BmPeople.h"
#include "BmRecvAccount.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmPrefsWin.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmSlaveHandler
		-	
\*------------------------------------------------------------------------------*/
class BmSlaveHandler {
public:
	BmSlaveHandler();
	~BmSlaveHandler();
	void Run( const char* name, thread_func func, BMessage* msg, 
				 uint16 minCountForThread=10);
private:
	int32 mSlaveNum;
};

BmSlaveHandler::BmSlaveHandler()
	:	mSlaveNum( 1)
{
}

BmSlaveHandler::~BmSlaveHandler()
{
}

void BmSlaveHandler::Run( const char* name, thread_func func, 
								  BMessage* msg, uint16 minCountForThread)
{
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	if (!refVect || refVect->size() <= minCountForThread) {
		// execute task in app-thread:
		func(msg);
	} else {
		// start new thread for task:
		BmString tname( name);
		tname << "(" << mSlaveNum++ << ")";
		thread_id t_id = spawn_thread( func, tname.String(), 
												 B_NORMAL_PRIORITY, msg);
		if (t_id < 0)
			throw BM_runtime_error("SlaveHandler::Run(): Could not spawn thread");
		resume_thread( t_id);
	}
}

static BmSlaveHandler SlaveHandler;




/*------------------------------------------------------------------------------*\
	MarkMailsAs( msg)
		-	sets status for all mailref's contained in the given message.
		-	this is a thread-entry func.
\*------------------------------------------------------------------------------*/
static int32 MarkMailsAs( void* data)
{
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	int32 buttonPressed = 1;
		// mark all messages by default
	BmString newStatus = msg->FindString( BeamApplication::MSG_STATUS);
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount>1) {
		// first step, check if user has asked to mark as read and 
		// has selected messages with advanced statii (like replied
		// or forwarded). If so,  we ask the user about how to 
		// handle those:
		if (newStatus == BM_MAIL_STATUS_READ) {
			bool hasAdvanced = false;
			BmMailRef* mailRef;
			BmMailRefVect::iterator iter;
			for( 
				iter = refVect->begin(); iter != refVect->end(); ++iter
			) {
				mailRef = iter->Get();
				if (mailRef->Status() != BM_MAIL_STATUS_NEW
				&&  mailRef->Status() != BM_MAIL_STATUS_READ) {
					hasAdvanced = true;
					break;
				}
			}
			if (hasAdvanced) {
				BmString s("Some of the selected messages are not new."
							  "\n\nShould Beam mark only the new messages "
							  "as being read?");
				BAlert* alert = new BAlert( 
					"Set Mail Status", s.String(), "Cancel", 
					"Set Status of All Messages", "Set Status of New Messages Only", 
					B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_IDEA_ALERT
				);
				alert->SetShortcut( 0, B_ESCAPE);
				buttonPressed = alert->Go();
			}
		}
	}
	if (buttonPressed != 0) {
		// tell sending ref-view that we are doing work:
		BMessenger sendingRefView;
		msg->FindMessenger( BeamApplication::MSG_SENDING_REFVIEW, &sendingRefView);
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_SET_BUSY);

		BmMailRef* mailRef;
		int i=0;
		BmMailRefVect::iterator iter;
		for( iter = refVect->begin(); 
			  !beamApp->IsQuitting() && iter != refVect->end(); ++iter) {
			mailRef = iter->Get();
			if (buttonPressed==1 || mailRef->Status() == BM_MAIL_STATUS_NEW) {
				BM_LOG( BM_LogApp, 
						  BmString("marking mail <") << mailRef->TrackerName()
						  		<< "> as " << newStatus);
				mailRef->MarkAs( newStatus.String());
				if (++i % 100 == 0)
					snooze(500*1000);
						// give mail monitor a chance to catch up...
			}
		}
		// now tell sending ref-view that we are finished:
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_UNSET_BUSY);
	}
	delete refVect;
				// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	MoveMails( msg)
		-	moves mails to a new folder.
		-	this is a thread-entry func.
\*------------------------------------------------------------------------------*/
static int32 MoveMails( void* data)
{
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	static int jobNum = 1;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 countFound = 0;
	if (refVect)
		countFound = refVect->size();
	if (countFound > 0) {
		// tell sending ref-view that we are doing work:
		BMessenger sendingRefView;
		msg->FindMessenger( BeamApplication::MSG_SENDING_REFVIEW, &sendingRefView);
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_SET_BUSY);

		BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
		BmString folderName = msg->FindString( BmListModel::MSG_ITEMKEY);
		BmRef<BmListModelItem> itemRef 
			= TheMailFolderList->FindItemByKey( folderName);
		BmMailFolder* folder = dynamic_cast< BmMailFolder*>( itemRef.Get());
		if (!folder)
			return B_OK;
		BmString jobName = folder->DisplayKey();
		jobName << jobNum++;
		tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
		tmpMsg.AddString( BmJobModel::MSG_MODEL, folder->Key().String());
		// now add a pointer to an array of entry_refs to msg:
		struct entry_ref *refs = new entry_ref [countFound];
		BmMailRef* mailRef;
		BmMailRefVect::iterator iter;
		int32 index=0;
		for( iter = refVect->begin(); 
			  !beamApp->IsQuitting() && iter != refVect->end(); ++iter) {
			mailRef = iter->Get();
			BM_LOG( 
				BM_LogApp, 
				BmString("Asked to move mail <") 
					<< mailRef->TrackerName() << "> to folder <"
					<< folder->DisplayKey() << ">"
			);
			refs[index++] = mailRef->EntryRef();
		}
		tmpMsg.AddPointer( BmMailMover::MSG_REFS, (void*)refs);
		tmpMsg.AddInt32( BmMailMover::MSG_REF_COUNT, countFound);
				// message takes ownership of refs-array!
		TheJobStatusWin->PostMessage( &tmpMsg);
		// now tell sending ref-view that we are finished:
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_UNSET_BUSY);
	}
	delete refVect;
				// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	TrashMails( msg)
		-	moves mails to trash.
		-	this is a thread-entry func.
\*------------------------------------------------------------------------------*/
static int32 TrashMails( void* data)
{
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	uint32 countFound = 0;
	if (refVect)
		countFound = refVect->size();
	if (countFound>0) {
		BM_LOG( BM_LogApp, 
				  BmString("Asked to trash ") << countFound << " mails");
		struct entry_ref *refs = new entry_ref [countFound];

		// tell sending ref-view that we are doing work:
		BMessenger sendingRefView;
		msg->FindMessenger( BeamApplication::MSG_SENDING_REFVIEW, &sendingRefView);
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_SET_BUSY);

		BmMailRef* mailRef;
		BmMailRefVect::iterator iter;
		int32 index=0;
		for( iter = refVect->begin(); 
			  !beamApp->IsQuitting() && iter != refVect->end(); ++iter) {
			mailRef = iter->Get();
			refs[index++] = mailRef->EntryRef();
		}
		if (!beamApp->IsQuitting())
			MoveToTrash( refs, index);
		delete [] refs;

		// now tell sending ref-view that we are finished:
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_UNSET_BUSY);
	}
	delete refVect;
				// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

struct OpenForEdit {
	void operator() ( const BmRef<BmMail>& mail) 
	{
		if (!beamApp->IsQuitting()) {
			BmMailEditWin* editWin 
				= BmMailEditWin::CreateInstance( mail.Get());
			if (editWin)
				editWin->Show();
		}
	}
};

/*------------------------------------------------------------------------------*\
	CreateMailsWithFactory( msg, factory)
		-	handles a request to create new mails with given factory
\*------------------------------------------------------------------------------*/
static void CreateMailsWithFactory( BMessage* msg, BmMailRefVect* refVect, 
												BmMailFactory* factory)
{
	if (!msg || !refVect || !factory)
		return;

	// tell sending ref-view that we are doing work:
	BMessenger sendingRefView;
	msg->FindMessenger( BeamApplication::MSG_SENDING_REFVIEW, &sendingRefView);
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_SET_BUSY);

	for( uint32 index=0; index < refVect->size(); ++index) {
		factory->AddBaseMailRef((*refVect)[index].Get());
	}
	factory->Produce();

	// ...and open all created copies:
	for_each( factory->TheMails.begin(), 
				 factory->TheMails.end(), 
				 OpenForEdit());

	// now tell sending ref-view that we are finished:
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_UNSET_BUSY);
}

/*------------------------------------------------------------------------------*\
	EditMailsAsNew( msg)
		-	creates mails from given set (copies them and starts edit).
		-	this is a thread-entry func.
\*------------------------------------------------------------------------------*/
static int32 EditMailsAsNew( void* data)
{
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount) {
		BM_LOG( BM_LogApp, 
				  BmString("Asked to edit ") << msgCount << " mails as new.");
		BmCopyMailFactory factory;	
		CreateMailsWithFactory( msg, refVect, &factory);
	}
	delete refVect;
		// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	RedirectMails( msg)
		-	redirects given mails.
		-	this is a thread-entry func.
\*------------------------------------------------------------------------------*/
static int32 RedirectMails( void* data)
{
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount) {
		BM_LOG( BM_LogApp, 
				  BmString("Asked to redirect ") << msgCount << " mails.");
		BmRedirectFactory factory;	
		CreateMailsWithFactory( msg, refVect, &factory);
	}
	delete refVect;
		// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	ForwardMails( msg, join)
		-	forwards all mailref's contained in the given message.
		-	depending on the param join, multiple mails are joined into one single 
			forward or one forward is generated for each mail.
\*------------------------------------------------------------------------------*/
static int32 ForwardMails( void* data) {
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;

	int32 buttonPressed = 1;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount>1) {
		// first step, ask user about how to forward multiple messages:
		BmString s("You have selected more than one message.\n\n"
					  "Should Beam join the message-bodies into one "
					  "single mail and forward that or would you prefer "
					  "to keep the messages separate?");
		BAlert* alert = new BAlert( 
			"Forwarding Multiple Mails", s.String(),
		 	"Cancel", "Keep Separate", "Join", 
		 	B_WIDTH_AS_USUAL,	B_OFFSET_SPACING, B_IDEA_ALERT
		);
		alert->SetShortcut( 0, B_ESCAPE);
		buttonPressed = alert->Go();
	}
	if (buttonPressed > 0) {
		BM_LOG( BM_LogApp, 
			BmString("Asked to forward ") << msgCount << " mails.");
	
		bool join = (buttonPressed == 2);
		BmString selectedText 
			= msg->FindString( BeamApplication::MSG_SELECTED_TEXT);

		BmForwardMode forwardMode;
		switch(msg->what) {
			case BMM_FORWARD_INLINE_ATTACH:
				forwardMode = BM_FORWARD_MODE_INLINE_ATTACH;
				break;
			case BMM_FORWARD_ATTACHED:
				forwardMode = BM_FORWARD_MODE_ATTACHED;
				break;
			default: // BMM_FORWARD_INLINE:
				forwardMode = BM_FORWARD_MODE_INLINE;
				break;
		};
		BmForwardFactory factory( forwardMode, join, selectedText);
		CreateMailsWithFactory( msg, refVect, &factory);
	}
	delete refVect;
		// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	ReplyToMails( msg, join)
		-	replies to all mailref's contained in the given message.
\*------------------------------------------------------------------------------*/
static int32 ReplyToMails( void* data) {
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;

	int32 buttonPressed = 0;
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	int32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount>1) {
		// first step, ask user about how to forward multiple messages:
		BmString s("You have selected more than one message.\n\n"
					  "Should Beam join the message-bodies into one "
					  "single mail and reply to that or would you prefer "
					  "to keep the messages separate?");
		BAlert* alert = new BAlert( 
			"Replying to Multiple Mails", s.String(),
			"Keep Separate", "Join per Recipient", "Join All", 
			B_WIDTH_AS_USUAL,	B_EVEN_SPACING, B_IDEA_ALERT
		);
		buttonPressed = alert->Go();
	}
	if (buttonPressed > -1) {
		BM_LOG( BM_LogApp, 
				  BmString("Asked to reply to ") << msgCount << " mails.");
	
		bool join = (buttonPressed > 0);
		bool joinIntoOne = (buttonPressed == 2);
		BmString selectedText 
			= msg->FindString( BeamApplication::MSG_SELECTED_TEXT);
	
		BmReplyMode replyMode;
		switch(msg->what) {
			case BMM_REPLY_LIST:
				replyMode = BM_REPLY_MODE_LIST;
				break;
			case BMM_REPLY_ORIGINATOR:
				replyMode = BM_REPLY_MODE_PERSON;
				break;
			case BMM_REPLY_ALL:
				replyMode = BM_REPLY_MODE_ALL;
				break;
			default: // BMM_REPLY:
				replyMode = BM_REPLY_MODE_SMART;
				break;
		};
		BmReplyFactory factory( replyMode, join, joinIntoOne, selectedText);
		CreateMailsWithFactory( msg, refVect, &factory);
	}
	delete refVect;
		// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	PrintMails( msg)
		-	prints all mailref's contained in the given message.
\*------------------------------------------------------------------------------*/
int32 PrintMails( void* data) {
	BMessage* msg = static_cast< BMessage*>( data);
	if (!msg)
		return B_OK;
	if (!beamApp->mPrintSetup)
		beamApp->PageSetup();
	if (!beamApp->mPrintSetup || !msg)
		return B_OK;

	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	uint32 msgCount = 0;
	if (refVect)
		msgCount = refVect->size();
	if (msgCount) {
		// tell sending ref-view that we are doing work:
		BMessenger sendingRefView;
		msg->FindMessenger( BeamApplication::MSG_SENDING_REFVIEW, &sendingRefView);
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_SET_BUSY);
	
		beamApp->mPrintJob.SetSettings( new BMessage( *beamApp->mPrintSetup));
		status_t result = beamApp->mPrintJob.ConfigJob();
		if (result == B_OK) {
			delete beamApp->mPrintSetup;
			beamApp->mPrintSetup = beamApp->mPrintJob.Settings();
			int32 firstPage = beamApp->mPrintJob.FirstPage();
			int32 lastPage = beamApp->mPrintJob.LastPage();
			if (lastPage-firstPage+1 <= 0)
				goto out;
			BmMailRef* mailRef = NULL;
			// we create a hidden mail-view-window which is being used for printing:
			BmMailViewWin* mailWin = BmMailViewWin::CreateInstance();
			mailWin->Hide();
			mailWin->Show();
			BmMailView* mailView = mailWin->MailView();
			// now get printable rect...
			BRect printableRect = beamApp->mPrintJob.PrintableRect();
			// ...and adjust mailview accordingly (to use the available space
			// effectively):
			if (mailView->LockLooper()) {
				mailWin->ResizeTo( printableRect.Width()+8, 600);
				mailView->UnlockLooper();
			}
			mailView->BodyPartView()->IsUsedForPrinting( true);
			// now we start printing...
			beamApp->mPrintJob.BeginJob();
			int32 page = 1;
			for(  uint32 mailIdx=0; 
					!beamApp->IsQuitting() && mailIdx < msgCount 
						&& page <= lastPage;
					++mailIdx) {
				mailRef = (*refVect)[mailIdx].Get();
				if (mailView->LockLooper()) {
					mailView->BodyPartView()->SetViewColor( 
						ui_color( B_DOCUMENT_BACKGROUND_COLOR)
					);
					mailView->ShowMail( mailRef, false);
					mailView->UnlockLooper();
				}
				while( !mailView->IsDisplayComplete())
					snooze( 50*1000);
				BRect currFrame = printableRect.OffsetToCopy( 0, 0);
				BRect textRect = mailView->TextRect();
				float totalHeight = textRect.top+mailView->TextHeight(0,100000);
				float height = currFrame.Height();
				BPoint topOfLine 
					= mailView->PointAt( mailView->OffsetAt( BPoint( 
						5, currFrame.bottom
					)));
				currFrame.bottom = topOfLine.y-1;
				while( !beamApp->IsQuitting() && page<=lastPage) {
					if (page >= firstPage) {
						beamApp->mPrintJob.DrawView( mailView, currFrame, BPoint(0,0));
						beamApp->mPrintJob.SpoolPage();
					}
					currFrame.top = currFrame.bottom+1;
					currFrame.bottom = currFrame.top + height-1;
					if (mailView->LockLooper()) {
						topOfLine 
							= mailView->PointAt( mailView->OffsetAt( BPoint( 
								5, currFrame.bottom
							)));
						mailView->UnlockLooper();
					}
					currFrame.bottom = topOfLine.y-1;
					page++;
					if (currFrame.top >= totalHeight || currFrame.Height() <= 1)
						// end of current mail reached
						break;
				}
			}
			beamApp->mPrintJob.CommitJob();
			mailWin->PostMessage( B_QUIT_REQUESTED);
		}
		// now tell sending ref-view that we are finished:
		if (sendingRefView.IsValid())
			sendingRefView.SendMessage( BMM_UNSET_BUSY);
	}
out:
	delete refVect;
			// freeing all references to mailrefs contained in vector
	delete msg;
	return B_OK;
}



BeamApplication* beamApp = NULL;

static const char* BM_BEEP_EVENT = "New E-mail";

const char* BM_APP_SIG = "application/x-vnd.zooey-beam";
const char* BM_TEST_APP_SIG = "application/x-vnd.zooey-testbeam";
const char* const BM_DeskbarItemName = "Beam_DeskbarItem";

const char* const BeamApplication::MSG_MAILREF_VECT = "bm:mrefv";
const char* const BeamApplication::MSG_STATUS = 		"bm:status";
const char* const BeamApplication::MSG_WHO_TO = 		"bm:to";
const char* const BeamApplication::MSG_OPT_FIELD = 	"bm:optf";
const char* const BeamApplication::MSG_OPT_VALUE = 	"bm:optv";
const char* const BeamApplication::MSG_SUBJECT = 		"bm:subj";
const char* const BeamApplication::MSG_SELECTED_TEXT = 	"bm:seltext";
const char* const BeamApplication::MSG_SENDING_REFVIEW = 	"bm:srefv";
const char* const BeamApplication::MSG_ENCLOSE = 		"bm:encl";


/*------------------------------------------------------------------------------*\
	BeamApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BeamApplication::BeamApplication( const char* sig)
	:	inherited( sig, false)
	,	mInitCheck( B_NO_INIT)
	,	mMailWin( NULL)
	,	mPrintSetup( NULL)
	,	mPrintJob( "Mail")
	,	mDeskbarItemIsOurs( false)
{
	beamApp = this;
	
	try {
		// create the GUI-info-roster:
		BeamGuiRoster = new BmGuiRoster();

		// load/determine all needed resources:
		BmResources::CreateInstance();
		TheResources->InitializeWithPrefs();

		ColumnListView::SetExtendedSelectionPolicy( 
									ThePrefs->GetBool( "ListviewLikeTracker", false));
							// make sure this actually get's initialized...

		// create BubbleHelper:
		BubbleHelper::CreateInstance();
		BmBusyView::SetErrorIcon( TheResources->IconByName("Error"));

		// init charset-tables:
		BmEncoding::InitCharsetMap();

		BM_LOG( BM_LogApp, BmString("...setting up foreign-keys..."));
		// now setup all foreign-key connections between these list-models:
		TheRecvAccountList->AddForeignKey( BmIdentity::MSG_RECV_ACCOUNT, 
													  TheIdentityList.Get());
		TheSmtpAccountList->AddForeignKey( BmIdentity::MSG_SMTP_ACCOUNT, 
													  TheIdentityList.Get());
		TheSignatureList->AddForeignKey( BmIdentity::MSG_SIGNATURE_NAME,
													TheIdentityList.Get());
		TheFilterChainList->AddForeignKey( BmRecvAccount::MSG_FILTER_CHAIN,
													  TheRecvAccountList.Get());
		TheRecvAccountList->AddForeignKey( BmSmtpAccount::MSG_ACC_FOR_SAP,
													  TheSmtpAccountList.Get());
		TheMailFolderList->AddForeignKey( BmRecvAccount::MSG_HOME_FOLDER,
													 TheRecvAccountList.Get());
		TheMailFolderList->AddForeignKey( BmFilterAddon::FK_FOLDER,
													 TheFilterList.Get());
		TheIdentityList->AddForeignKey( BmFilterAddon::FK_IDENTITY,
												  TheFilterList.Get());

		// create the node-monitor looper and the stored action flusher:
		BmMailMonitor::CreateInstance();
		BmStoredActionFlusher::CreateInstance();

		// create the job status window:
		BmJobStatusWin::CreateInstance();
		TheJobStatusWin->Hide();
		TheJobStatusWin->Show();
		TheJobMetaController = TheJobStatusWin;

		BmPeopleMonitor::CreateInstance();
		BmPeopleList::CreateInstance();

		bm_plain_font = *be_plain_font;
		bm_bold_font = *be_bold_font;
		if (ThePrefs->GetBool("ListviewUsesStringSpacing")) {
			bm_plain_font.SetSpacing(B_STRING_SPACING);
			bm_bold_font.SetSpacing(B_STRING_SPACING);
		}

		add_system_beep_event( BM_BEEP_EVENT);

		BM_LOG( BM_LogApp, BmString("...creating main-window..."));
		BmMainWindow::CreateInstance();
		
		TheBubbleHelper->EnableHelp( ThePrefs->GetBool( "ShowTooltips", true));

		mStartupLocker->Unlock();
		BM_LOG( BM_LogApp, BmString("BeamApp-initialization done."));
	} catch (BM_error& err) {
		BM_SHOWERR( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BeamApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BeamApplication::~BeamApplication() {
	RemoveDeskbarItem();
	ThePeopleMonitor = NULL;
	TheStoredActionFlusher = NULL;
	TheMailMonitor = NULL;
	ThePeopleList = NULL;
	delete mPrintSetup;
	delete TheResources;
	delete BeamGuiRoster;
}

/*------------------------------------------------------------------------------*\
	ReadyToRun()
		-	ensures that main-window is visible
		-	if Beam has been instructed to show a specific mail on startup, the
			mail-window is shown on top of the main-window
\*------------------------------------------------------------------------------*/
void BeamApplication::ReadyToRun() {
	if (TheMainWindow->IsMinimized())
		TheMainWindow->Minimize( false);
	if (mMailWin) {
		TheMainWindow->SendBehind( mMailWin);
		mMailWin = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	AppActivated()
		-	
\*------------------------------------------------------------------------------*/
void BeamApplication::AppActivated( bool active) {
}

/*------------------------------------------------------------------------------*\
	Run()
		-	starts Beam
\*------------------------------------------------------------------------------*/
thread_id BeamApplication::Run() {
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		if (BeamInTestMode) {
			// in test-mode the main-window will only be shown when neccessary:
			TheMainWindow->Hide();
			// Now wait until Test-thread allows us to start...
			snooze( 200*1000);
			mStartupLocker->Lock();
		} else if (ThePrefs->GetBool( "UseDeskbar"))
			InstallDeskbarItem();

		// start most of our list-models:
		BM_LOG( BM_LogApp, BmString("...reading receving accounts..."));
		TheRecvAccountList->StartJobInNewThread();

		// start most of our list-models:
		BM_LOG( BM_LogApp, BmString("...reading identities..."));
		TheIdentityList->StartJobInThisThread();

		BM_LOG( BM_LogApp, BmString("...reading signatures..."));
		TheSignatureList->StartJobInThisThread();

		BM_LOG( BM_LogApp, BmString("...reading filters..."));
		TheFilterList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading filter-chains..."));
		TheFilterChainList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading SMTP-accounts..."));
		TheSmtpAccountList->StartJobInThisThread();

		BM_LOG( BM_LogApp, BmString("...querying people..."));
		ThePeopleList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("Showing main-window."));
		TheMainWindow->Show();

		tid = inherited::Run();

		ThePrefs->Store();
			// always store prefs since it contains references to
			// list-items that are tracked via foreign-keys. If the
			// user has renamed a folder, for instance the references
			// to this folder will have moved along automatically, but
			// we need to store these new prefs, as otherwise it would
			// be lost from the next session onwards.
		TheRecvAccountList->StoreIfNeeded();
			// store recv-account-list since the certificate info
			// may have changed
		TheSmtpAccountList->StoreIfNeeded();
			// store smtp-account-list since the certificate info
			// may have changed
		TheIdentityList->StoreIfNeeded();
			// store identity-list since the current identity
			// may have changed
		ThePeopleList->StoreIfNeeded();
			// store people-list since new known addresses may have
			// been added
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		exit(10);
	}
	return tid;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BeamApplication::InstallDeskbarItem() {
	if (!mDeskbar.HasItem( BM_DeskbarItemName)) {
		status_t res;
		app_info appInfo;
		if ((res = beamApp->GetAppInfo( &appInfo)) == B_OK) {
			int32 id;
			appInfo.ref.set_name( BM_DeskbarItemName);
			if ((res = mDeskbar.AddItem( &appInfo.ref, &id)) != B_OK) {
				BM_SHOWERR( BmString("Unable to install Beam_DeskbarItem (")
								<< BM_DeskbarItemName<<").\nError: \n\t" 
								<< strerror( res));
			} else
				mDeskbarItemIsOurs = true;
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BeamApplication::RemoveDeskbarItem() {
	if (mDeskbarItemIsOurs && mDeskbar.HasItem( BM_DeskbarItemName))
		mDeskbar.RemoveItem( BM_DeskbarItemName);
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BeamApplication::QuitRequested() {
	BM_LOG( BM_LogApp, "App: quit requested, checking state...");
	mIsQuitting = true;
	bool shouldQuit = true;
	if (TheMailMonitor->LockLooper()) {

		int32 count = CountWindows();
		// first check if there are any active jobs:
		if (TheJobStatusWin && TheJobStatusWin->HasActiveJobs()) {
			BAlert* alert = new BAlert( "Active Jobs", 
				"There are still some jobs/connections active!"
					"\nDo you really want to quit now?",
				"Yes, Quit Now", "Cancel", NULL, B_WIDTH_AS_USUAL,
				B_WARNING_ALERT
			);
			alert->SetShortcut( 1, B_ESCAPE);
			if (alert->Go() == 1)
				shouldQuit = false;
		}
		if (shouldQuit) {
			// ask all windows if they are ready to quit, in which case we actually
			// do quit (only if *ALL* windows indicate that they are prepared to quit!):
			for( int32 i=count-1; shouldQuit && i>=0; --i) {
				BWindow* win = beamApp->WindowAt( i);
				win->Lock();
				if (win && !win->QuitRequested())
					shouldQuit = false;
				win->Unlock();
			}
		}
		
		if (!shouldQuit) {
			TheMailMonitor->UnlockLooper();
			mIsQuitting = false;
		} else {
			TheStoredActionFlusher->Quit();
			TheMailMonitor->Quit();
			for( int32 i=count-1; i>=0; --i) {
				BWindow* win = beamApp->WindowAt( i);
				if (win) {
					win->LockLooper();
					win->Quit();
				}
			}
		}
	}
	snooze( 200*1000);
		// there might be slaves running, give them some more time to stop.
	BM_LOG( BM_LogApp, 
			  shouldQuit ? "ok, App is quitting" : "no, app isn't quitting");
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	ArgvReceived( argc, argv)
		-	first argument is interpreted to be a destination mail-address, so a 
			new mail is generated for if an argument has been provided
\*------------------------------------------------------------------------------*/
void BeamApplication::ArgvReceived( int32 argc, char** argv) {
	if (argc>1 && !BeamInTestMode) {
		BmString arg( argv[1]);
		if (arg.ICompare("mailto:",7)==0)
			LaunchURL( arg);
		else {
			BMessage msg(BMM_NEW_MAIL);
			if (arg.ICompare("enclosure:",10)==0)
				msg.AddString( MSG_ENCLOSE, arg.String()+10);
			else
				msg.AddString( MSG_WHO_TO, arg.String());
			PostMessage( &msg);
		}
	}
}

/*------------------------------------------------------------------------------*\
	RefsReceived( msg)
		-	every given reference is opened, draft and pending messages are shown
			in the mail-edit-window, while other mails are opened view-only.
\*------------------------------------------------------------------------------*/
void BeamApplication::RefsReceived( BMessage* msg) {
	if (!msg)
		return;
	entry_ref inref;
	entry_ref eref;
	BEntry entry;
	for( int index=0; msg->FindRef( "refs", index, &inref) == B_OK; ++index) {
		if (entry.SetTo( &inref, true) != B_OK
		|| entry.GetRef( &eref) != B_OK)
			continue;
		BmRef<BmMailRef> ref = BmMailRef::CreateInstance( eref);
		if (ref && ref->IsValid()) {
			if (ref->Status().Length() == 0) {
				// mail has no status, meaning that it never has been
				// stored properly (the attributes haven't been written).
				// We read the mail and (re-)store it with the attributes:
				BmRef<BmMail> mail = BmMail::CreateInstance(ref.Get());
				mail->StartJobInThisThread(BmMail::BM_READ_MAIL_JOB);
				if (!mail->IsJobCompleted()) {
					BM_LOGERR( BmString("Unable to load mail ") << eref.name);
					continue;
				}
				mail->Store();
				ref = mail->MailRef();
					// update mail-ref with the attributes just created
			}
			if (ref->Status() == BM_MAIL_STATUS_DRAFT
			|| ref->Status() == BM_MAIL_STATUS_PENDING) {
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance( ref.Get());
				if (editWin) {
					editWin->Show();
					if (!mMailWin)
						mMailWin = editWin;
				}
			} else {
				BmMailViewWin* viewWin = BmMailViewWin::CreateInstance( ref.Get());
				if (viewWin) {
					viewWin->Show();
					if (!mMailWin)
						mMailWin = viewWin;
				}
			}
		} else if (CheckMimeType( &eref, "application/x-person")) {
			BmStringVect emails;
			ThePeopleList->GetEmailsFromPeopleFile( eref, emails);
			BmString email = SelectEmailForPerson( emails);
			BmRef<BmMail> mail = new BmMail( true);
			mail->SetFieldVal( BM_FIELD_TO, email);
			BmMailEditWin* editWin = BmMailEditWin::CreateInstance( mail.Get());
			if (editWin)
				editWin->Show();
		}
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles all actions on mailrefs, like replying, forwarding, 
			and creating new mails
\*------------------------------------------------------------------------------*/
void BeamApplication::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOBWIN_POP:
			case BM_JOBWIN_IMAP:
			case BMM_CHECK_MAIL: {
				while( TheRecvAccountList->IsJobRunning())
					snooze( 200*1000);
				const char* key = NULL;
				msg->FindString( BmRecvAccountList::MSG_ITEMKEY, &key);
				if (key) {
					bool isAutoCheck 
						= msg->FindBool( BmRecvAccountList::MSG_AUTOCHECK);
					BM_LOG( BM_LogApp, 
							  BmString("RecvAccount ") << key << " asks to check mail " 
							  	<< (isAutoCheck ? "(auto)" : "(manual)"));
					if (!isAutoCheck 
					|| !ThePrefs->GetBool( "AutoCheckOnlyIfPPPRunning", true) 
					|| IsPPPRunning()) {
						BM_LOG( BM_LogApp, 
								  BmString("RecvAccount ") << key	
								  		<< ": mail is checked now");
						TheRecvAccountList->CheckMailFor( key, isAutoCheck);
						if (!isAutoCheck
						&& ThePrefs->GetBool( "SendPendingMailsOnCheck", true))
							TheSmtpAccountList->SendPendingMails();
					} else
						BM_LOG( BM_LogApp, 
								  BmString("RecvAccount ") << key	
								  		<< ": mail is not checked (PPP isn't running)");
				} else {
					TheRecvAccountList->CheckMail( false);
					if (ThePrefs->GetBool( "SendPendingMailsOnCheck", true))
						TheSmtpAccountList->SendPendingMails();
				}
				break;
			}
			case BMM_CHECK_ALL: {
				BM_LOG( BM_LogApp, "Request to check mail for all accounts");
				while( TheRecvAccountList->IsJobRunning())
					snooze( 200*1000);
				TheRecvAccountList->CheckMail( true);
				if (ThePrefs->GetBool( "SendPendingMailsOnCheck", true))
					TheSmtpAccountList->SendPendingMails();
				break;
			}
			case BMM_NEW_MAIL: {
				BmRef<BmMail> mail = new BmMail( true);
				const char* to = NULL;
				if ((to = msg->FindString( MSG_WHO_TO))!=NULL)
					mail->SetFieldVal( BM_FIELD_TO, to);
				const char* optField = NULL;
				const char* enclPath = NULL;
				int32 i=0;
				for( ; msg->FindString( MSG_OPT_FIELD, i, &optField)==B_OK; ++i) {
					mail->SetFieldVal( optField, optField);
				}
				BM_LOG( BM_LogApp, 
						  BmString("Asked to create new mail with ") << i 
						  	<< " options");
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance( mail.Get());
				if (editWin)
					editWin->Show();
				int32 e=0;
				BEntry entry;
				entry_ref eref;
				status_t res;
				for( ; msg->FindString( MSG_ENCLOSE, e, &enclPath)==B_OK; ++e) {
					entry.SetTo(enclPath, true);
					if ((res=entry.GetRef(&eref)) == B_OK) {
						BMessage* attachMsg = new BMessage(BMM_ATTACH);
						attachMsg->AddRef("refs", &eref);
						editWin->PostMessage(attachMsg);
					}
				}
				break;
			}
			case BMM_REDIRECT: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Redirector", RedirectMails, msg);
				break;
			}
			case BMM_EDIT_AS_NEW: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Editor", EditMailsAsNew, msg);
				break;
			}
			case BMM_REPLY:
			case BMM_REPLY_LIST:
			case BMM_REPLY_ORIGINATOR:
			case BMM_REPLY_ALL: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Replier", ReplyToMails, msg);
				break;
			}
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Forwarder", ForwardMails, msg);
				break;
			}
			case BMM_MARK_AS: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Marker", MarkMailsAs, msg);
				break;
			}
			case BMM_MOVE: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Mover", MoveMails, msg);
				break;
			}
			case c_about_window_url_invoked: {
				const char* url;
				if (msg->FindString( "url", &url) == B_OK) {
					LaunchURL( url);
				}
				break;
			}
			case BMM_PAGE_SETUP: {
				PageSetup();
				break;
			}
			case BMM_PRINT: {
				if (!mPrintSetup)
					PageSetup();
				if (mPrintSetup) {
					DetachCurrentMessage();
					SlaveHandler.Run( "Msg-Printer", PrintMails, msg, 2);
				}
				break;
			}
			case BMM_PREFERENCES: {
				if (!ThePrefsWin) {
					BmPrefsWin::CreateInstance();
					ThePrefsWin->Show();
				} else  {
					if (ThePrefsWin->LockLooper()) {
						ThePrefsWin->Hide();
						ThePrefsWin->Show();
						ThePrefsWin->UnlockLooper();
					}
				}
				BmString subViewName = msg->FindString( "SubViewName");
				if (subViewName.Length())
					ThePrefsWin->PostMessage( msg);
				break;
			}
			case BMM_TRASH: {
				DetachCurrentMessage();
				SlaveHandler.Run( "Msg-Trasher", TrashMails, msg);
				break;
			}
			case B_SILENT_RELAUNCH: {
				BM_LOG( BM_LogApp, "App: silently relaunched");
				if (TheMainWindow->IsMinimized())
					TheMainWindow->Minimize( false);
				inherited::MessageReceived( msg);
				break;
			}
			case BM_DESKBAR_GET_MBOX: {
				if (!msg->IsReply()) {
					BMessage reply( BM_DESKBAR_GET_MBOX);
					entry_ref mboxRef;
					BEntry entry(ThePrefs->GetString("MailboxPath").String(), true);
					if (entry.GetRef(&mboxRef) == B_OK) {
						reply.AddRef( "mbox", &mboxRef);
						msg->SendReply( &reply, (BHandler*)NULL, 1000000);
					}
				}
				break;
			}
			case BMM_CREATE_PERSON_FROM_ADDR: {
				const char* name = NULL;
				const char* email = NULL;
				bool edit = false;
				msg->FindBool("edit", &edit);
				uint32 idx;
				entry_ref eref;
				for( idx=0; msg->FindString("name", idx, &name) == B_OK; ) {
					msg->FindString("email", idx, &email);
					BmRef<BmPerson> person 
						= ThePeopleList->FindPersonByName(name);
					if (person) {
						eref = person->EntryRef();
						person->CreateNewEmail(email);
					} else
						ThePeopleList->CreateNewPerson(name, email, &eref);
					if (edit)
						be_roster->Launch(&eref);
					idx++;
				}
				break;
			}
			case BMM_EDIT_PERSON_WITH_ADDR: {
				const char* email = NULL;
				uint32 idx;
				entry_ref eref;
				for( idx=0; msg->FindString("email", idx, &email) == B_OK; ) {
					BmRef<BmPerson> person 
						= ThePeopleList->FindPersonByEmail(email);
					if (person) {
						eref = person->EntryRef();
						be_roster->Launch(&eref);
					}
					idx++;
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
				break;
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("BmApp: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	PageSetup()
		-	sets up the basic printing environment
\*------------------------------------------------------------------------------*/
void BeamApplication::PageSetup() {
	if (mPrintSetup)
		mPrintJob.SetSettings( new BMessage( *mPrintSetup));
		
	status_t result = mPrintJob.ConfigPage();
	if (result == B_OK) {
		delete mPrintSetup;
		mPrintSetup = mPrintJob.Settings();
	}
}

/*------------------------------------------------------------------------------*\
	LaunchURL( url)
		-	launches the corresponding program for the given URL 
			(usually Netpositive)
		-	mailto: - URLs are handled internally
\*------------------------------------------------------------------------------*/
void BeamApplication::LaunchURL( const BmString url) {
	char* urlStr = const_cast<char*>( url.String());
	status_t result;
	if (url.ICompare( "https:", 6) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.https", 1, &urlStr);
	else if (url.ICompare( "http:", 5) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.http", 1, &urlStr);
	else if (url.ICompare( "ftp:", 4) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.ftp", 1, &urlStr);
	else if (url.ICompare( "file:", 5) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.file", 1, &urlStr);
	else if (url.ICompare( "mailto:", 7) == 0) {
		BMessage msg(BMM_NEW_MAIL);
		BmString to( urlStr+7);
		int32 optPos = to.IFindFirst("?");
		if (optPos != B_ERROR) {
			BmString opts;
			to.MoveInto( opts, optPos, to.Length());
			Regexx rx;
			int32 optCount = rx.exec( opts, "[?&]([^&=]+)=([^&]+)", 
											  Regexx::global);
			for( int i=0; i<optCount; ++i) {
				BmString rawVal( rx.match[i].atom[1]);
				rawVal.DeUrlify();
				BmString field( rx.match[i].atom[0]);
				msg.AddString( MSG_OPT_FIELD, field.String());
				msg.AddString( MSG_OPT_VALUE, rawVal.String());
			}
		}
		to.DeUrlify();
		msg.AddString( BeamApplication::MSG_WHO_TO, to.String());
		beamApp->PostMessage( &msg);
		return;
	}
	else result = B_ERROR;
	if (!(result == B_OK || result == B_ALREADY_RUNNING)) {
		(new BAlert( "", (BmString("Could not launch application for url\n\t") 
								<< url << "\n\nError:\n\t" 
								<< strerror(result)).String(),
					   "OK"))->Go();
	}
}

/*------------------------------------------------------------------------------*\
	AboutRequested()
		-	standard BeOS-behaviour, we show about-window
\*------------------------------------------------------------------------------*/
void BeamApplication::AboutRequested() {
	ImageAboutWindow* aboutWin = new ImageAboutWindow(
		"About Beam",
		"Beam",
		TheResources->IconByName("AboutIcon")->bitmap,
		15, 
		"BEware, Another Mailer\n(c) Oliver Tappe, Berlin, Germany",
		"mailto:beam@hirschkaefer.de",
		"http://beam.sourceforge.net",
		"\n\n\n\n\n\n\nBeam makes use of the following Software:\n\
\n\
	Libiconv, by Bruno Haible\n\
\n\
	Liblayout, by Marco Nelissen\n\
\n\
	OSBF from crm114,\n\
	    by Bill Yerazunis (crm114)\n\
	    and Fidelis Assis (OSBF)\n\
\n\
	PCRE, by Philip Hazel\n\
\n\
	Regexx, by Gustavo Niemeyer\n\
\n\
	SantasGiftBag, by Brian Tietz\n\
\n\
	if available:\n\
	    openssl, by the OpenSSL Project\n\
\n\n\n\n\n\n\nThanks to:\n\
\n\
Heike Herfart \n\
	for understanding the geek, \n\
	the testing sessions \n\
	and many, many suggestions. \n\
\n\
\n\
...and (in alphabetical order):\n\
\n\
Adam McNutt\n\
Alan Westbrook\n\
Atillâ Öztürk\n\
Bernd Thorsten Korz\n\
Cedric Vincent\n\
Charlie Clark\n\
David Vignoni\n\
Eberhard Hafermalz\n\
Eugenia Loli-Queru\n\
Hartmut Reh\n\
Helmar Rudolph\n\
Ingo Weinhold\n\
Jace Cavacini\n\
Jens Neuwerk\n\
Jon Hart\n\
Kevin Musick\n\
Koki\n\
Lars Müller\n\
Len G. Jacob\n\
Linus Almstrom\n\
Mathias Reitinger\n\
Max Hartmann\n\
MDR-team (MailDaemonReplacement)\n\
Mikael Larsson\n\
Mikhail Panasyuk\n\
Olivier Milla\n\
qwilk\n\
Paweł Lewicki\n\
Rainer Riedl\n\
Rob Lund\n\
Shard\n\
Stephan Assmus\n\
Stephan Buelling\n\
Stephen Butters\n\
Tyler Dauwalder\n\
Zach\n\
\n\
\n\n\n\n\
...and thanks to everyone I forgot, too!\n\
\n\
\n\n\n\n\n\n"
	);
	aboutWin->Show();
}

/*------------------------------------------------------------------------------*\
	ScreenFrame()
		-	returns the the current screen's frame
\*------------------------------------------------------------------------------*/
BRect BeamApplication::ScreenFrame() {
	BScreen screen( TheMainWindow);
	if (!screen.IsValid())
		BM_SHOWERR( BmString("Could not initialize BScreen object !?!"));
	return screen.Frame();
}

/*------------------------------------------------------------------------------*\
	SetNewWorkspace( newWorkspace)
		-	ensures that main-window and job-status-window are always shown inside
			the same workspace
\*------------------------------------------------------------------------------*/
void BeamApplication::SetNewWorkspace( uint32 newWorkspace) {
	if (TheMainWindow->Workspaces() != newWorkspace)
		TheMainWindow->SetWorkspaces( newWorkspace);
	if (TheJobStatusWin->Workspaces() != newWorkspace)
		TheJobStatusWin->SetWorkspaces( newWorkspace);
}

/*------------------------------------------------------------------------------*\
	CurrWorkspace()
		-	
\*------------------------------------------------------------------------------*/
uint32 BeamApplication::CurrWorkspace() {
	return TheMainWindow->Workspaces();
}

/*------------------------------------------------------------------------------*\
	HandlesMimetype( mimetype)
		-	determines whether or not Beam handles the given mimetype
		-	returns true for any mail-related types, false otherwise
\*------------------------------------------------------------------------------*/
bool BeamApplication::HandlesMimetype( const BmString mimetype) {
	return BeamRoster->IsSupportedEmailMimeType( mimetype);
}
