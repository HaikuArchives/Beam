/*
	BmApp.h
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


#ifndef _Beam_h
#define _Beam_h

#include "BmApp.h"

class BmWindow;

class BeamApp : public BmApplication
{
	typedef BmApplication inherited;

public:
	BeamApp();
	~BeamApp();

	// overrides of BmApplication
	thread_id Run();
	void MessageReceived(BMessage*);
	void ReadyToRun();
	void RefsReceived( BMessage* msg);

private:
	status_t mInitCheck;
	BmWindow* mMailWin;

	inline status_t InitCheck() 			{ return mInitCheck; }
};

extern BeamApp* beamApp;

#endif
