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
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>

#include "split.hh"
using namespace regexx;

#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPersonInfo
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	AddEmails()
		-	
\*------------------------------------------------------------------------------*/
void BmPersonInfo::AddEmails( const BmString& name, const BmStringVect& mails) {
	Regexx rx;
	for( uint32 m=0; m<mails.size(); ++m) {
		if (!rx.exec( mails[m], "^\\s*$")) {
			BmString mailAddr = BmPerson::GenerateMailAddr( name, mails[m]);
			bool found=false;
			for( uint32 i=0; i<emails.size(); ++i) {
				if (emails[i] == mailAddr)
					found=true;							// avoid duplicate entries
			}
			if (!found)
				emails.push_back( mailAddr);
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
						  const entry_ref& eref, bool foreign) 
	:	inherited( (BmString()<<nref.node).String(), model, (BmListModelItem*)NULL)
	,	mName( name)
	,	mNick( nick)
	,	mNodeRef( nref)
	,	mEntryRef( eref)
	,	mIsForeign( foreign)
{
	if (!AddEmail( email))
		return;			// empty email, we skip the rest
	
	BmStringVect grpVect;
	split( ",", groups, grpVect);
	// we trim leading/trailing whitespace from group-names and drop groups with
	// empty names:
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
bool BmPerson::AddEmail( const BmString& em) {
	Regexx rx;
	if (!rx.exec( em, "^\\s*$")) {
		for( uint32 i=0; i<mEmails.size(); ++i)
			if (mEmails[i] == em)
				return true;					// avoid duplicate entries
		mEmails.push_back( em);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	CreateNewEmail()
		-	
\*------------------------------------------------------------------------------*/
bool BmPerson::CreateNewEmail( const BmString& em) {
	uint32 count = mEmails.size();
	if (count >= 5)
		// can only handle up two 5 emails (even MrPeeps doesn't do more)
		return false;
	if (AddEmail(em)) {
		count++;
		BNode node(&mEntryRef);
		BmString attrName("META:email");
		if (count > 1)
			attrName << count;
		node.WriteAttr( attrName.String(), B_STRING_TYPE, 0, 
							 em.String(), em.Length()+1);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	HasEmail()
		-	
\*------------------------------------------------------------------------------*/
bool BmPerson::HasEmail( const BmString& em) const {
	for( uint32 i=0; i<mEmails.size(); ++i)
		if (mEmails[i].ICompare(em) == 0)
			return true;
	return false;
}

/*------------------------------------------------------------------------------*\
	GenerateMailAddr()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPerson::GenerateMailAddr( const BmString& name, 
												 const BmString& email,
												 const BmString& nick) {
	BmString addr;
	if (nick.Length() > 0)
		addr << "(" << nick << ") ";
	if (name.Length() > 0)
		addr = BmAddress::QuotedPhrase(name) << " <" << email << ">";
	else
		addr = email;
	return addr;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToNickMap( BmPersonMap& nickMap) const {
	if (!mIsForeign && mNick.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = nickMap[GenerateSortkeyFor( mNick)];
		personInfo.AddEmails( mName, mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mNick;
		else
			personInfo.name = mNick + ": " + GenerateMailAddr( mName, mEmails[0]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToForeignMap( BmPersonMap& foreignMap) const {
	if (mIsForeign && mName.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = foreignMap[GenerateSortkeyFor( mName)];
		personInfo.AddEmails( mName, mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = GenerateMailAddr( mName, mEmails[0]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToNoGroupMap( BmPersonMap& noGroupMap) const {
	if (!mIsForeign && mGroups.empty() && mName.Length() && !mEmails.empty()) {
		BmPersonInfo& personInfo = noGroupMap[GenerateSortkeyFor( mName)];
		personInfo.AddEmails( mName, mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = GenerateMailAddr( mName, mEmails[0]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPerson::AddToAllPeopleMap( BmPersonMap& allPeopleMap) const {
	if (mName.Length() || !mEmails.empty()) {
		for( uint32 i=0; i<mEmails.size(); ++i) {
			BmPersonInfo& personInfo = allPeopleMap[GenerateSortkeyFor( mName+mEmails[i])];
			personInfo.name = GenerateMailAddr( mName, mEmails[i]);
			BmStringVect mails;
			mails.push_back( mEmails[i]);
			personInfo.AddEmails( mName, mails);
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
		personInfo.AddEmails( mName, mEmails);
		if (personInfo.emails.size() > 1)
			personInfo.name = mName;
		else
			personInfo.name = GenerateMailAddr( mName, mEmails[0]);
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
	if (!theInstance)
		theInstance = new BmPeopleList();
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmPeopleList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmPeopleList::BmPeopleList()
	:	inherited( "PeopleList", BM_LogApp)
{
	NeedControllersToContinue( false);
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
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
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
														 BmString("Group ")<<group->second.name, 
														 &font, true);
		menu->AddItem( subMenu);
	}
	subMenu = CreateSubmenuForPersonMap( noGroupMap, templateMsg, addrField,
													 "(Not in any Group)", &font);
	menu->AddItem( subMenu);
	menu->AddSeparatorItem();
	if (foreignMap.size() > 0) {
		subMenu = CreateSubmenuForPersonMap( foreignMap, templateMsg, addrField,
														 "(Not in People-Folder)", &font);
		menu->AddItem( subMenu);
		menu->AddSeparatorItem();
	}
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
			for( uint32 i=0; i<numMails; ++i) {
				msg = new BMessage( templateMsg);
				msg->AddString( addrField, info.emails[i].String());
				addrMenu->AddItem( new BMenuItem( info.emails[i].String(), msg));
			}
			msg = new BMessage( templateMsg);
			msg->AddString( addrField, info.emails[0].String());
			subMenu->AddItem( new BMenuItem( addrMenu, msg));
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
	FindPersonByNodeRef()
		-	
\*------------------------------------------------------------------------------*/
BmRef< BmPerson> BmPeopleList::FindPersonByNodeRef( const node_ref& nref) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPerson* person = dynamic_cast< BmPerson*>( iter->second.Get());
		if (person && person->NodeRef() == nref)
			return person;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindPersonByEmail()
		-	
\*------------------------------------------------------------------------------*/
BmRef< BmPerson> BmPeopleList::FindPersonByEmail( const BmString& email) {
	if (!email.Length())
		return NULL;
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPerson* person = dynamic_cast< BmPerson*>( iter->second.Get());
		if (person && person->HasEmail(email))
			return person;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	FindPersonByName()
		-	
\*------------------------------------------------------------------------------*/
BmRef< BmPerson> BmPeopleList::FindPersonByName( const BmString& name) {
	if (!name.Length())
		return NULL;
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPerson* person = dynamic_cast< BmPerson*>( iter->second.Get());
		if (person && person->Name().ICompare(name) == 0)
			return person;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	CreateNewPerson( name, email)
		-	
\*------------------------------------------------------------------------------*/
status_t BmPeopleList::CreateNewPerson( const BmString& name, 
													 const BmString& email,
													 entry_ref *eref)
{
	if (!email.Length())
		return B_BAD_VALUE;
	BmString fileName;
	BmString peopleFolder = ThePrefs->GetString( "PeopleFolder", 
																"/boot/home/people");
	fileName << peopleFolder << "/" << (name.Length() ? name : email);
	BFile peopleFile(fileName.String(), B_CREATE_FILE | B_READ_WRITE);
	status_t res = peopleFile.InitCheck();
	if (res == B_OK) {
		peopleFile.WriteAttr( "META:name", B_STRING_TYPE, 0, 
									 name.String(), name.Length()+1);
		peopleFile.WriteAttr( "META:email", B_STRING_TYPE, 0, 
									 email.String(), email.Length()+1);
		BNodeInfo nodeInfo( &peopleFile);
		nodeInfo.SetType( "application/x-person");
		if (eref)
			res = get_ref_for_path(fileName.String(), eref);
	}
	return res;
}

/*------------------------------------------------------------------------------*\
	AddPerson( nref, eref)
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::AddPerson( const node_ref& nref, const entry_ref& eref) {
	BNode node;
	BmString name;
	BmString nick;
	BmString email;
	BmString groups;
	if (!FindPersonByNodeRef( nref) && node.SetTo( &eref) == B_OK) {
		BmString peopleFolder = ThePrefs->GetString( "PeopleFolder", 
																	"/boot/home/people");
		peopleFolder << "/";
		BEntry entry( &eref);
		BPath path;
		entry.GetPath( &path);
		bool foreign = false;
		if (path.InitCheck()==B_OK) {
			BmString personPath( path.Path());
			if (personPath.ICompare( peopleFolder, peopleFolder.Length()) != 0)
				foreign = true;
		}
		if (foreign && ThePrefs->GetBool( "LookForPeopleOnlyInPeopleFolder", true))
			return;

		name = nick = email = "";
		BmReadStringAttr( &node, "META:name", name);
		BmReadStringAttr( &node, "META:nickname", nick);
		BmReadStringAttr( &node, "META:email", email);
		BmReadStringAttr( &node, "META:group", groups);
		BmPerson* newPerson 
			= new BmPerson( this, nref, name, nick, email, groups, eref, foreign);
		for( int c=1; c<9; ++c) {
			email.Truncate(0);
			BmString emailAttr("META:email");
			BmReadStringAttr( &node, (emailAttr<<c).String(), email);
			if (email.Length())
				newPerson->AddEmail( email);
		}
		AddItemToList( newPerson, NULL);
	}
}

/*------------------------------------------------------------------------------*\
	RemovePerson( nref)
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::RemovePerson( const node_ref& nref) {
	BmRef<BmPerson> person = FindPersonByNodeRef( nref);
	if (person) {
		RemoveItemFromList( person.Get());
	}
}

/*------------------------------------------------------------------------------*\
	GetEmailsFromPeopleFile()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::GetEmailsFromPeopleFile( const entry_ref& eref,
														  BmStringVect& emails) {
	emails.clear();
	BNode personNode( &eref);
	if (personNode.InitCheck() != B_OK)
		return;

	BmString name;
	BmReadStringAttr( &personNode, "META:name", name);
	BmString addrSpec;
	BmReadStringAttr( &personNode, "META:email", addrSpec);
	BmString addr;
	if (name.Length()
	&& ThePrefs->GetBool( "AddPeopleNameToMailAddr", true))
		addr << '"' << name << '"' << " <" << addrSpec << ">";
	else
		addr = addrSpec;
	if (addr.Length())
		emails.push_back( addr);
	for( int c=1; c<9; ++c) {
		addrSpec.Truncate(0);
		BmString emailAttr("META:email");
		BmReadStringAttr( &personNode, (emailAttr<<c).String(), addrSpec);
		if (addrSpec.Length() > 0) {
			addr.Truncate(0);
			if (name.Length()
			&& ThePrefs->GetBool( "AddPeopleNameToMailAddr", true))
				addr << '"' << name << '"' << " <" << addrSpec << ">";
			else
				addr = addrSpec;
			if (addr.Length())
				emails.push_back( addr);
		}
	}
}

/*------------------------------------------------------------------------------*\
	InitializeItems()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleList::InitializeItems() {
	int32 count, peopleCount=0;
	status_t err;
	dirent* dent;
	node_ref nref;
	entry_ref eref;
	char buf[4096];
	BVolume peopleVolume;
	BEntry entry;
	
	// determine the volume of the People-folder:
	BmString peopleFolder = ThePrefs->GetString( "PeopleFolder", 
																"/boot/home/people");
	if ((err = entry.SetTo( peopleFolder.String(), true)) != B_OK)
		BM_THROW_RUNTIME( "Sorry, could not get entry for people-folder !?!");
	if (!entry.Exists())
		return;			// people-folder doesn't exist, we stop right here!

	if (entry.GetNodeRef( &nref) != B_OK)
		BM_THROW_RUNTIME( "Sorry, could not determine people-folder-volume !?!");
	peopleVolume = nref.device;

	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":InitializeItems(): Unable to get lock"
		);

	BM_LOG2( BM_LogApp, "Start of people-query");
	if ((err = mPeopleQuery.SetVolume( &peopleVolume)) != B_OK)
		BM_THROW_RUNTIME( BmString("SetVolume(): ") << strerror(err));
	if ((err = mPeopleQuery.SetPredicate( "META:email = '*'")) != B_OK)
		BM_THROW_RUNTIME( BmString("SetPredicate(): ") << strerror(err));
	if ((err = mPeopleQuery.SetTarget( BMessenger( ThePeopleMonitor))) != B_OK)
		BM_THROW_RUNTIME( BmString("BmPeopleList::InitializeItems(): could not set query target.\n\nError:") << strerror(err));
	if ((err = mPeopleQuery.Fetch()) != B_OK)
		BM_THROW_RUNTIME( BmString("Fetch(): ") << strerror(err));
	while ((count = mPeopleQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			peopleCount++;
			nref.device = dent->d_pdev;
			nref.node = dent->d_ino;
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name( dent->d_name);

			AddPerson( nref, eref);

			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogApp, BmString("End of people-query (") << peopleCount << " people found)");
	mInitCheck = B_OK;
}



/********************************************************************************\
	BmPeopleMonitor
\********************************************************************************/

BmPeopleMonitor* BmPeopleMonitor::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmPeopleMonitor* BmPeopleMonitor::CreateInstance() {
	if (!theInstance)
		theInstance = new BmPeopleMonitor();
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	BmPeopleMonitor()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmPeopleMonitor::BmPeopleMonitor()
	:	BLooper("PeopleMonitor")
	,	mCounter( 0)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleMonitor::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_QUERY_UPDATE: {
				if (ThePeopleList)
					HandleQueryUpdateMsg( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PeopleMonitor: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleQueryUpdateMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleMonitor::HandleQueryUpdateMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	status_t err;
	node_ref nref;
	BM_LOG2( BM_LogMailTracking, BmString("QueryUpdateMessage nr.") << ++mCounter << " received.");
	try {
		switch( opcode) {
			case B_ENTRY_CREATED: {
				entry_ref eref;
				const char* name;
				if ((err = msg->FindInt64( "directory", &eref.directory)) != B_OK)
					BM_THROW_RUNTIME( "Field 'directory' not found in msg !?!");
				if ((err = msg->FindInt32( "device", &nref.device)) != B_OK)
					BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				if ((err = msg->FindInt64( "node", &nref.node)) != B_OK)
					BM_THROW_RUNTIME( BmString("Field 'node' not found in msg !?!"));
				if ((err = msg->FindString( "name", &name)) != B_OK)
					BM_THROW_RUNTIME( BmString("Field 'name' not found in msg !?!"));
				eref.set_name( name);
				eref.device = nref.device;
				ThePeopleList->AddPerson( nref, eref);
				break;
			}
			case B_ENTRY_REMOVED: {
				if ((err = msg->FindInt32( "device", &nref.device)) != B_OK)
					BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				if ((err = msg->FindInt64( "node", &nref.node)) != B_OK)
					BM_THROW_RUNTIME( BmString("Field 'node' not found in msg !?!"));
				ThePeopleList->RemovePerson( nref);
				break;
			}
		}
	} catch( BM_error &e) {
		BM_LOGERR( e.what());
	}
}
