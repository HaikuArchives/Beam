/*
	BmApp.h
		$Id$
*/

#ifndef _Beam_h
#define _Beam_h

#include "BmApp.h"

class BmWindow;

class BeamApp : public BmApplication
{
	typedef BmApplication inherited;

public:
	BeamApp();
	~BeamApp();

	// overrides of BmApplication
	thread_id Run();
	void MessageReceived(BMessage*);
	void ReadyToRun();
	void RefsReceived( BMessage* msg);

private:
	status_t mInitCheck;
	BmWindow* mMailWin;

	inline status_t InitCheck() 			{ return mInitCheck; }
};

extern BeamApp* beamApp;

#endif
