/*
	BmMailRefList.h
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


#ifndef _BmMailRefList_h
#define _BmMailRefList_h

#include <sys/stat.h>

#include "BmDataModel.h"

class BmMailFolder;
class BmMailRef;

/*------------------------------------------------------------------------------*\*\
	BmMailRefList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailRefList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 2;

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder);
	virtual ~BmMailRefList();

	// native methods:
	BmRef<BmMailRef> AddMailRef( entry_ref& eref, struct stat& st);
	void MarkCacheAsDirty();

	// overrides of list-model base:
	bool Store();
	bool StartJob();
	void RemoveController( BmController* controller);
	const BmString SettingsFileName();
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	
	// getters:
	inline bool NeedsCacheUpdate() const	{ return mNeedsCacheUpdate; }
	
	// setters:
	inline void MarkAsChanged() 			{ mNeedsStore = true; }

private:

	// native methods:
	void InitializeItems();
	void MyInstantiateItems( BDataIO* dataIO, int32 numChildren);

	// the following members will NOT be archived at all:
	BmWeakRef<BmMailFolder> mFolder;
	bool mNeedsCacheUpdate;
	BmString mSettingsFileName;

	// Hide copy-constructor and assignment:
	BmMailRefList( const BmMailRefList&);
	BmMailRefList operator=( const BmMailRefList&);
};


#endif
