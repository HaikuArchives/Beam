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
#include <Entry.h>
#include <File.h>
#include <Query.h>

#include "split.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmFilter.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmMailRef.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"

static bool Verbose = false;
static bool Statistics = false;

static bool TrainingMode = false;

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

static vector<BmString> TrainingMailsSpam;
static vector<BmString> TrainingMailsTofu;
static const uint16 MaxTrainingCount = 1000;
static const uint16 MinTrainingCount =   10;
static uint16 TrainCount = 0;

/*------------------------------------------------------------------------------*\
	FindTrainingMails()
		-	
\*------------------------------------------------------------------------------*/
status_t FindTrainingMails( BmString &msgFileName)
{
	int32 count;
	status_t err;
	dirent* dent;
	entry_ref eref;
	BEntry entry;
	BPath path;
	BNode node;
	BQuery query;
	BmString pathStr;
	BmString classif;
	bool isSpam;
	char buf[4096];

	time_t now = time(NULL);
	time_t then = now-60*60*24*365;		// one year ago

	TrainingMailsSpam.clear();
	TrainingMailsTofu.clear();

	if (!Verbose)
		printf("querying...");

	query.SetVolume(&ThePrefs->MailboxVolume);
	query.PushAttr("MAIL:when");
	query.PushUInt32((uint32)then);
	query.PushOp(B_GT);
	if ((err = query.Fetch()) != B_OK)
		return err;
	bool done = false;
	while ((count = query.GetNextDirents((dirent* )buf, 4096)) > 0 && !done) {
		dent = (dirent* )buf;
		while (count-- > 0 && !done) {
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name(dent->d_name);
			entry.SetTo(&eref);
			entry.GetPath(&path);
			pathStr = path.Path();
			node.SetTo(&eref);
			classif = "";
			BmReadStringAttr( &node, BM_MAIL_ATTR_CLASSIFICATION, classif);
			isSpam = false;
			Out("found mail '%s' => ", eref.name);
			if (!classif.ICompare("Spam"))
				isSpam = true;
			else {
				int32 pos = pathStr.IFindFirst("spam/");
				if (pos >= 0 && (pos==0 || pathStr[pos-1]=='/'))
					isSpam = true;
			}
			if (isSpam) {
				Out("SPAM\n");
				if (TrainingMailsSpam.size() < MaxTrainingCount)
					TrainingMailsSpam.push_back(pathStr);
			} else {
				Out("TOFU\n");
				if (TrainingMailsTofu.size() < MaxTrainingCount)
					TrainingMailsTofu.push_back(pathStr);
			}
			if (TrainingMailsTofu.size() == MaxTrainingCount 
			&& TrainingMailsSpam.size() == MaxTrainingCount)
				done = true;
			
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}

	if (!Verbose)
		printf(" \n");

	TrainCount = min_c(TrainingMailsSpam.size(), TrainingMailsTofu.size());
	if (TrainCount < MinTrainingCount)
		// no mails found for training
		return B_BAD_VALUE;

	printf("Starting training with %u SPAM- & TOFU-mails\n", TrainCount);

	BmString paths;
	for( uint16 i=0; i<TrainCount; ++i) {
		paths << TrainingMailsTofu[i] << "\n";
		paths << TrainingMailsSpam[i] << "\n";
	}

	msgFileName = "/boot/var/tmp/SPAM-Filter-Training";
	TheTempFileList.AddFile(msgFileName);
	{
		BFile msgFile(msgFileName.String(), B_READ_WRITE | B_CREATE_FILE);
		msgFile.Write( paths.String(), paths.Length());
	}

	return B_OK;
}

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
	char* resultBuf = new char [pvs];
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
		// Learning as Tofu/Spam actually skips the learning if a mail has
		// already been marked as such. However, we are simulating a fresh
		// training session here, so in order to be able to execute learnAsSpam
		// and learnAsTofu, we force it:
		result.SetBool("ForceLearning", true);
		// now classify this message:
		result.mail = mail.Get();
		spamAddon->Execute( &result, &ClassifyJob);
		bool isSpam = result.GetBool("IsSpam");
		bool isTofu = result.GetBool("IsTofu");
		bool isReinforced = result.GetBool("IsReinforced");
		double overallPr = result.GetDouble("OverallPr");
		if (isSpam)
			Out("SPAM(%f)...", overallPr);
		else if (isTofu)
			Out("TOFU(%f)...", overallPr);
		if (pathVect[i].IFindFirst("spam/") >= B_OK 
		|| mail->MailRef()->Classification().ICompare("Spam") == 0) {
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
	delete [] resultBuf;
}

int 
main( int argc, char** argv) 
{
	BmString exitVal;
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
		else if (!strcmp(argv[as], "--do-training"))
			TrainingMode = true;
		else 
			fprintf(stderr, "unknown option %s ignored\n", argv[as]);
		as++;
	}
	if (argc<=as && !TrainingMode) {
		fprintf(stderr, "usage:\n\t%s [options] path-files\n", argv[0]);
		fprintf(stderr, "where options can be any combination of:\n"
				  "\t[--dehtml]\n"
				  "\t\tremove html-tags from mails before classifying\n"
				  "\t[--do-training]\n"
				  "\t\tquery for mails and automatically start training\n"
				  "\t[--keep-atags]\n"
				  "\t\tkeep <a>-tags (links) when removing html\n"
				  "\t[--limit=<num>]\n"
				  "\t\tstop after <num> mails have been classified\n"
				  "\t[--perf-count=<num>]\n"
				  "\t\tdo performance measurement for last <num> mails\n"
				  "\t\t(the default is one fifth of total number of mails)\n"
				  "\t[--reinforce-spam-threshold=<num>]\n"
				  "\t\tdo auto-learning if classification is below <num> for SPAM\n"
				  "\t[--reinforce-tofu-threshold=<num>]\n"
				  "\t\tdo auto-learning if classification is below <num> for TOFU\n"
				  "\t[--unsure-spam-threshold=<num>]\n"
				  "\t\ttreat mail as unsure if classification is below <num> for SPAM\n"
				  "\t[--unsure-tofu-threshold=<num>]\n"
				  "\t\ttreat mail as unsure if classification is below <num> for TOFU\n"
				  "\t[--statistics]\n" 
				  "\t\tshow resulting SPAM-filter statistics\n"
				  "\t[--verbose]\n"
				  "\t\tprint each mail and its classification-result\n");
		exit(10);
	}

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
	
	if (TrainingMode) {
		// make room for faked argument:
		argc++;
	};

	ResInfo* resInfo = new ResInfo [argc];
	const char* APP_SIG = "application/x-vnd.zooey-spamometer";
	BmApplication* app 
		= new BmApplication( APP_SIG, TrainingMode ? false : true);
	TheFilterList->StartJobInThisThread();

	{
		BmRef<BmFilter> anySpamFilter = TheFilterList->LearnAsSpamFilter();
		// remove data-files (start with empty database):
		spamAddon = anySpamFilter->Addon();
		if (!spamAddon) {
			fprintf(stderr, "could not access spam-filter-addon (not loaded)!\n");
			exitVal = "failed";
			goto out;
		}
	}
	
	if (TrainingMode) {
		BmString pathsFileName;
		printf("%c]2;Beam SPAM-Filter Training Session   (querying for mails...)%c\n", 27, 7);
		status_t err = FindTrainingMails( pathsFileName);
		if (err == B_OK) {
			printf("%c]2;Beam SPAM-Filter Training Session   (training...)%c\n", 27, 7);
			SpamOMeter( pathsFileName.String(), resInfo[as]);
			printf("%c]2;Beam SPAM-Filter Training Session   (done!)%c\n", 27, 7);
			fprintf(stderr, "%u mails have been succesfully trained.\n", TrainCount);
		} else if (err == B_BAD_VALUE) {
			printf("%c]2;Beam SPAM-Filter Training Session   (error!)%c\n", 27, 7);
			fprintf(stderr, 
					  "There aren't enough mails for training available.\n"
					  "At least %u SPAM- & TOFU-mails are required.\n"
					  "No training has been done!\n", MinTrainingCount);
			exitVal = "too few";
			goto out;
		} else {
			printf("%c]2;Beam SPAM-Filter Training Session   (error!)%c\n", 27, 7);
			fprintf(stderr, 
					  "Couldn't query for training mails.\n"
					  "Error: %s\n"
					  "No training has been done!\n", strerror(err));
			exitVal = "failed";
			goto out;
		}
	} else {
		for( int a=as; a<argc; ++a)
			SpamOMeter( argv[a], resInfo[a]);
	}

	if (argc-as > 1) {
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
	}

	if (Statistics) {
		BmMsgContext result;
		spamAddon->Execute( &result, &GetStatisticsJob);
		int32	spamBuckets = result.GetInt32("SpamBuckets");
		int32	spamBucketsUsed = result.GetInt32("SpamBucketsUsed");
		int32	spamLearnings = result.GetInt32("SpamLearnings");
		int32	spamClassifications = result.GetInt32("SpamClassifications");
		int32	spamMistakes = result.GetInt32("SpamMistakes");
		int32	spamAverageValue = result.GetInt32("SpamAverageValue");
		int32	spamChainsAverageLength = result.GetInt32("SpamChainsAverageLength");
		int32	tofuBuckets = result.GetInt32("TofuBuckets");
		int32	tofuBucketsUsed = result.GetInt32("TofuBucketsUsed");
		int32	tofuLearnings = result.GetInt32("TofuLearnings");
		int32	tofuClassifications = result.GetInt32("TofuClassifications");
		int32	tofuMistakes = result.GetInt32("TofuMistakes");
		int32	tofuAverageValue = result.GetInt32("TofuAverageValue");
		int32	tofuChainsAverageLength = result.GetInt32("TofuChainsAverageLength");
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
	exitVal = "ok";
out:
	TheFilterList = NULL;
	delete app;
	delete [] resInfo;
	BFile resultFile("/boot/var/tmp/bm_spamometer_results", 
						  B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);
	resultFile.Write(exitVal.String(), exitVal.Length());
	return 0;
}
