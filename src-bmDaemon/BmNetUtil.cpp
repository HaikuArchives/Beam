/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <socket.h>
#include <netdb.h>
#include <unistd.h>
#ifdef BEAM_FOR_BONE
# include <bone_serial_ppp.h>
# define GETHOSTNAME_OK 0
#else
# define GETHOSTNAME_OK 1
#endif

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmNetUtil.h"
#include "BmRosterBase.h"

/*------------------------------------------------------------------------------*\
	OwnDomain()
		-	tries to find out our own domain
\*------------------------------------------------------------------------------*/
BmString OwnDomain( BmString fqdn) {
	if (!fqdn.Length())
		fqdn = BeamRoster->OwnFQDN();
	int32 firstDot = fqdn.FindFirst(".");
	if (firstDot != B_ERROR) {
		if (fqdn.FindFirst(".",firstDot+1) != B_ERROR)
			return fqdn.Remove( 0, firstDot+1);
		else
			return fqdn;
	} else
		return "";
}

/*------------------------------------------------------------------------------*\
	IsPPPRunning()
		-	checks whether or not PPP-daemon is running
\*------------------------------------------------------------------------------*/
bool IsPPPRunning() {
	bool running = false;
#ifdef BEAM_FOR_BONE
	// the following has ruthlessly been ripped from the 
	// MailDaemonReplacement (MDR):
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	bsppp_status_t status;
	strcpy(status.if_name, "ppp0");
	if (ioctl(s, BONE_SERIAL_PPP_GET_STATUS, &status, sizeof(status)) == 0) {
		if (status.connection_status == BSPPP_CONNECTED)
			running = true;
	}
	close (s);
#else
	running = (find_thread("tty_thread") > 0);
#endif
	return running;
}

