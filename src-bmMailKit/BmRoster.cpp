/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <FindDirectory.h>
#include <Path.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmRoster.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

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
