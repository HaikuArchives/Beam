/*
	BmConnectionWin.h
		$Id$
*/

#ifndef _BmPopperWin_h
#define _BmPopperWin_h

#include <map>
#include <liblayout/MWindow.h>
#include <liblayout/VGroup.h>
#include "BmPopper.h"

#define BM_POPWIN_DONE	'bmca'

// for testing:
#define BM_MSG_NOCH_EINER 'bmt1'


// -----------------------------------------------
class BmConnectionWin : public MWindow {
	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

	static bool BMPREF_DYNAMIC_INTERFACES;

	struct BmConnectionWinInfo {
		MView* interface;
		thread_id thread;
		BStatusBar* statBar;
		BStatusBar* mailBar;
		BmConnectionWinInfo( thread_id t, MView* i, BStatusBar* sb, BStatusBar* mb)
			: interface(i)
			, thread(t)
			, statBar(sb)
			, mailBar(mb)
			{}
	};
	typedef map<BString, BmConnectionWinInfo*> ConnectionMap;
public:
	static bool IsConnectionWinAlive;

	BmConnectionWin( const char* title, BLooper *invoker=NULL);
	virtual ~BmConnectionWin();

	bool QuitRequested();
	virtual void MessageReceived(BMessage *message);

private:
	ConnectionMap mActiveConnections;
	VGroup *mOuterGroup;
	BLooper *mInvokingLooper;

	void AddPopper( BmPopAccount* popAccount);
	MView *AddPopperInterface( const char* name, BStatusBar*&, BStatusBar*&);
	void UpdatePopperInterface( BMessage* msg);
	void RemovePopper( const char* name);
	void RemovePopperInterface( BmConnectionWinInfo* interface);
};

#endif
