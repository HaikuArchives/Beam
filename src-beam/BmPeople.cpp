/*
	BmPeople.cpp

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


#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>

#include "split.hh"
using namespace regexx;

#include "BmMailFolderList.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPersonInfo
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	AddEmails()
		-	
\*------------------------------------------------------------------------------*/
void BmPersonInfo::AddEmails( const BmStringVect& mails) {
	Regexx rx;
	for( uint32 m=0; m<mails.size(); ++m) {
		if (!rx.exec( mails[m], "^\\s*$")) {
			bool found=false;
			for( uint32 i=0; i<emails.size(); ++i) {
				if (emails[i] == mails[m])
					found=true;							// avoid duplicate entries
			}
			if (!found)
				emails.push_back( mails[m]);
		}
	}
}

/********************************************************************************\
	BmPerson
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmPerson()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmPerson::BmPerson( BmPeopleList* model, const node_ref& nref, const BmString& name,
						  const BmString& nick, const BmString& email, const BmString& groups,
						  bool foreign) 
	:	inherited( (BmString()<<nref.node<<"_"<<nref.device).String(), model, (BmListModelItem*)NULL)
	,	mName( name)
	,	mNick( nick)
	,	mNodeRef( nref)
	,	mIsForeign( foreign)
{
	AddEmail( email);
	
	BmStringVect grpVect;
	split( ",", groups, grpVect);
	// we trim leading/trailing whitespace from group-names and drop empty groups:
	Regexx rx;
	mGroups.reserve( grpVect.size());
	for( uint32 i=0; i<grpVect.size(); ++i) {
		if (rx.exec( grpVect[i], "^\\s*(.+?)\\s*$"))
			mGroups.push_back( rx.match[0].atom[0]);
		else if (!rx.exec( grpVect[i], "^\\s*$"))
			mGroups.push_back( grpVect[i]);
	}
}

/*------------------------------------------------------------------------------*\
	~BmPerson()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmPerson::~BmPerson() {
}

/*------------------------------------------------------------------------------*\
	AddEmail()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddEmail( const BmString& em) {
	Regexx rx;
	if (!rx.exec( em, "^\\s*$")) {
		for( uint32 i=0; i<mEmails.size(); ++i)
			if (mEmails[i] == em)
				return;							// avoid duplicate entries
		mEmails.push_back( em);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToNickMap( BmPersonMap& nickMap) const {
	if (!mIsForeign && mNick.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = nickMap[GenerateSortkeyFor( mNick)];
		personInfo.AddEmails( mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mNick;
		else
			personInfo.name = mNick + " (" + mEmails[0] + ")";
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToForeignMap( BmPersonMap& foreignMap) const {
	if (mIsForeign && mName.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = foreignMap[GenerateSortkeyFor( mName)];
		personInfo.AddEmails( mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = mName + " (" + mEmails[0] + ")";
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToNoGroupMap( BmPersonMap& noGroupMap) const {
	if (!mIsForeign && mGroups.empty() && mName.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = noGroupMap[GenerateSortkeyFor( mName)];
		personInfo.AddEmails( mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = mName + " (" + mEmails[0] + ")";
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToAllPeopleMap( BmPersonMap& allPeopleMap) const {
	if (mName.Length() && !mEmails.empty()) {
		for( uint32 i=0; i<mEmails.size(); ++i) {
			BmPersonInfo& personInfo = allPeopleMap[GenerateSortkeyFor( mName+mEmails[i])];
			personInfo.name = mName + " (" + mEmails[i] + ")";
			BmStringVect mails;
			mails.push_back( mEmails[i]);
			personInfo.AddEmails( mails);
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToGroupMap( BmGroupMap& groupMap) const {
	if (mIsForeign)
		return;
	for( uint32 i=0; i<mGroups.size(); ++i) {
		BmGroupInfo& groupInfo = groupMap[GenerateSortkeyFor( mGroups[i])];
		groupInfo.name = mGroups[i];
		BmPersonInfo& personInfo = groupInfo.personMap[GenerateSortkeyFor( mName)];
		personInfo.AddEmails( mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = mName + " (" + mEmails[0] + ")";
	}
}



/********************************************************************************\
	BmPeopleList
\********************************************************************************/

BmRef< BmPeopleList> BmPeopleList::theInstance( NULL);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by fetching all people
\*------------------------------------------------------------------------------*/
BmPeopleList* BmPeopleList::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmPeopleList();
		theInstance->StartJobInThisThread();
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmPeopleList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmPeopleList::BmPeopleList()
	:	inherited( "PeopleList") 
{
}

