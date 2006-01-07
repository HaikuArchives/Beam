/*
	BmNetEndpoint.cpp

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

#include <cstring>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "BmLogHandler.h"
#include "BmNetEndpoint.h"
#include "BmNetEndpointRoster.h"
#include "BmRosterBase.h"

//*****************************************************************************
// #pragma mark - BmNetEndpointRoster
//*****************************************************************************

static BmNetEndpointRoster gNetEndpointRoster;
BmNetEndpointRoster* TheNetEndpointRoster = &gNetEndpointRoster;

/*------------------------------------------------------------------------------*\
	BmNetEndpointRoster()
		-	
\*------------------------------------------------------------------------------*/
BmNetEndpointRoster::BmNetEndpointRoster()
	:	mNeedInit(true)
	,	mAddonImage(-1)
	,	mAddonName()
	,	mAddonInstantiateFunc(NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmNetEndpointRoster()
		-	
\*------------------------------------------------------------------------------*/
BmNetEndpointRoster::~BmNetEndpointRoster()
{
	_Cleanup();
}

/*------------------------------------------------------------------------------*\
	CreateEndpoint()
		-	
\*------------------------------------------------------------------------------*/
BmNetEndpoint* BmNetEndpointRoster::CreateEndpoint()
{
	if (mNeedInit)
		_Initialize();
	BmNetEndpoint* endpoint = NULL;
	if (mAddonInstantiateFunc)
		// create endpoint via addon:
		endpoint = mAddonInstantiateFunc();
	if (!endpoint)
		// create default endpoint:
		endpoint = new BmNetEndpoint;
	return endpoint;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetEndpointRoster::SupportsEncryption()
{
	if (mNeedInit)
		_Initialize();
	bool supportsEncryption = false;
	mEncryptionInfo.FindBool("supports-encryption", &supportsEncryption);
	return supportsEncryption;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetEndpointRoster::SupportsEncryptionType(const char* _encType)
{
	if (mNeedInit)
		_Initialize();
	BmString encType(_encType);
	const char* type;
	for(int32 i=0; mEncryptionInfo.FindString("type", i, &type)==B_OK; ++i) {
		if (encType == type)
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmNetEndpointRoster::GetEncryptionInfo(BMessage* encryptionInfo)
{
	if (mNeedInit)
		_Initialize();
	if (!encryptionInfo)
		return B_BAD_VALUE;
		*encryptionInfo = mEncryptionInfo;
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	
\*------------------------------------------------------------------------------*/
void BmNetEndpointRoster::_Initialize()
{
	BDirectory addonDir;
	BPath path;
	BEntry entry;
	status_t err;

	BM_LOG2( BM_LogApp, "Started loading of BmNetEndpoint-addons...");

	BmString addonPath = BmString(BeamRoster->AppPath()) + "/add-ons/NetEndpoints";
	addonDir.SetTo( addonPath.String());
	// scan through all its entries for add-ons:
	while ( addonDir.GetNextEntry( &entry, true) == B_OK) {
		if (entry.IsFile()) {
			char nameBuf[B_FILE_NAME_LENGTH];
			entry.GetName( nameBuf);
			// try to load addon:
			BM_LOG2( BM_LogApp, BmString("Trying to load netendpoint-addon ") 
										<< nameBuf << "...");
			entry.GetPath( &path);
			if ((mAddonImage = load_add_on( path.Path())) < 0) {
				BM_LOG( BM_LogApp, 
						  BmString("Unable to load netendpoint-addon\n\t")
							<< nameBuf << "\n\nError:\n\t" << strerror( mAddonImage));
				continue;
			}
			if ((err = get_image_symbol( 
				mAddonImage, "InstantiateNetEndpoint", B_SYMBOL_TYPE_ANY, 
				(void**)&mAddonInstantiateFunc
			)) != B_OK) {
				BM_LOGERR( BmString("Unable to load netendpoint-addon\n\t")
								<< nameBuf
								<< "\n\nMissing symbol 'InstantiateNetEndpoint'");
				continue;
			}
			if ((err = get_image_symbol( 
				mAddonImage, "GetEncryptionInfo", B_SYMBOL_TYPE_ANY, 
				(void**)&mAddonGetEncryptionInfoFunc
			)) != B_OK) {
				BM_LOGERR( BmString("Unable to load netendpoint-addon\n\t")
								<< nameBuf
								<< "\n\nMissing symbol 'GetEncryptionInfo'");
				continue;
			}
			mAddonName = nameBuf;
			BM_LOG( BM_LogApp, BmString("Successfully loaded addon ") << nameBuf);
		}
	}
	BM_LOG2( BM_LogApp, "Done with loading BmNetEndpoint-addons.");
	mNeedInit = false;
	// fill message with info from addon:
	if (mAddonGetEncryptionInfoFunc)
		mAddonGetEncryptionInfoFunc(&mEncryptionInfo);
}

/*------------------------------------------------------------------------------*\
	Cleanup()
		-	
\*------------------------------------------------------------------------------*/
void BmNetEndpointRoster::_Cleanup()
{
	if (mAddonImage >= 0) {
		unload_add_on(mAddonImage);
		mAddonImage = -1;
		mAddonName = "";
		mAddonInstantiateFunc = NULL;
	}
	mEncryptionInfo.MakeEmpty();
}
