/*
	BmWindow.h
		$Id$
*/

#ifndef _BmWindow_h
#define _BmWindow_h

#include <String.h>

#include <MWindow.h>

class BmWindow : public MWindow
{
	typedef MWindow inherited;

	static const char* const MSG_FRAME = 		"bm:frm";

public:
	// creator-func, c'tors and d'tor:
	BmWindow( const char* statfileName, BRect frame, const char* title,
				 window_look look, window_feel feel, uint32 flags);
	~BmWindow();

	// native methods:
	virtual status_t ArchiveState( BMessage* archive) const;
	virtual status_t UnarchiveState( BMessage* archive);
	virtual bool ReadStateInfo();
	virtual bool WriteStateInfo();

	// overrides of BWindow-base:
	void Quit();

protected:
	BString mStatefileName;
};


#endif
