/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
/*
 * MailConverter reads mail-files that are plain text and writes 
 * them as BeOS mail-files (which are plain-text, too, but have a lot of
 * attributes).
 * Usage:
 *			MailConverter <folder_with_mails_to_convert>
 */

#include <Directory.h>
#include <File.h>

#include "BmApp.h"
#include "BmMail.h"
#include "BmMailRef.h"
#include "BmPrefs.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void MailConverter( const char* folder)
{
	BDirectory dir( folder);
	if (dir.InitCheck() != B_OK) {
		fprintf(stderr, "can't open folder %s\n", folder);
		exit(5);
	}
	entry_ref eref;
	BFile file;
	off_t size;
	BmString str;
	char* buf;
	off_t sz;
	BmMail mail( BM_DEFAULT_STRING, "");
	status_t res;
	BmString status( BM_MAIL_STATUS_READ);
	while(dir.GetNextRef(&eref) == B_OK) {
		if ((res = file.SetTo(&eref, B_READ_WRITE)) != B_OK) {
			fprintf(stderr, "%s: %s\n", eref.name, strerror(res));
			continue;
		}
		printf("%s...", eref.name);
		file.GetSize(&size);
		buf = str.LockBuffer(size+1);
		if (!buf) {
			fprintf(stderr, "not enough memory for %Lu bytes\n", size);
		}
		sz = file.Read(buf, size);
		str.UnlockBuffer(sz);
		mail.SetTo(str, "dummyAccount");
		mail.StoreIntoFile(&dir, eref.name, status, real_time_clock_usecs());
		printf("ok\n");
	}
}

int 
main( int argc, char** argv) 
{
	const char* APP_SIG = "application/x-vnd.zooey-mailconverter";
	if (argc != 2) {
		fprintf(stderr, "This program converts all files in a given path\n"
							 "to BeOS-mail-files (with attributes and all that).\n"
							 "usage:\n\t%s <path-to-mail-files>\n", argv[0]);
	} else {
		BmApplication* app = new BmApplication( APP_SIG, true);
		MailConverter( argv[1]);
		delete app;
	}
}
