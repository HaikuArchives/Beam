/*
	SpamOMeter.cpp
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
 * SpamOMeter reads a file that contains paths of mail-files and then 
 * proceeds to read all these mails, run a SPAM-classification, check
 * the result of the classification (the path of the mails indicates whether
 * or not the mails are considered to be SPAM) and trains-on-error.
 * The error-rate of the last 500 messages is taken as the SPAM-performance
 * (i.e. the quality of the classifier).
 * Usage:
 *			SpamOMeter <file_with_mail_paths>
 */

#include <stdarg.h>

#include <Directory.h>
#include <File.h>

#include "split.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmFilter.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmMailRef.h"

static bool Verbose = false;
static bool Statistics = false;

static int32 ThresholdForSpam = 0;
static int32 ThresholdForTofu = 0;
static int32 UnsureForSpam = 0;
static int32 UnsureForTofu = 0;

static uint32 Limit = 0xFFFFFFFF;
static int32 PerfCount = -1;

static bool DeHtml = false;
static bool KeepATags = false;

static void Out(const char* format, ...)
{
	if (Verbose) {
		va_list args;
		va_start( args, format);
		vprintf( format, args);
	} else  {
		static uint8 idx = 1;
		static const char chars[] = " -\\|/";
		printf("%c%c", chars[idx++], 0x08);
		if (chars[idx] == '\0')
			idx = 1;
	}
	fflush(stdout);
}

struct ResInfo 
{
	ResInfo() 
		:	corrSpam(0)
		,	corrTofu(0)
		,	reinfSpam(0)
		,	reinfTofu(0)
		,	falsePos(0)
		,	falseNeg(0)
		,	unsure(0)
		,	totalFalsePos(0)
		,	totalOverall(0)
		{}
	
	double corrSpam, corrTofu, reinfSpam, reinfTofu, falsePos, falseNeg,
			 unsure, totalFalsePos, totalOverall;
};

