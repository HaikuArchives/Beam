/*
	BmDeskbarView.h
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

#ifndef _BmDeskbarView_h
#define _BmDeskbarView_h


#include <Message.h>
#include <Query.h>
#include <String.h>
#include <View.h>


extern const char* BM_APP_SIG;
extern const char* BM_DESKBAR_APP_SIG;

extern const char* const BM_DeskbarItemName;
extern const char* const BM_DeskbarNormal;
extern const char* const BM_DeskbarNew;

// menu-messages for deskbar-view:
enum {
	BMM_RESET_ICON = 		'bMDa'
};

// messages send from deskbar-view to app:
enum {
	BM_DESKBAR_GET_MBOX_DEVICE = 'bMDb'
};

class BmDeskbarView: public BView {
	typedef BView inherited;
public:
	// c'tors and d'tor:
	BmDeskbarView( BRect frame);
	BmDeskbarView( BMessage *data);
	~BmDeskbarView();
	void Init();
	
	static BmDeskbarView *Instantiate( BMessage *data);

protected:	
	// native methods:
	void ChangeIcon( const char* iconName);
	void ShowMenu( BPoint point);
	void IncNewMailCount();
	void DecNewMailCount();
	void SendToBeam( BMessage *msg, BHandler *replyHandler = NULL);
	void InstallDeskbarMonitor( dev_t mbox_dev);
	void HandleQueryUpdateMsg( BMessage* msg);
	
	// overrides of BView base:
	void Draw( BRect updateRect);
	status_t Archive(BMessage *data, bool deep = true) const;
	void MouseDown(BPoint);
	void MessageReceived(BMessage *message);
	void Pulse( void);
	void AttachedToWindow();
	
private:
	BQuery mNewMailQuery;
	int32 mNewMailCount;
	BString mCurrIconName;
	BBitmap *mCurrIcon;
	node_ref mTrashNodeRef;
};

#endif
