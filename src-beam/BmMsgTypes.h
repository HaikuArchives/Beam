/*
	BmMsgTypes.h
		$Id$
*/

#ifndef _BmMsgTypes_h
#define _BmMsgTypes_h

/*------------------------------------------------------------------------------*\
	types of messages handled by the Beam-Application:
\*------------------------------------------------------------------------------*/
#define BM_APP_CONNWIN_DONE			'bmab'
						// sent from BmConnectionWin to app when a connection has finished

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
	types of messages handled by a BmConnectionWin:
\*------------------------------------------------------------------------------*/
#define BM_CONNWIN_FETCHPOP			'bmea'
						// sent from App BmConnectionWin in order to start pop-connection

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailHeaderView:
\*------------------------------------------------------------------------------*/
#define BM_HEADERVIEW_SMALL			'bmfa'
#define BM_HEADERVIEW_LARGE			'bmfb'
#define BM_HEADERVIEW_FULL				'bmfc'


#endif
