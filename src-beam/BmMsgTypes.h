/*
	BmMsgTypes.h
		$Id$
*/

#ifndef _BmMsgTypes_h
#define _BmMsgTypes_h

/*------------------------------------------------------------------------------*\
	types of messages handled by the Beam-Application:
\*------------------------------------------------------------------------------*/
#define BM_APP_CONNWIN_DONE			'bmaa'
						// sent from BmConnectionWin to app when a connection has finished

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
#define BM_NTFY_MAILREF_SELECTION	'bmba'
						// sent from BmMailRefView to observers whenever mail-selection changes

/*------------------------------------------------------------------------------*\
	types of messages handled by a listview-controller:
\*------------------------------------------------------------------------------*/
#define BM_LISTVIEW_SHOW_COLUMN		'bmca'
							// the user has chosen to show a column
#define BM_LISTVIEW_HIDE_COLUMN		'bmcb'
							// the user has chosen to hide a column

/*------------------------------------------------------------------------------*\
	message types for BmDataModel (and subclasses), all msgs are sent to 
	the handler specified via each Controllers GetControllerHandler()-method,
	so these messages are sent from a datamodel to all its controllers:
\*------------------------------------------------------------------------------*/
#define BM_JOB_DONE						'bmda'
							// the job has finished or was stopped
#define BM_JOB_UPDATE_STATE			'bmdb'
							// the job wants to update (one of) its state(s)
#define BM_LISTMODEL_ADD				'bmdc'
							// the listmodel has added a new item
#define BM_LISTMODEL_REMOVE			'bmdd'
							// the listmodel has removed an item
#define BM_LISTMODEL_UPDATE			'bmde'
							// the listmodel indicates a state-change of on of its items

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmJobStatusWin:
\*------------------------------------------------------------------------------*/
#define BM_JOBWIN_FETCHPOP				'bmea'
						// sent to BmJobStatusWin in order to start pop-connection
#define BM_JOBWIN_MOVEMAILS			'bmeb'
						// sent to BmJobStatusWin in order to move mails

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailHeaderView:
\*------------------------------------------------------------------------------*/
#define BM_HEADERVIEW_SMALL			'bmfa'
#define BM_HEADERVIEW_LARGE			'bmfb'
#define BM_HEADERVIEW_FULL				'bmfc'

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmBodyPartView:
\*------------------------------------------------------------------------------*/
#define BM_BODYPARTVIEW_SHOWALL		'bmga'
#define BM_BODYPARTVIEW_SHOWINLINE	'bmgb'

/*------------------------------------------------------------------------------*\
	DRAG-message:
\*------------------------------------------------------------------------------*/
#define BM_MAIL_DRAG						'bmha'
#define BM_ATTACHMENT_DRAG				'bmhb'

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmLogfile:
\*------------------------------------------------------------------------------*/
#define BM_LOG_MSG						'bmha'


/*------------------------------------------------------------------------------*\
	Menu-messages:
\*------------------------------------------------------------------------------*/
// main window
#define BMM_NEW_MAILFOLDER				'bMfa'
#define BMM_PAGE_SETUP					'bMfb'
#define BMM_PRINT							'bMfc'
#define BMM_PREFERENCES					'bMfd'
#define BMM_FIND							'bMfe'
#define BMM_FIND_MESSAGES				'bMff'
#define BMM_FIND_NEXT					'bMfg'
#define BMM_CHECK_MAIL					'bMfh'
#define BMM_CHECK_ALL					'bMfi'
#define BMM_SEND_PENDING				'bMfj'
#define BMM_NEW_MAIL						'bMfk'
#define BMM_REPLY							'bMfl'
#define BMM_REPLY_ALL					'bMfm'
#define BMM_FORWARD						'bMfn'
#define BMM_FORWARD_ATTACHMENTS		'bMfo'
#define BMM_BOUNCE						'bMfp'
#define BMM_FILTER						'bMfq'
#define BMM_TRASH							'bMfr'
// mail edit window
#define BMM_ATTACH						'bMga'
#define BMM_OPEN							'bMgb'
#define BMM_SAVE							'bMgc'
#define BMM_SEND_LATER					'bMgd'
#define BMM_SEND_NOW						'bMge'
#define BMM_SHOW_PEOPLE					'bMgf'


#endif
