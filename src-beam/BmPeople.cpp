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


//#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Messenger.h>
#include <Path.h>

#include "BmMailFolderList.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPerson
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmPerson()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmPerson::BmPerson( BmPeopleList* model, const node_ref& nref, const BString& name,
						  const BString& nick, const BString& email) 
	:	inherited( GenerateSortkeyFor( GenerateDisplayName( name, nick, email)).String(), 
					  model, (BmListModelItem*)NULL)
	,	mName( name)
	,	mNick( nick)
	,	mEmail( email)
	,	mDisplayName( GenerateDisplayName( name, nick, email))
	,	mNodeRef( nref)
{
}

/*------------------------------------------------------------------------------*\
	~BmPerson()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmPerson::~BmPerson() {
}

/*------------------------------------------------------------------------------*\
	~BmPerson()
		-	d'tor
\*------------------------------------------------------------------------------*/
BString BmPerson::GenerateDisplayName( const BString& name, const BString& nick,
													const BString& email) {
	BString displayName = ThePrefs->GetString( "PeopleDisplayStr", "nick/name (email)");
	ReplaceSubstringWith( displayName, "nick/name", nick.Length() ? nick : name);
	ReplaceSubstringWith( displayName, "name/nick", name.Length() ? name : nick);
	ReplaceSubstringWith( displayName, "nick", nick);
	ReplaceSubstringWith( displayName, "name", name);
	ReplaceSubstringWith( displayName, "email", email);
	return displayName;
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
	BString name;
	BString nick;
	BString email;

	BmAutolock lock( mModelLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ":InitializeItems(): Unable to get lock");

	BM_LOG2( BM_LogUtil, "Start of people-query");
	(err = mPeopleQuery.SetVolume( &TheResources->MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BString("SetVolume(): ") << strerror(err));
	(err = mPeopleQuery.SetPredicate( "META:email == '*'")) == B_OK
													|| BM_THROW_RUNTIME( BString("SetPredicate(): ") << strerror(err));
	(err = mPeopleQuery.SetTarget( BMessenger( TheMailMonitor))) == B_OK
													|| BM_THROW_RUNTIME( BString("BmPeopleList::InitializeItems(): could not set query target.\n\nError:") << strerror(err));
	(err = mPeopleQuery.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BString("Fetch(): ") << strerror(err));
	while ((count = mPeopleQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			peopleCount++;
			nref.device = dent->d_pdev;
			nref.node = dent->d_pino;
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name( dent->d_name);
			if (node.SetTo( &eref) == B_OK) {
				name = nick = email = "";
				node.ReadAttrString( "META:name", &name);
				node.ReadAttrString( "META:nickname", &nick);
				node.ReadAttrString( "META:email", &email);
				BmPerson* newPerson = new BmPerson( this, nref, name, nick, email);
				AddItemToList( newPerson, NULL);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogUtil, BString("End of people-query (") << peopleCount << " people found)");
}

