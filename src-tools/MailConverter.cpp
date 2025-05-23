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
#include <NodeInfo.h>

#include "BmApp.h"
#include "BmMail.h"
#include "BmMailRef.h"

// NOTE: The pragma(s) below do not change any functionality in the code. They only hide those 
// warnings when building. At some point, the lines that produce the(se) warning(s), should be
// reviewed to confirm, whether there is something that needs to be "fixed".
#pragma GCC diagnostic ignored "-Wformat="

class MyMail : public BmMail {
public:
	MyMail()
		:	BmMail(BM_DEFAULT_STRING, "") 
	{
	}
	void SetTo(const BmString &msgText) 
	{
		BmMail::SetTo(msgText, "dummy-account");
	}
	void StoreAttributes( BFile& mailFile, const BmString& status, 
								 bigtime_t whenCreated) 
	{
		BmMail::StoreAttributes(mailFile, status, whenCreated);
		Header()->StoreAttributes(mailFile);
	}
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void MailConverter( const char* folder)
{
	BDirectory dir( folder);
	if (dir.InitCheck() != B_OK) {
		fprintf(stderr, "can't open folder %s\n", folder);
		return;
	}
	entry_ref eref;
	BFile file;
	off_t size;
	BmString str;
	char* buf;
	off_t sz;
	MyMail mail;
	status_t res;
	time_t modTime;
	bigtime_t modTimeBig;
	BmString status( BM_MAIL_STATUS_READ);
	int errorCount = 0;
	int okCount = 0;
	while(dir.GetNextRef(&eref) == B_OK) {
		printf("%s...", eref.name);
		if ((res = file.SetTo(&eref, B_READ_WRITE)) != B_OK) {
			fprintf(stderr, "unable to access file - %s\n", strerror(res));
			errorCount++;
			continue;
		}
		file.GetSize(&size);
		buf = str.LockBuffer(int32(size+1));
		if (!buf) {
			fprintf(stderr, "not enough memory for %Lu bytes\n", size);
			errorCount++;
			continue;
		}
		sz = file.Read(buf, size_t(size));
		if (sz < 0) {
			fprintf(stderr, "unable to read from file - %s\n", strerror(res));
			errorCount++;
			continue;
		}
		str.UnlockBuffer(int32(sz));
		if (sz != size) {
			fprintf(
				stderr, "unable to read %Ld bytes from file (only got %Ld)\n", 
				size, sz
			);
			errorCount++;
			continue;
		}
		mail.SetTo(str);
		file.GetModificationTime(&modTime);
		modTimeBig = ((bigtime_t)modTime) * 1000*1000;
		mail.StoreAttributes(file, status, modTimeBig);
		BNodeInfo nodeInfo(&file);
		nodeInfo.SetType("text/x-email");
		okCount++;
		printf("ok\n");
	}
	printf("%d mails have been converted successfully!\n", okCount);
	if (errorCount)
		printf("%d mails could not be converted!\n", errorCount);
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
