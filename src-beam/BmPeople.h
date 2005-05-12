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

#include <vector>

#include <List.h>
#include <Locker.h>
#include "BmString.h"

#include <Locker.h>
#include <Looper.h>
#include <Query.h>

#include "BmDataModel.h"

class BMenu;

class BmPerson;
class BmPeopleList;

typedef vector< BmString> BmStringVect;

struct BmPersonInfo {
	BmString name;
	BmString nick;
	BmStringVect emails;
	BmPersonInfo()								{}
	BmPersonInfo( const BmString& nm, const BmString& nk, const BmString& em)
		: name( nm), nick( nk)				{	emails.push_back( em); }
	void AddEmails( const BmString& name, const BmStringVect& mails);
};
typedef map< BmString, BmPersonInfo> BmPersonMap;

struct BmGroupInfo {
	BmString name;
	BmPersonMap personMap;
};
typedef map< BmString, BmGroupInfo> BmGroupMap;

/*------------------------------------------------------------------------------*\
	BmPerson
		-	holds information about one specific Person
\*------------------------------------------------------------------------------*/
class BmPerson : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmPerson( BmPeopleList* model, const node_ref& nref, const BmString& name,
				 const BmString& nick, const BmString& email, const BmString& groups,
				 const entry_ref& eref, bool foreign);
	virtual ~BmPerson();
	
	// native methods:
	void AddToGroupMap( BmGroupMap& groupMap) const;
	void AddToNoGroupMap( BmPersonMap& noGroupMap) const;
	void AddToNickMap( BmPersonMap& nickMap) const;
	void AddToForeignMap( BmPersonMap& foreignMap) const;
	void AddToAllPeopleMap( BmPersonMap& allPeopleMap) const;
	//
	static BmString GenerateMailAddr( const BmString& name, 
												 const BmString& email);

	bool AddEmail( const BmString &em);
	bool CreateNewEmail( const BmString &em);
	bool HasEmail( const BmString &em) const;

	// getters:
	inline const node_ref &NodeRef() const		
													{ return mNodeRef; }
	inline const entry_ref &EntryRef() const		
													{ return mEntryRef; }
	inline const BmString &Name() const	{ return mName; }

	// setters:

	int16 ArchiveVersion() const			{ return 0; }

private:

	// native methods:
	BmPerson();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmPerson( const BmPerson&);
	BmPerson operator=( const BmPerson&);

	BmString mNick;
	BmString mName;
	BmStringVect mEmails;
	BmStringVect mGroups;
	node_ref mNodeRef;
	entry_ref mEntryRef;
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
	BmRef< BmPerson> FindPersonByNodeRef( const node_ref& nref);
	BmRef< BmPerson> FindPersonByEmail( const BmString& email);
	BmRef< BmPerson> FindPersonByName( const BmString& name);
	//
	void AddPerson( const node_ref& nref, const entry_ref& eref);
	void RemovePerson( const node_ref& nref);
	//
	status_t CreateNewPerson( const BmString& name, const BmString& email, 
									  entry_ref* eref=NULL);

	// overrides of listmodel base:
	const BmString SettingsFileName()	{ return ""; }
	int16 ArchiveVersion() const			{ return 0; }

	static BmRef<BmPeopleList> theInstance;

private:
	// native methods:
	BMenu* CreateSubmenuForPersonMap( const BmPersonMap& personMap, 
												 const BMessage& templateMsg,
												 const char* addrField,
												 BmString label, BFont* font,
												 bool createAllEntry=false);

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


/*------------------------------------------------------------------------------*\*\
	BmPeopleMonitor
		-	class 
\*------------------------------------------------------------------------------*/
class BmPeopleMonitor : public BLooper {
	typedef BLooper inherited;

public:
	// creator-func and c'tor:
	static BmPeopleMonitor* CreateInstance();
	BmPeopleMonitor();

	//	native methods:
	void HandleQueryUpdateMsg( BMessage* msg);

	// overrides of looper base:
	void MessageReceived( BMessage* msg);

	static BmPeopleMonitor* theInstance;
	
private:
	int32 mCounter;

	// Hide copy-constructor and assignment:
	BmPeopleMonitor( const BmPeopleMonitor&);
	BmPeopleMonitor operator=( const BmPeopleMonitor&);
};

#define ThePeopleMonitor BmPeopleMonitor::theInstance

#endif
