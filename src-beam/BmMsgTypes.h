/*
	BmMsgTypes.h
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


#ifndef _BmMsgTypes_h
#define _BmMsgTypes_h

/*------------------------------------------------------------------------------*\
	Menu-messages:
\*------------------------------------------------------------------------------*/

enum {
	// main window
	BMM_NEW_MAILFOLDER			= 'bMaa',
	BMM_RENAME_MAILFOLDER		= 'bMab',
	BMM_DELETE_MAILFOLDER		= 'bMac',
	BMM_RECACHE_MAILFOLDER		= 'bMad',
	BMM_PAGE_SETUP					= 'bMae',
	BMM_PRINT						= 'bMaf',
	BMM_PREFERENCES				= 'bMag',
	BMM_SHOW_LOGFILE				= 'bMah',
	
	BMM_FIND							= 'bMba',
	BMM_FIND_MESSAGES				= 'bMbb',
	BMM_FIND_NEXT					= 'bMbc',
	
	BMM_CHECK_MAIL					= 'bMca',
	BMM_CHECK_ALL					= 'bMcb',
	BMM_SEND_PENDING				= 'bMcc',
	
	BMM_NEW_MAIL					= 'bMda',
	BMM_REPLY						= 'bMdb',
	BMM_REPLY_LIST					= 'bMdc',
	BMM_REPLY_ORIGINATOR			= 'bMdd',
	BMM_REPLY_ALL					= 'bMde',
	BMM_FORWARD_ATTACHED			= 'bMdf',
	BMM_FORWARD_INLINE			= 'bMdg',
	BMM_FORWARD_INLINE_ATTACH	= 'bMdh',
	BMM_REDIRECT					= 'bMdi',
	BMM_FILTER						= 'bMdj',
	BMM_TRASH						= 'bMdk',
	BMM_MARK_AS						= 'bMdl',
	BMM_SWITCH_RAW					= 'bMdm',
	BMM_SWITCH_HEADER				= 'bMdn',
	BMM_MOVE							= 'bMdo',
	BMM_CREATE_FILTER				= 'bMdp',
	
	// mail edit window
	BMM_ATTACH						= 'bMea',
	BMM_OPEN							= 'bMeb',
	BMM_SAVE							= 'bMec',
	BMM_SEND_LATER					= 'bMed',
	BMM_SEND_NOW					= 'bMee',
	BMM_SHOW_PEOPLE				= 'bMef'
	
};

#endif
