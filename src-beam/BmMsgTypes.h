/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	BMM_EDIT_AS_NEW				= 'bMdq',
	BMM_LEARN_AS_SPAM				= 'bMdr',
	BMM_LEARN_AS_TOFU				= 'bMds',
	BMM_NEXT_MESSAGE				= 'bMdt',
	BMM_PREVIOUS_MESSAGE			= 'bMdu',
	BMM_NARROW_DOWN				= 'bMdv',
	BMM_SET_TIME_SPAN				= 'bMdw',
	
	// mail edit window
	BMM_ATTACH						= 'bMea',
	BMM_OPEN							= 'bMeb',
	BMM_SAVE							= 'bMec',
	BMM_SEND_LATER					= 'bMed',
	BMM_SEND_NOW					= 'bMee',
	BMM_SHOW_PEOPLE				= 'bMef'
	
};

#endif
