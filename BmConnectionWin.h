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

#define BM_POPWIN_FETCHMSGS		'bmca'
						// sent from App BmConnectionWin in order to start connection
#define BM_POPWIN_DONE				'bmcb'
						// sent from BmConnectionWin to app when a connection has finished

// -----------------------------------------------
class BmConnectionWin : public MWindow {
	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

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
	typedef map<string, BmConnectionWinInfo*> ConnectionMap;
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
