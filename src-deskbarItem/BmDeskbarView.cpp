/*
	BmDeskbarView.cpp
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

/*************************************************************************/
/*                                                                       */
/* Parts of this source-file have been derived (ripped?) from Scooby!    */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <Volume.h>

#include "BmDeskbarView.h"
#include "BmMsgTypes.h"

/***********************************************************
 * This is the exported function that will be used by Deskbar
 * to create and add the replicant
 ***********************************************************/
extern "C" _EXPORT BView* instantiate_deskbar_item();

const char* BM_DESKBAR_APP_SIG = "application/x-vnd.zooey-beam_deskbar";
const char* BM_APP_SIG = "application/x-vnd.zooey-beam";

const char* const BM_DeskbarItemName = "Beam_DeskbarItem";
const char* const BM_DeskbarNormal = "DeskbarIconNormal";
const char* const BM_DeskbarNew = "DeskbarIconNew";

/***********************************************************
 * Deskbar item installing function
 ***********************************************************/
BView* instantiate_deskbar_item(void) {
	return new BmDeskbarView(BRect(0, 0, 15, 15));
}

/***********************************************************
 * Constructor.
 ***********************************************************/
BmDeskbarView::BmDeskbarView(BRect frame)
	:	BView( frame, BM_DeskbarItemName, B_FOLLOW_NONE, 
				 B_WILL_DRAW|B_PULSE_NEEDED)
	,	mCurrIcon( NULL)
	,	mNewMailCount( 0)
{
}

/***********************************************************
 * Constructor for achiving.
 ***********************************************************/
BmDeskbarView::BmDeskbarView(BMessage *message)
	:	BView( message)
	,	mCurrIcon( NULL)
	,	mNewMailCount( 0)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
BmDeskbarView::~BmDeskbarView() {
	delete mCurrIcon;
}

/***********************************************************
 * Instantiate
 ***********************************************************/
BmDeskbarView* BmDeskbarView::Instantiate(BMessage *data) {
	if (!validate_instantiation(data, "BmDeskbarView"))
		return NULL;
	return new BmDeskbarView(data);
}

/***********************************************************
 * Archive
 ***********************************************************/
status_t BmDeskbarView::Archive( BMessage *data,
										   bool deep) const {
	BView::Archive(data, deep);
	return data->AddString( "add_on", BM_DESKBAR_APP_SIG);
}

void BmDeskbarView::AttachedToWindow() {
	// ask app for the volume of our mailbox-directory:
	BMessage request(BM_DESKBAR_GET_MBOX_DEVICE);
	SendToBeam( &request, this);
}


/***********************************************************
 * Draw
 ***********************************************************/
