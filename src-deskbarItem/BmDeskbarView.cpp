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
/* This source has been derived (ripped?) from Scooby!                   */
/*                                                                       */
/*************************************************************************/

#include <Autolock.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Entry.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <stdio.h>

#include "BmDeskbarView.h"

extern const char* BM_APP_SIG;

/***********************************************************
 * This is the exported function that will be used by Deskbar
 * to create and add the replicant
 ***********************************************************/
extern "C" _EXPORT BView* instantiate_deskbar_item();

const char* const DbViewName = "Beam";

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
	:	BView( frame, DbViewName, B_FOLLOW_NONE, B_WILL_DRAW|B_PULSE_NEEDED)
	,	mCurrIcon( NULL)
{
	mResetLabel = strdup( "Reset Icon");
}

/***********************************************************
 * Constructor for achiving.
 ***********************************************************/
BmDeskbarView::BmDeskbarView(BMessage *message)
	:	BView( message)
	,	mCurrIcon( NULL)
{
	mResetLabel = strdup( "Reset Icon");
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
	return data->AddString( "add_on", BM_APP_SIG);
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
void BmDeskbarView::MessageReceived(BMessage *message) {
	switch(message->what)
	{
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * ChangeIcon
 ***********************************************************/
void BmDeskbarView::ChangeIcon( const char* iconName) {
	if (!iconName || mCurrIconName == iconName)
		return;
		
	BBitmap* newIcon = NULL;
	entry_ref ref;
	if (be_roster->FindApp( BM_APP_SIG, &ref) == B_OK)
	{
		// Load icon from Beam's resources
		BFile file( &ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BResources rsrc( &file);
			size_t len;
			const void *data = rsrc.LoadResource( 'BBMP', iconName, &len);
			if (len) {
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
	Invalidate();
}

/***********************************************************
 * Pulse
 ***********************************************************/
void BmDeskbarView::Pulse() {
	BMessenger beam( BM_APP_SIG);
	if (beam.IsValid())
	{
		BMessage reply;
		BMessage msg( BM_CHECK_STATE);
		if (beam.SendMessage( &msg, &reply, 1000000, 1000000) == B_OK
		&& reply.what != B_NO_REPLY) {
			const char* iconName;
			if (reply.FindString( "iconName", &iconName) == B_OK)
				ChangeIcon( iconName);
			return;
		}
	}
	// Beam has probably crashed, we quit, too:
	BDeskbar().RemoveItem( DbViewName);
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void BmDeskbarView::MouseDown(BPoint pos) {
/*
	entry_ref app;
	BMessage msg(M_SHOW_MSG);
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 

	if(buttons == B_SECONDARY_MOUSE_BUTTON)
	{
  		BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
  		BFont font(be_plain_font);
  		font.SetSize(10);
  		theMenu->SetFont(&font);
  	
  		MenuUtils utils;
  		utils.AddMenuItem(theMenu,fLabels[3],M_LAUNCH_SCOOBY,NULL,NULL,0,0);
  		utils.AddMenuItem(theMenu,fLabels[0],M_NEW_MESSAGE,NULL,NULL,0,0);
  		
  		theMenu->AddSeparatorItem();
  		utils.AddMenuItem(theMenu,fLabels[1],M_CHECK_NOW,NULL,NULL,0,0);
  		theMenu->AddSeparatorItem();
		utils.AddMenuItem(theMenu,fLabels[2],B_QUIT_REQUESTED,NULL,NULL,0,0);
  
		BRect r ;
   		ConvertToScreen(&pos);
   		r.top = pos.y - 5;
   		r.bottom = pos.y + 5;
   		r.left = pos.x -5;
	   	r.right = pos.x +5;
         
		BMenuItem *bItem = theMenu->Go(pos, false,true,r);  
    	if(bItem)
    	{
    		BMessage*	aMessage = bItem->Message();
			if(aMessage)
			{
				if(be_roster->IsRunning(APP_SIG))
				{
					team_id id = be_roster->TeamFor(APP_SIG);
					BMessenger messenger(APP_SIG,id);
					messenger.SendMessage(aMessage,(BHandler*)NULL,1000000);
				}
			}
		}
		delete theMenu;
	}else{
		BMessage sndMsg(M_RESET_ICON);
		if(be_roster->IsRunning(APP_SIG))
		{
			team_id id = be_roster->TeamFor(APP_SIG);
			BMessenger messenger(APP_SIG,id);
			messenger.SendMessage(&sndMsg,(BHandler*)NULL,1000000);
		}
		ChangeIcon(DESKBAR_NORMAL_ICON);
	}
*/
	inherited::MouseDown(pos);
}