static BmFilterAddon* spamAddon;
static BMessage ResetJob;
static BMessage ClassifyJob;
static BMessage LearnAsSpamJob;
static BMessage LearnAsTofuJob;
static BMessage GetStatisticsJob;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void SpamOMeter( const char* pathfileName, ResInfo& ri)
{
	status_t res;
	off_t size;
	off_t sz;
	BmString str;

	spamAddon->Execute( NULL, &ResetJob);

	time_t starttime = time(NULL);
	printf("%s: ", pathfileName);
	if (Verbose)
		printf("\n");

	BFile pathFile(pathfileName, B_READ_ONLY);
	if ((res = pathFile.InitCheck()) != B_OK) {
		fprintf(stderr, "%s: %s\n", pathfileName, strerror(res));
		return;
	}
	pathFile.GetSize(&size);
	char* buf = str.LockBuffer(size+1);
	if (!buf) {
		fprintf(stderr, "not enough memory for %Lu bytes\n", size);
		return;
	}
	sz = pathFile.Read(buf, size);
	str.UnlockBuffer(sz);
	vector<BmString> pathVect;
	split( "\n", str, pathVect);
	BmRef<BmMailRef> ref;
	BmRef<BmMail> mail;
	entry_ref eref;
	BEntry entry;
	unsigned int pvs = min_c(pathVect.size(), Limit);
	int perfs = PerfCount >= 0 ? min_c(PerfCount, (int32)pvs) : pvs/5;
	char resultBuf[pvs];
	for(uint32 i=0; i<pvs; ++i) {
		Out("%s...", pathVect[i].String());
		if ((res = entry.SetTo(pathVect[i].String())) != B_OK) {
			fprintf(stderr, "%s: %s\n", pathVect[i].String(), strerror(res));
			continue;
		}
		entry.GetRef(&eref);
		ref = BmMailRef::CreateInstance(eref);
		if (!ref)
			continue;
		mail = BmMail::CreateInstance(ref.Get());
		if (!mail)
			continue;
		mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
		if (mail->InitCheck() != B_OK) {
			// couldn't read this mail, ignore it:
			Out("### couldn't read! ###\n");
			continue;
		}
		Out("%ld...", mail->RawText().Length());
		BmMsgContext result;
		result.mail = mail.Get();
		spamAddon->Execute( &result, &ClassifyJob);
		bool isSpam = result.data.FindBool("IsSpam");
		bool isTofu = result.data.FindBool("IsTofu");
		bool isReinforced = result.data.FindBool("IsReinforced");
		double overallPr = 0.0;
		result.data.FindDouble("OverallPr", &overallPr);
		if (isSpam)
			Out("SPAM(%f)...", overallPr);
		else if (isTofu)
			Out("TOFU(%f)...", overallPr);
		if (pathVect[i].IFindFirst("spam") >= B_OK) {
			if (overallPr >= 0) {
				// false negative
				if (isReinforced)
					// in order to trigger neccessary unlearning, we mark this
					// message (only in memory) as being classified as tofu:
					mail->MailRef()->Classification("Genuine");
				spamAddon->Execute( &result, &LearnAsSpamJob);
				resultBuf[i] = 'N';
				Out("laS");
			} else if (isReinforced) {
				resultBuf[i] = 's';
				Out("riS");
			} else if (!isSpam) {
				// unsure region
				resultBuf[i] = 'U';
				Out("uns");
			} else {
				resultBuf[i] = 'S';
				Out("ok");
			}
		} else {
			if (overallPr < 0) {
				// false positive
//				if (i>=pvs-perfs)
//					fprintf(stderr, "FP: %s\n", eref.name);
				if (isReinforced)
					// in order to trigger neccessary unlearning, we mark this
					// message (only in memory) as being classified as spam:
					mail->MailRef()->Classification("Spam");
				spamAddon->Execute( &result, &LearnAsTofuJob);
				resultBuf[i] = 'P';
				Out("laT");
			} else if (isReinforced) {
				resultBuf[i] = 't';
				Out("riT");
			} else if (!isTofu) {
				// unsure region
				resultBuf[i] = 'U';
				Out("uns");
			} else {
				resultBuf[i] = 'T';
				Out("ok");
			}
		}
		Out("\n");
	}
	if (perfs) {
		// compute performance of last perfs entries:
		for( uint32 r=pvs-perfs; r<pvs; ++r) {
			switch(resultBuf[r]) {
				case 'P': ri.falsePos++; break;
				case 'N': ri.falseNeg++; break;
				case 'S': ri.corrSpam++; break;
				case 'T': ri.corrTofu++; break;
				case 's': ri.reinfSpam++; break;
				case 't': ri.reinfTofu++; break;
				case 'U': ri.unsure++; break;
			}
		}
		if (!Verbose)
			printf(" \n");
		ri.totalFalsePos = 100.0-ri.falsePos*100.0/perfs;
		ri.totalOverall = 100.0-(ri.falsePos+ri.falseNeg)*100.0/perfs;
		fprintf(stderr,"%s done (%d messages in %ld secs)\n", pathfileName,
							pvs, time(NULL)-starttime);
		fprintf(stderr,"\tsetup: reinforce=(S:%ld/T:%ld) unsure=(S:%ld/T:%ld) dehtml=%s keep-atags=%s\n", 
							ThresholdForSpam, ThresholdForTofu,
							UnsureForSpam, UnsureForTofu,
							DeHtml ? "yes" : "no", KeepATags ? "yes" : "no");
		fprintf(stderr,"\tperformance of last %d messages:\n", perfs);
		fprintf(stderr,"\tsimple spam:      %6.2f   simple tofu:       %6.2f\n", ri.corrSpam, ri.corrTofu);
		fprintf(stderr,"\treinforced spam:  %6.2f   reinforced tofu:   %6.2f\n", ri.reinfSpam, ri.reinfTofu);
		fprintf(stderr,"\tfalse positives:  %6.2f   false negatives:   %6.2f\n", ri.falsePos, ri.falseNeg);
		fprintf(stderr,"\tunsure:           %6.2f\n", ri.unsure);
		fprintf(stderr,"\tcorrectness (FP): %6.2f   correctness (all): %6.2f\n", ri.totalFalsePos, ri.totalOverall);
	}
}

