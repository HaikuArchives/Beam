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

/*************************************************************************/
/*                                                                       */
/* This source has been derived (ripped?) from Scooby!                   */
/*                                                                       */
/*************************************************************************/

#include <Message.h>
#include <View.h>

extern const char* const DbViewName;

// message types for communication between deskbar-view and Beam:
enum{
	BM_CHECK_STATE = 	'bmDa'
};

// menu-messages for deskbar-view:
enum{
	BMM_RESET_ICON = 	'bMDa'
};

class BmDeskbarView: public BView {
	typedef BView inherited;
public:
	// c'tors and d'tor:
	BmDeskbarView( BRect frame);
	BmDeskbarView( BMessage *data);
	~BmDeskbarView();
	
	// native methods:
	void ChangeIcon( const char* iconName);
	
protected:	
	// overrides of BView base:
	void Draw( BRect updateRect);
	status_t Archive(BMessage *data, bool deep = true) const;
	void MouseDown(BPoint);
	void MessageReceived(BMessage *message);
	void Pulse();

	static BmDeskbarView *Instantiate(BMessage *data);

private:
	BString mCurrIconName;
	BBitmap *mCurrIcon;
	BString mResetLabel;
};

#endif
