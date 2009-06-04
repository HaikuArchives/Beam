/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <algorithm>

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
#include "BmGuiRoster.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMailRefFilterControl.h"
#include "BmMailRefViewFilterControl.h"
#include "BmMenuControllerBase.h"
#include "BmMsgTypes.h"
#include "BmPeople.h"
#include "BmRecvAccount.h"
#include "BmPrefs.h"
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
bool BmGuiRoster::AskUserForPopAcc( const BmString& accName, 
												BmString& popAccName)
{
	// ask user about password:
   BmString text = BmString( "Please select the POP3-account\nto be used "
   									  "in authentication\nfor SMTP-account <" )
	   				   << accName << ">:";
	BList list;
	BmAutolockCheckGlobal lock( TheRecvAccountList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			TheRecvAccountList->ModelNameNC() << ": Unable to get lock"
		);
	BmModelItemMap::const_iterator iter;
	for(	iter = TheRecvAccountList->begin(); 
			iter != TheRecvAccountList->end(); ++iter) {
		BmRecvAccount* acc = dynamic_cast<BmRecvAccount*>( iter->second.Get());
		list.AddItem( (void*)acc->Name().String());
	}
	ListSelectionAlert* alert = new ListSelectionAlert( 
		"Recv-Account", text.String(), list, "", "Cancel", "OK"
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

/*------------------------------------------------------------------------------*\
	IsEmailKnown()
		-	
\*------------------------------------------------------------------------------*/
bool BmGuiRoster::IsEmailKnown( const BmString& email)
{
	return ThePeopleList->FindPersonByEmail(email) != (BmPerson*)NULL
			 || ThePeopleList->IsAddressKnown(email);
}

namespace BmPrivate {

/*------------------------------------------------------------------------------*\
	ClearMenu()
		-	
\*------------------------------------------------------------------------------*/
static void ClearMenu( BmMenuControllerBase* menu)
{
	menu->Clear();
}

/*------------------------------------------------------------------------------*\
	ListMenuBuilder()
		-	helper class that builds a menu from a given listmodel.
\*------------------------------------------------------------------------------*/
class ListMenuBuilder {

	typedef vector< BmListModelItem*> ItemVect;
public:

	struct ItemFilter {
		// returns true if item should be filtered:
		virtual bool operator() (const BmListModelItem* item) = 0;
	};

	struct ItemComparer {
		bool operator() (const BmListModelItem* a, const BmListModelItem* b) {
			return a->DisplayKey().ICompare(b->DisplayKey()) < 0;
		}
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
	if (mList) {
		BmAutolockCheckGlobal lock( mList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				mList->ModelNameNC() << ": Unable to get lock"
			);
		ItemVect sortedVect;
		BmModelItemMap::const_iterator iter;
		for( iter = mList->begin();  iter != mList->end();  ++iter) {
			if (!mItemFilter || !(*mItemFilter)(iter->second.Get()))
				sortedVect.push_back(iter->second.Get());
		}
		sort(sortedVect.begin(), sortedVect.end(), ItemComparer());
		if (mAddNoneItem && mMenu) {
			BMenuItem* noneItem = new BMenuItem( BM_NoItemLabel.String(),
															 new BMessage( *mMsgTemplate));
			noneItem->SetTarget( mMsgTarget);
			mMenu->AddItem( noneItem);
		}
		for( uint32 i=0; i<sortedVect.size(); ++i) {
			AddListItemToMenu( sortedVect[i], mMenu, mSkipFirstLevel, 
									 i < (uint32)mShortcuts.Length() 
									 	? mShortcuts[i] 
									 	: '\0');
		}
	}
	return B_OK;
}

void ListMenuBuilder::AddListItemToMenu( BmListModelItem* item, 
													  BMenu* menu,
													  bool skipThisButAddChildren,
													  char shortcut) 
{
	if (menu && item) {
		ItemVect sortedVect;
		BmModelItemMap::const_iterator iter;
		for( iter = item->begin();  iter != item->end();  ++iter) {
			if (!mItemFilter || !(*mItemFilter)(iter->second.Get()))
				sortedVect.push_back(iter->second.Get());
		}
		sort(sortedVect.begin(), sortedVect.end(), ItemComparer());
		if (skipThisButAddChildren) {
			for( uint32 i=0; i<sortedVect.size(); ++i)
				AddListItemToMenu( sortedVect[i], menu);
		} else {
			BMessage* msg = new BMessage( *mMsgTemplate);
			msg->AddString( BmListModel::MSG_ITEMKEY, item->Key().String());
			BMenuItem* menuItem;
			if (!item->empty()) {
				BMenu* subMenu = new BMenu( item->DisplayKey().String());
				subMenu->SetFont( &mFont);
				for( uint32 i=0; i<sortedVect.size(); ++i) {
					if (!mItemFilter || !(*mItemFilter)(iter->second.Get()))
						AddListItemToMenu( sortedVect[i], subMenu);
				}
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
/*
	struct ItemFilter : public ListMenuBuilder::ItemFilter {
		bool operator() ( const BmListModelItem* item)
		{
			const BmFilter* filter = dynamic_cast<const BmFilter*>(item);
			return !filter || filter->Kind().ICompare("Spam") == 0;
		}
	};
	ItemFilter itemFilter;
	ClearMenu( menu);	
	ListMenuBuilder builder(TheFilterList.Get(), menu, menu->MsgTemplate(), 
									menu->MsgTarget(), menu->Shortcuts());
	builder.ItemFilter(&itemFilter);
	builder.Go();
*/
	RebuildList( menu, TheFilterList.Get());
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
void BmGuiRoster::RebuildRecvAccountMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheRecvAccountList.Get());
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
		msg->AddString("charset", charset.String());
		AddItemToMenu( menu, CreateMenuItem( charset.String(), msg), target);
	}
	// add all other charsets:
	BMenu* moreMenu = new BMenu( "<show all>");
	moreMenu->SetLabelFromMarked( false);
	BFont font( *be_plain_font);
	moreMenu->SetFont( &font);
	BmCharsetMap::const_iterator iter;
	for( iter = TheCharsetMap.begin(); iter != TheCharsetMap.end(); ++iter) {
		if (iter->second) {
			charset = iter->first;
			charset.ToLower();
			BMessage* msg = new BMessage( msgType);
			msg->AddString("charset", charset.String());
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
		BMenu* recvMenu = new BMenu( "Receiving Accounts");
		BmAutolockCheckGlobal lock( TheRecvAccountList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				TheRecvAccountList->ModelNameNC() << ": Unable to get lock"
			);
		BmModelItemMap::const_iterator iter;
		for( 	iter = TheRecvAccountList->begin(); 
				iter != TheRecvAccountList->end(); ++iter) {
			BmRecvAccount* recvAcc 
				= dynamic_cast<BmRecvAccount*>(iter->second.Get());
			if (!recvAcc)
				continue;
			BmString logname 
				= BmString(recvAcc->Type()) << "_" << iter->second->Key();
			BMessage* logMsg( new BMessage( logMenu->MsgTemplate()->what));
			logMsg->AddString( "logfile", logname.String());
			recvMenu->AddItem( new BMenuItem( iter->second->Key().String(), 
														 logMsg));
		}
		logMenu->AddItem( recvMenu);
	}
	// SMTP
	{
		BMenu* smtpMenu = new BMenu( "Sending Accounts");
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildMailRefFilterMenu( BmMenuControllerBase* menu)
{
	ClearMenu(menu);
	BmString options = ThePrefs->GetString("TimeSpanOptions");
	vector<BmString> choices;
	split(",", options, choices);
	choices.insert(choices.begin(), BmMailRefFilterControl::TIME_SPAN_NONE);
	for (uint32 i = 0; i < choices.size(); ++i) {
		BmString label = choices[i];
		if (i > 0)
			label << " Days";
		BMessage* msg = new BMessage(*(menu->MsgTemplate()));
		msg->AddString(BmMailRefFilterControl::MSG_TIME_SPAN_LABEL,
			label.String());
		BMenuItem* item = new BMenuItem(label.String(), msg);
		item->SetTarget(menu->MsgTarget());
		menu->AddItem( item);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGuiRoster::RebuildMailRefViewFilterMenu( BmMenuControllerBase* menu)
{
	ClearMenu(menu);
	const char* choices[] = {
		BmMailRefItemFilter::FILTER_SUBJECT_OR_ADDRESS,
		BmMailRefItemFilter::FILTER_MAILTEXT,
		NULL
	};
	for( int i=0; choices[i]; ++i) {
		BMessage* msg = new BMessage(*(menu->MsgTemplate()));
		msg->AddString(BmMailRefViewFilterControl::MSG_FILTER_KIND, 
			choices[i]);
		BMenuItem* item = new BMenuItem(choices[i], msg);
// TODO: remove, once it is implemented
if (i==1)
	item->SetEnabled(false);
		item->SetTarget( menu->MsgTarget());
		menu->AddItem( item);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 BmGuiRoster::ShowAlert( const BmString& text, const char* btn1,
										const char* btn2, const char* btn3)
{
	BAlert* alert = new BAlert( "Info needed", text.String(),
									 	 btn1, btn2, btn3);
	alert->SetShortcut( 0, B_ESCAPE);
	return alert->Go();
}
