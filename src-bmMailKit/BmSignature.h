/*
	BmSignature.h

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


#ifndef _BmSignature_h
#define _BmSignature_h

#include <stdexcept>

#include <Archivable.h>
#include <String.h>

#include "BmDataModel.h"

class BmSignatureList;

/*------------------------------------------------------------------------------*\
	BmSignature 
		-	holds information about one signature
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmSignature : public BmListModelItem {
	typedef BmListModelItem inherited;
	// archivable components:
	static const char* const MSG_NAME = 		"bm:name";
	static const char* const MSG_DYNAMIC = 	"bm:dynamic";
	static const char* const MSG_CONTENT = 	"bm:content";
	static const char* const MSG_ENCODING = 	"bm:encoding";
	static const int16 nArchiveVersion = 1;

public:
	BmSignature( const char* name, BmSignatureList* model);
	BmSignature( BMessage* archive, BmSignatureList* model);
	virtual ~BmSignature();
	
	// native methods:
	BString GetSignatureString();

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BString &Content() const	{ return mContent; }
	inline bool Dynamic() const 				{ return mDynamic; }
	inline const BString &Name() const		{ return Key(); }
	inline uint32 Encoding() const			{ return mEncoding; }

	// setters:
	inline void Content( const BString &s) { mContent = s; TellModelItemUpdated( UPD_ALL); }
	inline void Dynamic( bool b) 				{ mDynamic = b;  TellModelItemUpdated( UPD_ALL); }
	inline void Encoding( uint32 i) 			{ mEncoding = i;  TellModelItemUpdated( UPD_ALL); }

private:
	BmSignature();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmSignature( const BmSignature&);
	BmSignature operator=( const BmSignature&);

	BString mContent;
	bool mDynamic;						// if mContents is static text or a script-call
	uint32 mEncoding;					// character-encoding of this sig
};


/*------------------------------------------------------------------------------*\
	BmSignatureList 
		-	holds list of all Signatures
\*------------------------------------------------------------------------------*/
class BmSignatureList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 1;

public:
	// creator-func, c'tors and d'tor:
	static BmSignatureList* CreateInstance();
	BmSignatureList();
	~BmSignatureList();
	
	// native methods:
	BString GetSignatureStringFor( const BString sigName);
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmSignatureList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmSignatureList( const BmSignatureList&);
	BmSignatureList operator=( const BmSignatureList&);
	
};

#define TheSignatureList BmSignatureList::theInstance

#endif
