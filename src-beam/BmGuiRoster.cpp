/*
	BmGuiRoster.cpp
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

#include <FindDirectory.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>

#include "split.hh"
using namespace regexx;

#include "TextEntryAlert.h"
#include "ListSelectionAlert.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMenuControllerBase.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmGuiRoster.h"
#include "BmSmtpAccount.h"
#include "BmSignature.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmGuiRoster::BmGuiRoster()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmGuiRoster::AskUserForPwd( const BmString& text, BmString& pwd) 
{
	// ask user about password:
	TextEntryAlert* alert = new TextEntryAlert( "Info needed", text.String(),
									 						  "", "Cancel", "OK");
	alert->SetFeel( B_FLOATING_APP_WINDOW_FEEL);
	alert->SetLook( B_FLOATING_WINDOW_LOOK);
	alert->TextEntryView()->HideTyping( true);
	alert->SetShortcut( 0, B_ESCAPE);
	BmString buf;
	int32 result = alert->Go( buf);
	if (result == 1) {
		pwd = buf;
		if (buf.Length())
			memset( (char*)buf.String(), '*', buf.Length());
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmGuiRoster::AskUserForPopAcc( const BmString& accName, BmString& popAccName)
{
	// ask user about password:
   BmString text = BmString( "Please select the POP3-account\nto be used "
   									  "in authentication\nfor SMTP-account <" )
	   				   << accName << ">:";
	BList list;
	BmAutolockCheckGlobal lock( ThePopAccountList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ThePopAccountList->ModelNameNC() << ": Unable to get lock"
		);
	BmModelItemMap::const_iterator iter;
	for(	iter = ThePopAccountList->begin(); 
			iter != ThePopAccountList->end(); ++iter) {
		BmPopAccount* acc = dynamic_cast<BmPopAccount*>( iter->second.Get());
		list.AddItem( (void*)acc->Name().String());
	}
	ListSelectionAlert* alert = new ListSelectionAlert( 
		"Pop-Account", text.String(), list, "", "Cancel", "OK"
	);
	alert->SetFeel( B_FLOATING_APP_WINDOW_FEEL);
	alert->SetLook( B_FLOATING_WINDOW_LOOK);
	alert->SetShortcut( 0, B_ESCAPE);
	char buf[128];
	int32 result = alert->Go( buf, 128);
	if (result == 1) {
		popAccName = buf;
		return popAccName.Length() > 0;
	} else
		return false;
}

namespace BmPrivate {

/*------------------------------------------------------------------------------*\
	ClearMenu()
		-	
\*------------------------------------------------------------------------------*/
static void ClearMenu( BmMenuControllerBase* menu)
{
	BMenuItem* old;
	while( (old = menu->RemoveItem( (int32)0)) != NULL)
		delete old;
	if (menu->Flags() & BM_MC_ADD_NONE_ITEM) {
		BMessage* msg = new BMessage( *(menu->MsgTemplate()));
		AddItemToMenu( menu, CreateMenuItem( BM_NoItemLabel.String(), msg), 
							menu->MsgTarget());
	}
}

/*------------------------------------------------------------------------------*\
	ListMenuBuilder()
		-	helper class that builds a menu from a given listmodel.
\*------------------------------------------------------------------------------*/
class ListMenuBuilder {

	typedef map< BmString, BmListModelItem* > BmSortedItemMap;
public:

	struct ItemFilter {
		virtual bool operator() (const BmListModelItem* item) = 0;
	};
	
	ListMenuBuilder( BmListModel* list, BMenu* menu, BMessage* msgTemplate,
						  BHandler* msgTarget, const BmString& shortcuts);
	~ListMenuBuilder();
	status_t Go();
	
	void SkipFirstLevel( bool b)			{ mSkipFirstLevel = b; }
	void AddNoneItem( bool b)				{ mAddNoneItem = b; }
	void ItemFilter( ItemFilter* f)		{ mItemFilter = f; }

private:
	void AddListItemToMenu( BmListModelItem* item, 
									BMenu* menu,
									bool skipThisButAddChildren=false,
									char shortcut=0);

	BmListModel* mList;
	BMenu* mMenu;
	BMessage* mMsgTemplate;
	BHandler* mMsgTarget;
	bool mSkipFirstLevel;
	bool mAddNoneItem;
	BFont mFont;
	struct ItemFilter* mItemFilter;
	BmString mShortcuts;
};

