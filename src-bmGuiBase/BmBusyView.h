/*
	BmBusyView.h
		$Id$
*/

#ifndef _BmBusyView_h
#define _BmBusyView_h

#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>

class BmBusyView : public BView
{
	typedef BView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmBusyView( BRect frame);
	~BmBusyView();

	// native methods:
	void SetBusy();
	void UnsetBusy();
	void Pulse();

	// overrides of BStringView base:
	void Draw( BRect bounds);
	void MessageReceived( BMessage* msg);

private:
	BRect mCachedBounds;
	BMessageRunner* mMsgRunner;
	int16 mBusyCount;
	int32 mCurrState;

	// Hide copy-constructor and assignment:
	BmBusyView( const BmBusyView&);
	BmBusyView operator=( const BmBusyView&);
};


#endif