int 
main( int argc, char** argv) 
{
	int as = 1;
	while( as<argc && *argv[as] == '-') {
		if (!strcmp(argv[as], "--verbose"))
			Verbose = true;
		else if (!strncmp(argv[as], "--reinforce-spam-threshold=", 27)
		|| !strncmp(argv[as], "--rst=", 6))
			ThresholdForSpam = atoi(strchr(argv[as],'=')+1);
		else if (!strncmp(argv[as], "--reinforce-tofu-threshold=", 27)
		|| !strncmp(argv[as], "--rtt=", 6))
			ThresholdForTofu = atoi(strchr(argv[as],'=')+1);
		else if (!strncmp(argv[as], "--unsure-spam-threshold=", 24)
		|| !strncmp(argv[as], "--ust=", 6))
			UnsureForSpam = atoi(strchr(argv[as],'=')+1);
		else if (!strncmp(argv[as], "--unsure-tofu-threshold=", 24)
		|| !strncmp(argv[as], "--utt=", 6))
			UnsureForTofu = atoi(strchr(argv[as],'=')+1);
		else if (!strncmp(argv[as], "--limit=", 8))
			Limit = atoi(argv[as]+8);
		else if (!strncmp(argv[as], "--perf-count=", 13))
			PerfCount = atoi(argv[as]+13);
		else if (!strcmp(argv[as], "--statistics"))
			Statistics = true;
		else if (!strcmp(argv[as], "--dehtml"))
			DeHtml = true;
		else if (!strcmp(argv[as], "--keep-atags"))
			KeepATags = true;
		else 
			fprintf(stderr, "unknown option %s ignored\n", argv[as]);
		as++;
	}
	if (argc>as) {
		ResetJob.AddString("jobSpecifier", "Reset");
		ClassifyJob.AddString("jobSpecifier", "Classify");
		LearnAsSpamJob.AddString("jobSpecifier", "LearnAsSpam");
		LearnAsTofuJob.AddString("jobSpecifier", "LearnAsTofu");
		GetStatisticsJob.AddString("jobSpecifier", "GetStatistics");
		if (DeHtml) {
			ClassifyJob.AddBool("DeHtml", true);
			LearnAsSpamJob.AddBool("DeHtml", true);
			LearnAsTofuJob.AddBool("DeHtml", true);
		}
		if (KeepATags) {
			ClassifyJob.AddBool("KeepATags", true);
			LearnAsSpamJob.AddBool("KeepATags", true);
			LearnAsTofuJob.AddBool("KeepATags", true);
		}
		ClassifyJob.AddInt32("ThresholdForSpam", ThresholdForSpam);
		ClassifyJob.AddInt32("ThresholdForTofu", ThresholdForTofu);
		ClassifyJob.AddInt32("UnsureForSpam", UnsureForSpam);
		ClassifyJob.AddInt32("UnsureForTofu", UnsureForTofu);
		
		ResInfo resInfo[argc];
		const char* APP_SIG = "application/x-vnd.zooey-spamometer";
		BmApplication* app = new BmApplication( APP_SIG, true);
		TheFilterList->StartJobInThisThread();
	
		{
			BmRef<BmFilter> anySpamFilter = TheFilterList->LearnAsSpamFilter();
			// remove data-files (start with empty database):
			spamAddon = anySpamFilter->Addon();
			if (!spamAddon) {
				fprintf(stderr, "could not access spam-filter-addon (not loaded)!\n");
				exit(5);
			}
		}
	
		for( int a=as; a<argc; ++a)
			SpamOMeter( argv[a], resInfo[a]);
	
		ResInfo& ri = resInfo[0];
		for( int r=as; r<argc; ++r) {
			ri.corrSpam += resInfo[r].corrSpam;
			ri.corrTofu += resInfo[r].corrTofu;
			ri.reinfSpam += resInfo[r].reinfSpam;
			ri.reinfTofu += resInfo[r].reinfTofu;
			ri.falseNeg += resInfo[r].falseNeg;
			ri.falsePos += resInfo[r].falsePos;
			ri.unsure += resInfo[r].unsure;
			ri.totalFalsePos += resInfo[r].totalFalsePos;
			ri.totalOverall += resInfo[r].totalOverall;
		}
		ri.corrSpam /= argc-as;
		ri.corrTofu /= argc-as;
		ri.reinfSpam /= argc-as;
		ri.reinfTofu /= argc-as;
		ri.falseNeg /= argc-as;
		ri.falsePos /= argc-as;
		ri.unsure /= argc-as;
		ri.totalFalsePos /= argc-as;
		ri.totalOverall /= argc-as;
		fprintf(stderr,"############################################################\n");
		fprintf(stderr,"overall mean values:\n");
		fprintf(stderr,"\tsetup: reinf=(S:%ld/T:%ld) unsure=(S:%ld/T:%ld) dehtml=%s keep-atags=%s\n", 
							ThresholdForSpam, ThresholdForTofu,
							UnsureForSpam, UnsureForTofu,
							DeHtml ? "yes" : "no", KeepATags ? "yes" : "no");
		fprintf(stderr,"\tsimple spam:      %6.2f   simple tofu:       %6.2f\n", ri.corrSpam, ri.corrTofu);
		fprintf(stderr,"\treinforced spam:  %6.2f   reinforced tofu:   %6.2f\n", ri.reinfSpam, ri.reinfTofu);
		fprintf(stderr,"\tfalse positives:  %6.2f   false negatives:   %6.2f\n", ri.falsePos, ri.falseNeg);
		fprintf(stderr,"\tunsure:           %6.2f\n", ri.unsure);
		fprintf(stderr,"\tcorrectness (FP): %6.2f   correctness (all): %6.2f\n", ri.totalFalsePos, ri.totalOverall);
		fprintf(stderr,"############################################################\n");

		if (Statistics) {
			BmMsgContext result;
			spamAddon->Execute( &result, &GetStatisticsJob);
			int32	spamBuckets = result.data.FindInt32("SpamBuckets");
			int32	spamBucketsUsed = result.data.FindInt32("SpamBucketsUsed");
			int32	spamLearnings = result.data.FindInt32("SpamLearnings");
			int32	spamClassifications = result.data.FindInt32("SpamClassifications");
			int32	spamMistakes = result.data.FindInt32("SpamMistakes");
			int32	spamAverageValue = result.data.FindInt32("SpamAverageValue");
			int32	spamChainsAverageLength = result.data.FindInt32("SpamChainsAverageLength");
			int32	tofuBuckets = result.data.FindInt32("TofuBuckets");
			int32	tofuBucketsUsed = result.data.FindInt32("TofuBucketsUsed");
			int32	tofuLearnings = result.data.FindInt32("TofuLearnings");
			int32	tofuClassifications = result.data.FindInt32("TofuClassifications");
			int32	tofuMistakes = result.data.FindInt32("TofuMistakes");
			int32	tofuAverageValue = result.data.FindInt32("TofuAverageValue");
			int32	tofuChainsAverageLength = result.data.FindInt32("TofuChainsAverageLength");
			double errorsFP = spamClassifications 
										? 100.0*spamMistakes/(float)spamClassifications
										: 0;
			double errorsAll = (spamClassifications+tofuClassifications)
										? 100.0*(tofuMistakes+spamMistakes)
											/(double)(tofuClassifications+spamClassifications)
										: 0;
			fprintf(stderr,"************************************************************\n");
			fprintf(stderr, "          SPAM-datafile       |          TOFU-datafile\n");
			fprintf(stderr, " features: %6ld (%3ld%% used) | features: %6ld (%3ld%% used)\n",
									spamBuckets, 100*spamBucketsUsed/spamBuckets, 
									tofuBuckets, 100*tofuBucketsUsed/tofuBuckets);
			fprintf(stderr, " average value:       %6ld  | average value:        %6ld\n",
									spamAverageValue, tofuAverageValue);
			fprintf(stderr, " average chain-length: %5ld  | average chain-length:  %5ld\n",
									spamChainsAverageLength, tofuChainsAverageLength);
			fprintf(stderr, " learnings:           %6ld  | learnings:            %6ld\n",
									spamLearnings, tofuLearnings);
			fprintf(stderr, " classifications:     %6ld  | classifications:      %6ld\n",
									spamClassifications, tofuClassifications);
			fprintf(stderr, " mistakes:            %6ld  | mistakes:             %6ld\n",
									spamMistakes, tofuMistakes);
			fprintf(stderr, " correctness(FP):     %6.2f  | correctness (all):    %6.2f\n",
									100.0-errorsFP, 100.0-errorsAll);
			fprintf(stderr,"************************************************************\n");
		}
		TheFilterList = NULL;
		delete app;
	} else
		fprintf(stderr, "usage:\n\t%s [--verbose] [--reinforce-threshold=<num>] "
				  "path-files\n", argv[0]);
}