ListMenuBuilder::ListMenuBuilder( BmListModel* list, BMenu* menu, 
											 BMessage* msgTemplate, BHandler* msgTarget,
											 const BmString& shortcuts)
	:	mList(list)
	,	mMenu(menu)
	,	mMsgTemplate(msgTemplate)
	,	mMsgTarget(msgTarget)
	,	mSkipFirstLevel(false)
	,	mAddNoneItem(false)
	,	mItemFilter(NULL)
	,	mShortcuts(shortcuts)
{
	menu->GetFont( &mFont);
}

ListMenuBuilder::~ListMenuBuilder()
{
}

status_t ListMenuBuilder::Go()
{
	BmSortedItemMap sortedMap;
	if (mList) {
		BmAutolockCheckGlobal lock( mList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				mList->ModelNameNC() << ": Unable to get lock"
			);
		BmModelItemMap::const_iterator iter;
		for( iter = mList->begin();  iter != mList->end();  ++iter) {
			if (mItemFilter && !(*mItemFilter)(iter->second.Get()))
				continue;
			BmString sortKey = iter->second->DisplayKey();
			sortedMap[sortKey.ToLower()] = iter->second.Get();
		}
		if (mAddNoneItem && mMenu) {
			BMenuItem* noneItem = new BMenuItem( BM_NoItemLabel.String(),
															 new BMessage( *mMsgTemplate));
			noneItem->SetTarget( mMsgTarget);
			mMenu->AddItem( noneItem);
		}
		int s=0;
		BmSortedItemMap::const_iterator siter;
		for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter, ++s) {
			if (s<mShortcuts.Length())
				AddListItemToMenu( siter->second, mMenu, mSkipFirstLevel, 
										 mShortcuts[s]);
			else
				AddListItemToMenu( siter->second, mMenu, mSkipFirstLevel);
		}
	}
	return B_OK;
}

void ListMenuBuilder::AddListItemToMenu( BmListModelItem* item, 
													  BMenu* menu,
													  bool skipThisButAddChildren,
													  char shortcut) 
{
	if (menu) {
		BmSortedItemMap sortedMap;
		if (skipThisButAddChildren) {
			if (!item->empty()) {
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					if (mItemFilter && !(*mItemFilter)(iter->second.Get()))
						continue;
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddListItemToMenu( siter->second, menu);
			}
		} else {
			BMessage* msg = new BMessage( *mMsgTemplate);
			msg->AddString( BmListModel::MSG_ITEMKEY, item->Key().String());
			BMenuItem* menuItem;
			if (!item->empty()) {
				BMenu* subMenu = new BMenu( item->DisplayKey().String());
				subMenu->SetFont( &mFont);
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					if (mItemFilter && !(*mItemFilter)(iter->second.Get()))
						continue;
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddListItemToMenu( siter->second, subMenu);
				menuItem = new BMenuItem( subMenu, msg);
			} else {
				menuItem = new BMenuItem( item->DisplayKey().String(), msg);
			}
			if (shortcut)
				menuItem->SetShortcut( shortcut, 0);
			menuItem->SetTarget( mMsgTarget);
			menu->AddItem( menuItem);
		}
	}
}

static void RebuildList( BmMenuControllerBase* menu, BmListModel* list, 
								 bool skipFirstLevel=false);
/*------------------------------------------------------------------------------*\
	RebuildList()
		-	
\*------------------------------------------------------------------------------*/
static void RebuildList( BmMenuControllerBase* menu, BmListModel* list,
								 bool skipFirstLevel)
{
	ClearMenu( menu);	
	ListMenuBuilder builder(list, menu, menu->MsgTemplate(), menu->MsgTarget(),
									menu->Shortcuts());
	builder.SkipFirstLevel(skipFirstLevel);
	builder.Go();
}

};		// namespace BmPrivate