void BmDeskbarView::Draw(BRect /*updateRect*/) {	
	rgb_color oldColor = HighColor();
	SetHighColor(Parent()->ViewColor());
	FillRect(BRect(0.0,0.0,15.0,15.0));
	SetHighColor(oldColor);
	SetDrawingMode(B_OP_OVER);
	if(mCurrIcon)
		DrawBitmap(mCurrIcon,BRect(0.0,0.0,15.0,15.0));
	SetDrawingMode(B_OP_COPY);
	Sync();
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void BmDeskbarView::MessageReceived(BMessage *msg) {
	switch( msg->what) {
		case BMM_RESET_ICON: {
			ChangeIcon( BM_DeskbarNormal); 
			break;
		}
		case B_QUERY_UPDATE: {
			HandleQueryUpdateMsg( msg);
			break;
		}
		case BM_DESKBAR_GET_MBOX_DEVICE: {
			dev_t mbox_dev = msg->FindInt32( "mbox_dev");
			InstallDeskbarMonitor( mbox_dev);
			break;
		}
		default: {
			BView::MessageReceived( msg);
		}
	}
}

/***********************************************************
 * IncNewMailCount
 ***********************************************************/
void BmDeskbarView::IncNewMailCount()	{ 
	if (mNewMailCount++ == 0 || mCurrIconName != BM_DeskbarNew)
		ChangeIcon( BM_DeskbarNew); 
}

/***********************************************************
 * DecNewMailCount()
 ***********************************************************/
void BmDeskbarView::DecNewMailCount() {
	if (--mNewMailCount < 0)
		mNewMailCount = 0;
	if (!mNewMailCount) 
		ChangeIcon( BM_DeskbarNormal); 
}

/***********************************************************
 * ChangeIcon
 ***********************************************************/
void BmDeskbarView::ChangeIcon( const char* iconName) {
	if (!iconName || mCurrIconName == iconName)
		return;
		
	BBitmap* newIcon = NULL;
	team_id beamTeam = be_roster->TeamFor( BM_APP_SIG);
	app_info appInfo;
	if (be_roster->GetRunningAppInfo( beamTeam, &appInfo) == B_OK) {
		appInfo.ref.set_name( BM_DeskbarItemName);
		// Load icon from the deskbaritem's resources
		BFile file( &appInfo.ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BResources rsrc( &file);
			size_t len = 0;
			const void *data = rsrc.LoadResource( 'BBMP', iconName, &len);
			if (len && data) {
				BMemoryIO stream( data, len);
				stream.Seek( 0, SEEK_SET);
				BMessage archive;
				if (archive.Unflatten( &stream) == B_OK)
					newIcon = new BBitmap( &archive);
			}
		}
	}
	delete mCurrIcon;
	mCurrIcon = newIcon;
	mCurrIconName = newIcon ? iconName : "";
	if (LockLooper()) {
		Invalidate();
		UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	InstallDeskbarMonitor()
		-	
\*------------------------------------------------------------------------------*/
void BmDeskbarView::InstallDeskbarMonitor( dev_t mbox_dev) {
	int32 count;
	dirent* dent;
	char buf[4096];

	entry_ref eref;
	BEntry entry;
	
	BMessenger thisAsTarget( this);
	
	// determine root-dir of our mailbox-directory:
	BDirectory mboxRoot;
	BVolume mboxVolume( mbox_dev);
	mboxVolume.GetRootDirectory( &mboxRoot);
	if (mboxRoot.InitCheck() == B_OK) {

		// fetch node-ref of Trash (a kludge, should be able to handle more than one):
		BDirectory trash( &mboxRoot, "home/Desktop/Trash");
		trash.GetNodeRef( &mTrashNodeRef);

		if (mNewMailQuery.SetVolume( &mboxVolume) == B_OK
		&& mNewMailQuery.SetPredicate( "MAIL:status = 'New'") == B_OK
		&& mNewMailQuery.SetTarget( thisAsTarget) == B_OK
		&& mNewMailQuery.Fetch() == B_OK) {
			while ((count = mNewMailQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
				dent = (dirent* )buf;
				while (count-- > 0) {
					eref.device = dent->d_pdev;
					eref.directory = dent->d_pino;
					eref.set_name(dent->d_name);
					entry.SetTo(&eref);
					if (!trash.Contains(&entry))
						mNewMailCount++;
					// Bump the dirent-pointer by length of the dirent just handled:
					dent = (dirent* )((char* )dent + dent->d_reclen);
				}
			}
		}
	}
	ChangeIcon( mNewMailCount ? BM_DeskbarNew : BM_DeskbarNormal);
}

/***********************************************************
 * SendToBeam
 ***********************************************************/
void BmDeskbarView::SendToBeam( BMessage* msg, BHandler* replyHandler) {
	BMessenger beam( BM_APP_SIG);
	if (beam.IsValid()) {
		if (beam.SendMessage( msg, replyHandler, 1000000) == B_OK)
			return;
	}
	// Beam has probably crashed, we quit, too:
	BDeskbar().RemoveItem( BM_DeskbarItemName);
}

/***********************************************************
 * Pulse
 ***********************************************************/
void BmDeskbarView::Pulse() {
	BMessenger beam( BM_APP_SIG);
	if (!beam.IsValid()) {
		// Beam has probably crashed, we quit, too:
		BDeskbar().RemoveItem( BM_DeskbarItemName);
	}
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void BmDeskbarView::MouseDown(BPoint pos) {
	inherited::MouseDown(pos);
	int32 buttons;
	BMessage* msg = Looper()->CurrentMessage();
	if (msg && msg->FindInt32( "buttons", &buttons)==B_OK) {
		if (buttons == B_PRIMARY_MOUSE_BUTTON) {
			BMessage actMsg( B_SILENT_RELAUNCH);
			SendToBeam( &actMsg);
		} else if (buttons == B_SECONDARY_MOUSE_BUTTON)
			ShowMenu( pos);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmDeskbarView::ShowMenu( BPoint point) {
	BMenuItem* item;
	BPopUpMenu* theMenu = new BPopUpMenu( BM_DeskbarItemName, false, false);
	BFont menuFont( *be_plain_font);
	theMenu->SetFont( &menuFont);

	item = new BMenuItem( "Check Mail", new BMessage( BMM_CHECK_MAIL));
	item->SetTarget( BMessenger( BM_APP_SIG));
	theMenu->AddItem( item);

	theMenu->AddSeparatorItem();
	item = new BMenuItem( "New Message...", new BMessage( BMM_NEW_MAIL));
	item->SetTarget( BMessenger( BM_APP_SIG));
	theMenu->AddItem( item);

	theMenu->AddSeparatorItem();
	item = new BMenuItem( "Reset Icon", new BMessage( BMM_RESET_ICON));
	item->SetTarget( this);
	theMenu->AddItem( item);

	theMenu->AddSeparatorItem();
	item = new BMenuItem( "Quit Beam", new BMessage( B_QUIT_REQUESTED));
	item->SetTarget( BMessenger( BM_APP_SIG));
	theMenu->AddItem( item);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
}

/*------------------------------------------------------------------------------*\
	HandleQueryUpdateMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmDeskbarView::HandleQueryUpdateMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	node_ref nref;
	switch( opcode) {
		case B_ENTRY_CREATED: {
			if (msg->FindInt64( "directory", &nref.node) == B_OK
			&& msg->FindInt32( "device", &nref.device) == B_OK
			&& nref != mTrashNodeRef)
				IncNewMailCount();
			break;
		}
		case B_ENTRY_REMOVED: {
			if (msg->FindInt64( "directory", &nref.node) == B_OK
			&& msg->FindInt32( "device", &nref.device) == B_OK
			&& nref != mTrashNodeRef)
				DecNewMailCount();
			break;
		}
	}
}
