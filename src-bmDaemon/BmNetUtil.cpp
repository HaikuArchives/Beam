/*
	BmNetUtil.cpp
		$Id$
*/

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

/*------------------------------------------------------------------------------*\
	OwnFQHN()
		-	tries to find out our own FQHN (full qualified hostname)
\*------------------------------------------------------------------------------*/
BString OwnFQHN() {
	char hostname[MAXHOSTNAMELEN+1];
	hostname[MAXHOSTNAMELEN] = '\0';
	int result;
	if ((result=gethostname( hostname, MAXHOSTNAMELEN)) < GETHOSTNAME_OK) {
		BM_SHOWERR("Sorry, could not determine name of this host, giving up.");
		return "";
	}
	hostent *hptr;
	if ((hptr = gethostbyname( hostname)) == NULL) {
		BM_SHOWERR("Sorry, could not determine IP-address of this host, giving up.");
		return "";
	}
	if ((hptr = gethostbyaddr( hptr->h_addr_list[0], 4, AF_INET)) == NULL) {
		BM_SHOWERR("Sorry, could not determine FQHN of this host, giving up.");
		return "";
	}
	return hptr->h_name;
}

/*------------------------------------------------------------------------------*\
	OwnDomain()
		-	tries to find out our own domain
\*------------------------------------------------------------------------------*/
BString OwnDomain() {
	BString fqhn = OwnFQHN();
	int32 firstDot = fqhn.FindFirst(".");
	if (firstDot != B_ERROR)
		return fqhn.Remove( 0, firstDot+1);
	else
		return "";
}
