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
// main window
#define BMM_NEW_MAILFOLDER				'bMaa'
#define BMM_RENAME_MAILFOLDER			'bMab'
#define BMM_DELETE_MAILFOLDER			'bMac'
#define BMM_RECACHE_MAILFOLDER		'bMad'
#define BMM_PAGE_SETUP					'bMae'
#define BMM_PRINT							'bMaf'
#define BMM_PREFERENCES					'bMag'

#define BMM_FIND							'bMba'
#define BMM_FIND_MESSAGES				'bMbb'
#define BMM_FIND_NEXT					'bMbc'

#define BMM_CHECK_MAIL					'bMca'
#define BMM_CHECK_ALL					'bMcb'
#define BMM_SEND_PENDING				'bMcc'

#define BMM_NEW_MAIL						'bMda'
#define BMM_REPLY							'bMdb'
#define BMM_REPLY_ALL					'bMdc'
#define BMM_FORWARD_ATTACHED			'bMdd'
#define BMM_FORWARD_INLINE				'bMde'
#define BMM_FORWARD_INLINE_ATTACH	'bMdf'
#define BMM_REDIRECT						'bMdg'
#define BMM_FILTER						'bMdh'
#define BMM_TRASH							'bMdi'
#define BMM_MARK_AS						'bMdj'

// mail edit window
#define BMM_ATTACH						'bMea'
#define BMM_OPEN							'bMeb'
#define BMM_SAVE							'bMec'
#define BMM_SEND_LATER					'bMed'
#define BMM_SEND_NOW						'bMee'
#define BMM_SHOW_PEOPLE					'bMef'


#endif
