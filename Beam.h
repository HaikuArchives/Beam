/*
	BmApp.h
		$Id$
*/

#ifndef _Beam_h
#define _Beam_h

#include "BmApp.h"

class BeamApp : public BmApplication
{
	typedef BmApplication inherited;

public:
	BeamApp();
	~BeamApp();
	virtual thread_id Run();
	virtual void MessageReceived(BMessage*);
	virtual void ReadyToRun();

private:
	status_t mInitCheck;

	inline status_t InitCheck() 			{ return mInitCheck; }
};

extern BeamApp* beamApp;

#endif