/*------------------------------------------------------------------------------*\
	~BmPeopleList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPeopleList::~BmPeopleList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	AddPeopleToMenu()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::AddPeopleToMenu( BMenu* menu, const BMessage& templateMsg,
												const char* addrField) {
	if (!menu)
		return;
	BmPersonMap allPeopleMap;
	BmPersonMap foreignMap;
	BmPersonMap nickMap;
	BmPersonMap noGroupMap;
	BmGroupMap groupMap;
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPerson* person = dynamic_cast< BmPerson*>( iter->second.Get());
		if (person) {
			person->AddToGroupMap( groupMap);
			person->AddToNoGroupMap( noGroupMap);
			person->AddToNickMap( nickMap);
			person->AddToForeignMap( foreignMap);
			person->AddToAllPeopleMap( allPeopleMap);
		}
	}
	BFont font;
	menu->GetFont( &font);
	BMenu* subMenu = CreateSubmenuForPersonMap( nickMap, templateMsg, addrField,
															  "Nicknames", &font);
	menu->AddItem( subMenu);
	menu->AddSeparatorItem();
	BmGroupMap::const_iterator group;
	for( group = groupMap.begin(); group != groupMap.end(); ++group) {
		subMenu = CreateSubmenuForPersonMap( group->second.personMap, 
														 templateMsg, addrField,
														 BmString("Group ")<<group->second.name, &font, true);
		menu->AddItem( subMenu);
	}
	subMenu = CreateSubmenuForPersonMap( noGroupMap, templateMsg, addrField,
													 "(Not in any Group)", &font);
	menu->AddItem( subMenu);
	menu->AddSeparatorItem();
	subMenu = CreateSubmenuForPersonMap( foreignMap, templateMsg, addrField,
													 "(Not in ~/People)", &font);
	menu->AddItem( subMenu);
	menu->AddSeparatorItem();
	subMenu = CreateSubmenuForPersonMap( allPeopleMap, templateMsg, addrField,
													 "All People", &font);
	menu->AddItem( subMenu);
}

/*------------------------------------------------------------------------------*\
	CreateSubmenuForPersonMap()
		-	
\*------------------------------------------------------------------------------*/
BMenu* BmPeopleList::CreateSubmenuForPersonMap( const BmPersonMap& personMap, 
																const BMessage& templateMsg,
																const char* addrField,
																BmString label, BFont* font,
																bool createAllEntry) {
	BMessage* msg;
	BMenu* subMenu = new BMenu( label.String());
	subMenu->SetFont( font);
	BmPersonMap::const_iterator person;
	BmString allAddrs;
	for( person = personMap.begin(); person != personMap.end(); ++person) {
		const BmPersonInfo& info = person->second;
		if (createAllEntry) {
			if (person==personMap.begin())
				allAddrs << info.emails[0];
			else
				allAddrs << ", " << info.emails[0];
		}
		uint32 numMails = info.emails.size();
		if (numMails>1) {
			BMenu* addrMenu = new BMenu( info.name.String());
			addrMenu->SetFont( font);
			subMenu->AddItem( addrMenu);
			for( uint32 i=0; i<numMails; ++i) {
				msg = new BMessage( templateMsg);
				msg->AddString( addrField, info.emails[i].String());
				addrMenu->AddItem( new BMenuItem( info.emails[i].String(), msg));
			}
		} else {
			msg = new BMessage( templateMsg);
			msg->AddString( addrField, info.emails[0].String());
			subMenu->AddItem( new BMenuItem( info.name.String(), msg));
		}
	}
	if (createAllEntry) {
		msg = new BMessage( templateMsg);
		msg->AddString( addrField, allAddrs.String());
		subMenu->AddSeparatorItem();
		subMenu->AddItem( new BMenuItem( "<Add Complete Group>", msg));
	}
	return subMenu;
}

/*------------------------------------------------------------------------------*\
	QueryForNewMails()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::InitializeItems() {
	int32 count, peopleCount=0;
	status_t err;
	dirent* dent;
	node_ref nref;
	entry_ref eref;
	char buf[4096];
	BNode node;
	BmString name;
	BmString nick;
	BmString email;
	BmString groups;
	
	BmString peopleFolder = TheResources->HomePath + "/People";

	BmAutolock lock( mModelLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ":InitializeItems(): Unable to get lock");

	BM_LOG2( BM_LogUtil, "Start of people-query");
	(err = mPeopleQuery.SetVolume( &TheResources->MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BmString("SetVolume(): ") << strerror(err));
	(err = mPeopleQuery.SetPredicate( "META:name == '**'")) == B_OK
													|| BM_THROW_RUNTIME( BmString("SetPredicate(): ") << strerror(err));
	(err = mPeopleQuery.SetTarget( BMessenger( TheMailMonitor))) == B_OK
													|| BM_THROW_RUNTIME( BmString("BmPeopleList::InitializeItems(): could not set query target.\n\nError:") << strerror(err));
	(err = mPeopleQuery.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BmString("Fetch(): ") << strerror(err));
	while ((count = mPeopleQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			peopleCount++;
			nref.device = dent->d_pdev;
			nref.node = dent->d_ino;
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name( dent->d_name);
			if (node.SetTo( &eref) == B_OK) {
				name = nick = email = "";
				BmReadStringAttr( &node, "META:name", name);
				BmReadStringAttr( &node, "META:nickname", nick);
				BmReadStringAttr( &node, "META:email", email);
				BmReadStringAttr( &node, "META:group", groups);
				BEntry entry( &eref);
				BPath path;
				entry.GetPath( &path);
				bool foreign = ThePrefs->GetBool( "LookForPeopleOnlyInPeopleFolder", true);
				if (path.InitCheck()==B_OK) {
					BmString personPath( path.Path());
					if (personPath.ICompare( peopleFolder, peopleFolder.Length()) == 0)
						foreign = false;
				}
				BmPerson* newPerson = new BmPerson( this, nref, name, nick, email, groups, foreign);
				for( int c=1; c<9; ++c) {
					email.Truncate(0);
					BmString emailAttr("META:email");
					BmReadStringAttr( &node, (emailAttr<<c).String(), email);
					if (email.Length())
						newPerson->AddEmail( email);
				}
				AddItemToList( newPerson, NULL);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogUtil, BmString("End of people-query (") << peopleCount << " people found)");
}