using namespace BmPrivate;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildFilterMenu( BmMenuControllerBase* menu)
{
	struct ItemFilter : public ListMenuBuilder::ItemFilter {
		bool operator() ( const BmListModelItem* item)
		{
			const BmFilter* filter = dynamic_cast<const BmFilter*>(item);
			return filter && filter->Kind().ICompare("Spam") != 0;
		}
	};
	ItemFilter itemFilter;
	ClearMenu( menu);	
	ListMenuBuilder builder(TheFilterList.Get(), menu, menu->MsgTemplate(), 
									menu->MsgTarget(), menu->Shortcuts());
//	builder.ItemFilter(&itemFilter);
	builder.Go();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildFilterChainMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheFilterChainList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildFolderMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheMailFolderList.Get(), true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildIdentityMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheIdentityList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildPeopleMenu( BmMenuControllerBase* menu)
{
	ClearMenu( menu);	
	// Delegate task (back) to MailEditWin:
	BmMailEditWin::RebuildPeopleMenu( menu);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildPopAccountMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, ThePopAccountList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildSignatureMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheSignatureList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildSmtpAccountMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheSmtpAccountList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildStatusMenu( BmMenuControllerBase* menu)
{
	ClearMenu( menu);	
	const char* stats[] = {
		BM_MAIL_STATUS_DRAFT, BM_MAIL_STATUS_FORWARDED, 
		BM_MAIL_STATUS_NEW, 
		BM_MAIL_STATUS_PENDING, BM_MAIL_STATUS_READ, 
		BM_MAIL_STATUS_REDIRECTED,
		BM_MAIL_STATUS_REPLIED, BM_MAIL_STATUS_SENT,	NULL
	};
	for( int i=0; stats[i]; ++i) {
		BMessage* msg = new BMessage(*(menu->MsgTemplate()));
		msg->AddString(BeamApplication::MSG_STATUS, stats[i]);
		BMenuItem* item 
			= new BMenuItem( stats[i], msg);
		item->SetTarget( menu->MsgTarget());
		menu->AddItem( item);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildCharsetMenu( BmMenuControllerBase* menu)
{
	ClearMenu( menu);	
	AddCharsetMenu(menu, menu->MsgTarget(), menu->MsgTemplate()->what);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType)
{
	menu->SetLabelFromMarked( false);
	// add standard entries:
	BmString charset;
	vector<BmString> charsets;
	BmString sets = ThePrefs->GetString( "StandardCharsets");
	split( BmPrefs::nListSeparator, sets, charsets);
	int32 numStdSets = charsets.size();
	for( int i=0; i<numStdSets; ++i) {
		charset = charsets[i];
		charset.ToLower();
		BMessage* msg = new BMessage( msgType);
		AddItemToMenu( menu, CreateMenuItem( charset.String(), msg), target);
	}
	// add all other charsets:
	BMenu* moreMenu = new BMenu( "<show all>");
	moreMenu->SetLabelFromMarked( false);
	BFont font( *be_plain_font);
	font.SetSize( 10);
	moreMenu->SetFont( &font);
	BmCharsetMap::const_iterator iter;
	for( iter = TheCharsetMap.begin(); iter != TheCharsetMap.end(); ++iter) {
		if (iter->second) {
			charset = iter->first;
			charset.ToLower();
			BMessage* msg = new BMessage( msgType);
			AddItemToMenu( moreMenu, 
								CreateMenuItem( charset.String(), msg), 
								target);
		}
	}
	menu->AddSeparatorItem();
	menu->AddItem( moreMenu);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildLogMenu( BmMenuControllerBase* logMenu) {
	const char* logNames[] = {
		"Beam",
		"Filter",
		"MailParser",
		NULL
	};
	BMenuItem* old;
	while( (old = logMenu->RemoveItem( (int32)0))!=NULL)
		delete old;
	for( int i=0; logNames[i] != NULL; ++i) {
		BMessage* logMsg( new BMessage( logMenu->MsgTemplate()->what));
		logMsg->AddString( "logfile", logNames[i]);
		logMenu->AddItem( new BMenuItem( logNames[i], logMsg));
	}
	// POP
	{
		BMenu* popMenu = new BMenu( "POP-Accounts");
		BmAutolockCheckGlobal lock( ThePopAccountList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ThePopAccountList->ModelNameNC() << ": Unable to get lock"
			);
		BmModelItemMap::const_iterator iter;
		for( 	iter = ThePopAccountList->begin(); 
				iter != ThePopAccountList->end(); ++iter) {
			BmString logname = BmString("POP_") + iter->second->Key();
			BMessage* logMsg( new BMessage( logMenu->MsgTemplate()->what));
			logMsg->AddString( "logfile", logname.String());
			popMenu->AddItem( new BMenuItem( iter->second->Key().String(), logMsg));
		}
		logMenu->AddItem( popMenu);
	}
	// SMTP
	{
		BMenu* smtpMenu = new BMenu( "SMTP-Accounts");
		BmAutolockCheckGlobal lock( TheSmtpAccountList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				TheSmtpAccountList->ModelNameNC() << ": Unable to get lock"
			);
		BmModelItemMap::const_iterator iter;
		for( 	iter = TheSmtpAccountList->begin(); 
				iter != TheSmtpAccountList->end(); ++iter) {
			BmString logname = BmString("SMTP_") + iter->second->Key();
			BMessage* logMsg( new BMessage( logMenu->MsgTemplate()->what));
			logMsg->AddString( "logfile", logname.String());
			smtpMenu->AddItem( new BMenuItem( iter->second->Key().String(), logMsg));
		}
		logMenu->AddItem( smtpMenu);
	}
	TheLogHandler->mLocker.Unlock();
}

