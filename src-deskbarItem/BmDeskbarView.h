/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmDeskbarView_h
#define _BmDeskbarView_h

#include <map>

#include <Entry.h>
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
	BM_DESKBAR_GET_MBOX = 'bMDb'
};

class BmDeskbarView: public BView {
	typedef BView inherited;
	typedef map<int64, entry_ref> NewMailMap;
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
	int32 NewMailCount();
	void SendToBeam( BMessage *msg, BHandler *replyHandler = NULL);
	void InstallDeskbarMonitor();
	bool LivesInMailbox(const entry_ref& eref);
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
	bool mNewMailCountNeedsUpdate;
	BString mCurrIconName;
	BBitmap *mCurrIcon;
	entry_ref mMailboxRef;
	NewMailMap mNewMailMap;
};

#endif
