/*
	BmPeople.h

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


#ifndef _BmPeople_h
#define _BmPeople_h

#include <List.h>
#include <Locker.h>
#include <String.h>

#include <Locker.h>
#include <Looper.h>
#include <Query.h>

#include "BmDataModel.h"

class BMenu;

class BmPerson;
class BmPeopleList;

typedef vector< BString> BmStringVect;

struct BmPersonInfo {
	BString nick;
	BString name;
	BmStringVect emails;
	BmPersonInfo()								{}
	BmPersonInfo( const BString& nm, const BString& nk, const BString& em)
		: name( nm) 							{	emails.push_back( em); }
};
typedef map< BString, BmPersonInfo> BmPersonMap;

struct BmGroupInfo {
	BString name;
	BmPersonMap personMap;
};
typedef map< BString, BmGroupInfo> BmGroupMap;

/*------------------------------------------------------------------------------*\
	BmPerson
		-	holds information about one specific Person
\*------------------------------------------------------------------------------*/
class BmPerson : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmPerson( BmPeopleList* model, const node_ref& nref, const BString& name,
				 const BString& nick, const BString& email, const BString& groups,
				 bool foreign);
	virtual ~BmPerson();
	
	// native methods:
	void AddToGroupMap( BmGroupMap& groupMap) const;
	void AddToNoGroupMap( BmPersonMap& noGroupMap) const;
	void AddToNickMap( BmPersonMap& nickMap) const;
	void AddToForeignMap( BmPersonMap& foreignMap) const;

	void AddEmail( const BString &em);

	// getters:
	inline const node_ref &NodeRef() const		{ return mNodeRef; }

	// setters:

	int16 ArchiveVersion() const			{ return 0; }

private:

	// native methods:
	static BString GenerateDisplayName( const BString& name, const BString& nick,
													const BString& email);

	BmPerson();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmPerson( const BmPerson&);
	BmPerson operator=( const BmPerson&);

	BString mNick;
	BString mName;
	BmStringVect mEmails;
	BmStringVect mGroups;
	node_ref mNodeRef;
	bool mIsForeign;							// true if person does not live under ~/People
};



/*------------------------------------------------------------------------------*\
	BmPeopleList 
		-	holds list of all known People
\*------------------------------------------------------------------------------*/
class BmPeopleList : public BmListModel {
	typedef BmListModel inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmPeopleList* CreateInstance();
	~BmPeopleList();
	
	// native methods:
	void AddPeopleToMenu( BMenu* menu, const BMessage& templateMsg, 
								 const char* addrField);

	// overrides of listmodel base:
	const BString SettingsFileName()		{ return ""; }
	int16 ArchiveVersion() const			{ return 0; }

	static BmRef<BmPeopleList> theInstance;

private:
	// native methods:
	BMenu* CreateSubmenuForPersonMap( const BmPersonMap& personMap, 
												 const BMessage& templateMsg,
												 const char* addrField,
												 BString label, BFont* font);

	// overrides of listmode base:
	void InitializeItems();

	// Hide default constructor, BmPeopleList is a Singleton
	BmPeopleList();
	// Hide copy-constructor and assignment:
	BmPeopleList( const BmPeopleList&);
	BmPeopleList operator=( const BmPeopleList&);
	
	BQuery mPeopleQuery;
};

#define ThePeopleList BmPeopleList::theInstance

#endif
