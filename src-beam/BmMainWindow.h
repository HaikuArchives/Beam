/*
	BmMainWindow.h
		$Id$
*/

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <MWindow.h>

class BmMainWindow : public MWindow
{
	typedef MWindow inherited;
public:
	BmMainWindow();
	~BmMainWindow();
	void MessageReceived(BMessage*);
};

#endif
