/*
	MailConverter.cpp
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
	BEntry entry;
	BPath path;
	while(dir.GetNextRef(&eref) == B_OK) {
		if ((res = file.SetTo(&eref, B_READ_WRITE)) != B_OK) {
			fprintf(stderr, "%s: %s\n", eref.name, strerror(res));
			continue;
		}
		printf("%s...", eref.name);
		entry.SetTo(&eref);
		entry.GetPath(&path);
		file.GetSize(&size);
		buf = str.LockBuffer(size+1);
		if (!buf) {
			fprintf(stderr, "not enough memory for %Lu bytes\n", size);
		}
		sz = file.Read(buf, size);
		str.UnlockBuffer(sz);
		mail.SetTo(str, "dummyAccount");
		mail.StoreIntoFile(path.Path(), status, real_time_clock_usecs());
		printf("ok\n");
	}
}

int 
main( int argc, char** argv) 
{
	const char* APP_SIG = "application/x-vnd.zooey-mailconverter";
	BmApplication* app = new BmApplication( APP_SIG, true);
	MailConverter( argv[1]);
	delete app;
}
