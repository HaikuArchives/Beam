/*
	BmMailMover.h

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#ifndef _BmMailMover_h
#define _BmMailMover_h

#include <memory>

#include <Message.h>

#include "BmDataModel.h"
#include "BmUtil.h"

#define BM_JOBWIN_MOVEMAILS			'bmec'
						// sent to JobMetaController in order to move mails

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
	static const char* const MSG_MOVER;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;
	static const char* const MSG_REFS;

	BmMailMover( const BmString& name, BList* refList, BmMailFolder* destFolder);
	virtual ~BmMailMover();

	inline BmString Name() const			{ return ModelName(); }

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
