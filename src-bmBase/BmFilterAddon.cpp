/*
	BmFilterAddon.cpp

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


#include "BmFilterAddon.h"

/********************************************************************************\
	BmFilterAddon
\********************************************************************************/

const char* const BmFilterAddon::FK_FOLDER =   "bm:folder";
const char* const BmFilterAddon::FK_IDENTITY = "bm:identity";

/*------------------------------------------------------------------------------*\
	BmFilterAddon()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilterAddon::BmFilterAddon() 
{
}

/*------------------------------------------------------------------------------*\
	~BmFilterAddon()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilterAddon::~BmFilterAddon() {
}



/********************************************************************************\
	BmMsgContext
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMsgContext()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmMsgContext::BmMsgContext( const BmString& mtxt, const BmString& mid,
									 bool outb, const BmString& stat, 
									 const BmString& acc)
	:	rawMsgText( mtxt)
	,	mailId( mid)
	,	outbound( outb)
	,	account( acc)
	,	headerInfos( NULL)
	,	status( stat)
	,	moveToTrash( false)
	,	stopProcessing( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmMsgContext()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMsgContext::~BmMsgContext() {
	if (headerInfos) {
		for( int i=0; i<headerInfoCount; ++i)
			delete [] headerInfos[i].values;
		delete [] headerInfos;
	}
}

