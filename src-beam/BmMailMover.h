/*
	BmMailMover.h

		$Id$
*/

#ifndef _BmMailMover_h
#define _BmMailMover_h

#include <memory>

#include <Message.h>

#include "BmDataModel.h"
#include "BmUtil.h"

class BmMailFolder;

/*------------------------------------------------------------------------------*\
	BmMailMover
		-	implements the moving of mails inside the file-system
		-	in general, each BmMailMover is started as a thread which exits when the
			moving-operation has ended
\*------------------------------------------------------------------------------*/
class BmMailMover : public BmJobModel {
	typedef BmJobModel inherited;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_MOVER = 		"bm:mover";
	static const char* const MSG_DELTA = 		"bm:delta";
	static const char* const MSG_TRAILING = 	"bm:trailing";
	static const char* const MSG_LEADING = 	"bm:leading";

	BmMailMover( const BString& name, BList* refList, BmMailFolder* destFolder);
	virtual ~BmMailMover();

	inline BString Name() const			{ return ModelName(); }

	bool StartJob();

private:
	void UpdateStatus( const float delta, const char* filename, const char* currentCount);
	
	BList* mRefList;
	BmMailFolder* mDestFolder;

	// Hide copy-constructor and assignment:
	BmMailMover( const BmMailMover&);
	BmMailMover operator=( const BmMailMover&);
};

#endif
