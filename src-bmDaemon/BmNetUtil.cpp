/*
	BmNetUtil.cpp
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


#include <socket.h>
#include <netdb.h>
#include <unistd.h>
#ifdef BONE_VERSION
#define GETHOSTNAME_OK 0
#else
#define GETHOSTNAME_OK 1
#endif

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmNetUtil.h"
#include "BmResources.h"

/*------------------------------------------------------------------------------*\
	OwnFQDN()
		-	tries to find out our own FQDN (full qualified domainname)
\*------------------------------------------------------------------------------*/
BString OwnFQDN() {
	char hostname[MAXHOSTNAMELEN+1];
	hostname[MAXHOSTNAMELEN] = '\0';
	int result;
	if ((result=gethostname( hostname, MAXHOSTNAMELEN)) < GETHOSTNAME_OK) {
		BM_SHOWERR("Sorry, could not determine name of this host, giving up.");
		return "";
	}
	hostent *hptr;
	// in case we can't get a FQDN by means of gethostbyname, we try the network-settings
	// file, if that fails as well, we return the hostname:
	if ((hptr = gethostbyname( hostname)) == NULL) {
		if (TheResources->mOwnFQDN.Length())
			return TheResources->mOwnFQDN;		// FQDN that was built from network settings file
		else
			return hostname;
	} else {
		hostent *h2ptr;
		if ((h2ptr = gethostbyaddr( hptr->h_addr_list[0], 4, AF_INET)) == NULL)
			return hptr->h_name;
		else
			return h2ptr->h_name;
	}
}

/*------------------------------------------------------------------------------*\
	OwnDomain()
		-	tries to find out our own domain
\*------------------------------------------------------------------------------*/
BString OwnDomain( BString fqdn) {
	if (!fqdn.Length())
		fqdn = OwnFQDN();
	int32 firstDot = fqdn.FindFirst(".");
	if (firstDot != B_ERROR) {
		if (fqdn.FindFirst(".",firstDot+1) != B_ERROR)
			return fqdn.Remove( 0, firstDot+1);
		else
			return fqdn;
	} else
		return "";
}
