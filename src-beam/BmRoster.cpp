/*
	BmRoster.cpp
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

#include "regexx.hh"
using namespace regexx;

#include "TextEntryAlert.h"
#include "ListSelectionAlert.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMenuControllerBase.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmRoster.h"
#include "BmSmtpAccount.h"
#include "BmSignature.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

static void RebuildList( BmMenuControllerBase* menu, BmListModel* list, 
								 bool skipFirstLevel=false);

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRoster::BmRoster()
{
	BPath path;
	// determine the path to the user-settings-directory:
	if (find_directory( B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		BM_THROW_RUNTIME( "Sorry, could not determine user's settings-dir !?!");

	mSettingsPath.SetTo( path.Path());
	if (BeamInTestMode)
		// in order to avoid clobbering precious settings,
		// we use a different settings-folder  in testmode:
		mSettingsPath << "/Beam_Test";
	else if (BeamInDevelMode)
		// in order to avoid clobbering precious settings,
		// we use a different settings-folder  in devel-mode:
		mSettingsPath << "/Beam_Devel";
	else
		// standard settings folder:
		mSettingsPath << "/Beam";

	SetupFolder( mSettingsPath + "/MailCache/", &mMailCacheFolder);
	SetupFolder( mSettingsPath + "/StateInfo/", &mStateInfoFolder);

	// Determine our own FQDN from network settings file, if possible:
	FetchOwnFQDN();
}

/*------------------------------------------------------------------------------*\
	UpdateMimeTypeFile( sig, appModTime)
		-	checks age of our own mimetype-file 
			(.../settings/beos_mime/application/x-vnd.zooey-beam)
			and removes the file if it's older than the application file.
\*------------------------------------------------------------------------------*/
void BmRoster::UpdateMimeTypeFile( const char* s, time_t appModTime) {
	BmString sig(s);
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		sig.ToLower();
		BEntry mtEntry( (BmString(path.Path())<<"/beos_mime/"<<sig).String());
		if (mtEntry.InitCheck() == B_OK) {
			time_t modTime;
			if (mtEntry.GetModificationTime( &modTime) == B_OK) {
				if (appModTime > modTime) {
					// application is newer than mimetype-file, we simply remove
					// that and let BeOS recreate it when needed. The new version
					// will then contain all current icons, etc.
					mtEntry.Remove();
				}
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	FetchOwnFQDN()
		-	fetches hostname and domainname from network settings and build FQDN 
			from that.
\*------------------------------------------------------------------------------*/
void BmRoster::FetchOwnFQDN() {
	BmString buffer;
	Regexx rx;
#ifdef BEAM_FOR_BONE
	FetchFile( "/etc/hostname", mOwnFQDN);
	mOwnFQDN.RemoveSet( " \n\r\t");
	if (!mOwnFQDN.Length())
		mOwnFQDN = "bepc";
	FetchFile( "/etc/resolv.conf", buffer);
	if (rx.exec( buffer, "DOMAIN\\s*(\\S*)", Regexx::nocase)
	&& rx.match[0].atom[0].Length())
		mOwnFQDN << "." << rx.match[0].atom[0];
	else
		mOwnFQDN << "." << time( NULL) << ".fake";
#else
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		FetchFile( BmString(path.Path())<<"/network", buffer);
		if (rx.exec( buffer, "HOSTNAME\\s*=[ \\t]*(\\S*)", Regexx::nocase)) {
			mOwnFQDN = rx.match[0].atom[0];
			if (!mOwnFQDN.Length())
				mOwnFQDN = "bepc";
			if (rx.exec( buffer, "DNS_DOMAIN\\s*=[ \\t]*(\\S*)", Regexx::nocase)
			&& rx.match[0].atom[0].Length())
				mOwnFQDN << "." << rx.match[0].atom[0];
			else
				mOwnFQDN << "." << time( NULL) << ".fake";
		}
	}
#endif
	if (!mOwnFQDN.Length())
		mOwnFQDN << "bepc." << time( NULL) << ".fake";
	mOwnFQDN.RemoveSet( "\r\n");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmRoster::AskUserForPwd( const BmString& text, BmString& pwd) 
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
bool BmRoster::AskUserForPopAcc( const BmString& accName, BmString& popAccName)
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
	RebuildList()
		-	
\*------------------------------------------------------------------------------*/
static void RebuildList( BmMenuControllerBase* menu, BmListModel* list,
								 bool skipFirstLevel)
{
	ClearMenu( menu);	
	BFont font;
	menu->GetFont( &font);
	AddListToMenu( list, menu, menu->MsgTemplate(), menu->MsgTarget(), 
						&font, skipFirstLevel);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildFilterMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheFilterList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildFilterChainMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheFilterChainList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildFolderMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheMailFolderList.Get(), true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildIdentityMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheIdentityList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildPeopleMenu( BmMenuControllerBase* menu)
{
	ClearMenu( menu);	
	// Delegate task (back) to MailEditWin:
	BmMailEditWin::RebuildPeopleMenu( menu);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildPopAccountMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, ThePopAccountList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildSignatureMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheSignatureList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildSmtpAccountMenu( BmMenuControllerBase* menu)
{
	RebuildList( menu, TheSmtpAccountList.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildStatusMenu( BmMenuControllerBase* menu)
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
		BMenuItem* item 
			= new BMenuItem( stats[i], new BMessage(*(menu->MsgTemplate())));
		item->SetTarget( menu->MsgTarget());
		menu->AddItem( item);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::RebuildCharsetMenu( BmMenuControllerBase* menu)
{
	ClearMenu( menu);	
	AddCharsetMenu(menu, menu->MsgTarget(), menu->MsgTemplate()->what);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRoster::AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType)
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
void BmRoster::RebuildLogMenu( BmMenuControllerBase* logMenu) {
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

