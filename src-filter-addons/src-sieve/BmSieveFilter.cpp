/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Alert.h>
#include <Application.h>
#include <File.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>

#include <VGroup.h>
#include <HGroup.h>
#include <Space.h>
#include <MBorder.h>
#include <MButton.h>
#include <MStringView.h>
#include <LayeredGroup.h>

#include "regexx.hh"
#include "split.hh"
using namespace regexx;

extern "C" {
#undef DOMAIN
#include "script.h"
#include "tree.h"
#include "sieve.h"
}

#include "BubbleHelper.h"

#include "BmLogHandler.h"
#include "BmSieveFilter.h"
#include "BmCheckControl.h"
#include "BmMail.h"
#include "BmMenuAlert.h"
#include "BmMenuControl.h"
#include "BmMenuControllerBase.h"
#include "BmMultiLineTextControl.h"
#include "BmRosterBase.h"
#include "BmTextControl.h"


extern "C"
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind);

extern "C"
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind);

/********************************************************************************\
	BmSieveFilter
\********************************************************************************/

const char* const BmSieveFilter::MSG_VERSION = 		"bm:version";
const char* const BmSieveFilter::MSG_CONTENT = 		"bm:content";
const int16 BmSieveFilter::nArchiveVersion = 1;
BLocker* BmSieveFilter::nSieveLock = NULL;

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

const char* FILTER_SIEVE 			= "Sieve";
const char* FILTER_SIEVE_SCRIPT 	= "Sieve-script";

extern "C" {
	const char* FilterKinds[] = {
		FILTER_SIEVE,
		FILTER_SIEVE_SCRIPT,
		NULL
	};
}

static const BmString BmNotifySetIdentity = "BeamSetIdentity";
static const BmString BmNotifySetStatus = "BeamSetStatus";
static const BmString BmNotifyStopProcessing = "BeamStopProcessing";
static const BmString BmNotifySetSpamTofu = "BeamSetSpamTofu";
static const BmString BmNotifySetListId = "BeamSetListId";

/*------------------------------------------------------------------------------*\
	BmSieveFilter( archive)
		-	c'tor
		-	constructs a BmSieveFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmSieveFilter::BmSieveFilter( const BmString& name, const BMessage* archive) 
	:	mName( name)
	,	mCompiledScript( NULL)
	,	mSieveInterp( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mContent = archive->FindString( MSG_CONTENT);
}

/*------------------------------------------------------------------------------*\
	~BmSieveFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmSieveFilter::~BmSieveFilter() {
	if (mCompiledScript)
		sieve_script_free( &mCompiledScript);
	if (mSieveInterp)
		sieve_interp_free( &mSieveInterp);
}

/*------------------------------------------------------------------------------*\
	SieveLock()
\*------------------------------------------------------------------------------*/
BLocker* BmSieveFilter::SieveLock() {
	if (!nSieveLock)
		nSieveLock = new BLocker( "SieveLock", true);
	return nSieveLock;
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSieveFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmSieveFilter::Archive( BMessage* archive, bool) const {
	status_t ret = (archive->AddInt16( MSG_VERSION, nArchiveVersion)
		||	archive->AddString( MSG_CONTENT, mContent.String()));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Execute()
		-	
\*------------------------------------------------------------------------------*/
bool 
BmSieveFilter::Execute( BmMsgContext* msgContext, const BMessage* /*jobSpecs*/)
{
	BmString mailId;
	if (msgContext)
		mailId = msgContext->mail->Name();
	BM_LOG2( BM_LogFilter, BmString("Sieve-Addon: asked to execute filter <") 
									<< Name() 
									<< "> on mail with Id <" << mailId << ">");

	// set lock to serialize SIEVE-calls:
	BAutolock lock( SieveLock());
	if (!lock.IsLocked()) {
		mLastErr = "Unable to get SIEVE-lock";
		return false;
	}

	if (!mCompiledScript) {
		bool scriptOK = CompileScript();
		if (!scriptOK || !mCompiledScript) {
			BmString errString = LastErr() + "\n" 
										<< "Error: " 
										<< sieve_strerror(LastErrVal()) 
										<< "\n"
										<< LastSieveErr();
			BM_LOGERR( BmString("Sieve-Addon: compilation failed.\n")<<errString);
			return false;
		}
	}
	BM_LOG2( BM_LogFilter, "Sieve-Addon: starting execution of script...");
	int res = sieve_execute_script( mCompiledScript, msgContext);
	BM_LOG2( BM_LogFilter, "Sieve-Addon: done with script.");
	return res == SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	CompileScript()
		-	
\*------------------------------------------------------------------------------*/
bool BmSieveFilter::CompileScript() {
	bool ret = false;
	FILE* scriptFile = NULL;
	BmString scriptFileName;
	status_t err;
	BFile scriptBFile;

	BM_LOG( BM_LogFilter, 
			  BmString("Sieve-Addon: compiling SIEVE-script of filter ") 
					<< Name()); 

	// set lock to serialize SIEVE-calls:
	BAutolock lock( SieveLock());
	if (!lock.IsLocked()) {
		mLastErr = "Unable to get SIEVE-lock";
		return false;
	}
	if (mCompiledScript)
		// script has already been compiled
		return true;

	mLastErr = mLastSieveErr = "";

	BM_LOG2( BM_LogFilter, "Sieve-Addon: compilation...register");

	// create sieve interpreter:
	int res = sieve_interp_alloc( &mSieveInterp, this);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Name()) << ": Could not create SIEVE-interpreter";
		goto cleanup;
	}
	RegisterCallbacks( mSieveInterp);

	BM_LOG2( BM_LogFilter, "Sieve-Addon: compilation...create tmpfile");
	// create temporary file with script-contents 
	// (since SIEVE parses from file only):
	scriptFileName = "/tmp/bm_sieve_script";
	err = scriptBFile.SetTo( scriptFileName.String(), 
									 B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (err != B_OK) {
		mLastErr = BmString(Name()) << ":\nCould not create temporary file\n\t<" 
											 << scriptFileName << ">\n\n Result: " 
											 << strerror(err);
		goto cleanup;
	}
	scriptBFile.Write( mContent.String(), mContent.Length());
	scriptBFile.Unset();

	BM_LOG2( BM_LogFilter, "Sieve-Addon: compilation...open tmpfile for sieve");
	// open script file for sieve...
	scriptFile = fopen( scriptFileName.String(), "r");
	if (!scriptFile) {
		mLastErr = BmString(Name()) << ":\nCould not re-open file \n\t<" 
											 << scriptFileName << ">";
		goto cleanup;
	}
	// ...and compile the script:	
	if (mCompiledScript) {
		sieve_script_free( &mCompiledScript);
		mCompiledScript = NULL;
	}
	BM_LOG2( BM_LogFilter, "Sieve-Addon: compilation...parsing");
	res = sieve_script_parse( mSieveInterp, scriptFile, this, &mCompiledScript);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Name()) 
							<< ":\nThe script could not be parsed correctly";
		goto cleanup;
	}
	BM_LOG2( BM_LogFilter, "Sieve-Addon: compilation...done");
	ret = true;

cleanup:
	mLastErrVal = res;
	if (scriptFile)
		fclose( scriptFile);
	return ret;
}

/*------------------------------------------------------------------------------*\
	Content()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilter::Content( const BmString &s)
{
	mContent = s;
	if (mCompiledScript) {
		sieve_script_free(&mCompiledScript);
		mCompiledScript = NULL; 
	}
}

/*------------------------------------------------------------------------------*\
	RegisterCallbacks()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilter::RegisterCallbacks( sieve_interp_t* interp) {
	sieve_register_redirect( interp, BmSieveFilter::sieve_redirect);
	sieve_register_keep( interp, BmSieveFilter::sieve_keep);
	sieve_register_discard( interp, BmSieveFilter::sieve_discard);
	sieve_register_fileinto( interp, BmSieveFilter::sieve_fileinto);
	sieve_register_reject( interp, BmSieveFilter::sieve_reject);
	sieve_register_notify( interp, BmSieveFilter::sieve_notify);
	sieve_register_imapflags( interp, NULL);

	sieve_register_size( interp, BmSieveFilter::sieve_get_size);
	sieve_register_header( interp, BmSieveFilter::sieve_get_header);

	sieve_register_execute_error( interp, BmSieveFilter::sieve_execute_error);

	sieve_register_parse_error( interp, BmSieveFilter::sieve_parse_error);
}

/*------------------------------------------------------------------------------*\
	sieve_parse_error()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_parse_error( int lineno, const char* msg, 
												  void*, void* script_context) {
	if (script_context) {
		BmSieveFilter* filter = static_cast< BmSieveFilter*>( script_context);
		if (filter)
			filter->mLastSieveErr = BmString("Line ")<<lineno<<": "<<msg;
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_parse_error()
		-	
\*------------------------------------------------------------------------------*/
BmString BmSieveFilter::ErrorString() const {
	return LastErr() + "\n\n" 
				<< "Error: " << sieve_strerror(LastErrVal()) << "\n\n"
				<< LastSieveErr();
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	prepares the filter for execution (compiles the script).
\*------------------------------------------------------------------------------*/
void BmSieveFilter::Initialize()
{
	CompileScript();
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmSieveFilter::SanityCheck( BmString& complaint, BmString& fieldName) {
	if (!CompileScript()) {
		complaint = ErrorString();
		fieldName = "content";
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	sieve_redirect()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_redirect( void* /*action_context*/, 
											  void* /*interp_context*/, 
			   							  void* /*script_context*/, 
			   							  void* /*message_context*/, 
			   							  const char** /*errmsg*/) {
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	SetMailFlags()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilter::SetMailFlags( sieve_imapflags_t* flags, 
											 BmMsgContext* msgContext) {
	if (msgContext && flags) {
		for( int i=0; i<flags->nflags; ++i) {
			BmString flag( flags->flag[i]);
			BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: SetMailFlags called "
													  "with flag ") << flag); 
			if (!flag.ICompare( "\\Seen"))
				msgContext->SetString("Status", "Read");
		}
	}
}

/*------------------------------------------------------------------------------*\
	sieve_keep()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_keep( void* action_context, void*, 
			   				  		 void*, void* message_context, 
			   				  		 const char**) {
	BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: sieve_keep called")); 
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_keep_context* keepContext 
		= static_cast< sieve_keep_context*>( action_context);
	if (msgContext && keepContext)
		SetMailFlags( keepContext->imapflags, msgContext);
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_discard()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_discard( void* /*action_context*/, void*, 
			   				  		    void*, void* message_context, 
			   				  		    const char**) {
	BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: sieve_discard called")); 
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext)
		msgContext->SetBool("MoveToTrash", true);
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_fileinto()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_fileinto( void* action_context, void* script_context, 
			   				  			 void*, void* message_context, 
			   				 			 const char**) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_fileinto_context* fileintoContext 
		= static_cast< sieve_fileinto_context*>( action_context);
	BmSieveFilter* filter = static_cast< BmSieveFilter*>( script_context);
	if (msgContext && filter && fileintoContext) {
		BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: sieve_fileinto called "
												  "with folder ")
												  <<fileintoContext->mailbox);
		SetMailFlags( fileintoContext->imapflags, msgContext);
		if (msgContext->mail->Outbound() && filter->AskBeforeFileInto()) {
			BmMenuAlert* alert = new BmMenuAlert( 
				300, 100, "File into folder", 
				"Please select the folder this mail\n should be filed into:",
				new BmMenuControl(
					"Target folder:",
					new BmMenuControllerBase( 
						fileintoContext->mailbox, NULL,
						new BMessage( BM_MENUITEM_SELECTED), 
						&BmGuiRosterBase::RebuildFolderMenu
					)
				),
				"Cancel", "OK"
			);
			alert->SetShortcut( 0, B_ESCAPE);
			char pathbuf[1024];
			if (alert->Go( pathbuf, 1024) == 1)
				msgContext->SetString("FolderName", pathbuf);
		} else
			msgContext->SetString("FolderName", fileintoContext->mailbox);
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_reject()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_reject( void* action_context, void* script_context, 
			   				  			void*, void* message_context, 
			   				 			const char**) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_reject_context* rejectContext 
		= static_cast< sieve_reject_context*>( action_context);
	BmSieveFilter* filter = static_cast< BmSieveFilter*>( script_context);
	if (msgContext && filter && rejectContext) {
		BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: sieve_reject called "
												  "with msg ")
												  <<rejectContext->msg);
		msgContext->SetString("RejectMsg", rejectContext->msg);
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_notify()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_notify( void* action_context, void*, 
			   				  		   void*, void* message_context, 
			   				  		   const char**) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_notify_context* notifyContext 
		= static_cast< sieve_notify_context*>( action_context);
	if (msgContext && notifyContext) {
		BM_LOG3( BM_LogFilter, BmString("Sieve-Addon: sieve_notify called with "
												  "method ")<<notifyContext->method);
		for( int i=0; notifyContext->options[i]; ++i) {
			BM_LOG3( BM_LogFilter, BmString("option[")<<i<<"] = "
												<< notifyContext->options[i]);
		}
		if (!BmNotifySetIdentity.ICompare( notifyContext->method)) {
			// set identity for mail according to notify-options:
			msgContext->SetString("Identity", notifyContext->options[0]);
		} else if (!BmNotifySetStatus.ICompare( notifyContext->method)) {
			// set status for mail according to notify-options:
			msgContext->SetString("Status", notifyContext->options[0]);
		} else if (!BmNotifyStopProcessing.ICompare( notifyContext->method)) {
			// we stop processing since this filter has matched:
			msgContext->SetBool("StopProcessing", true);
		} else if (!BmNotifySetSpamTofu.ICompare( notifyContext->method)) {
			// set spam/tofu-state for mail according to notify-options:
			BmString spam("Spam");
			BmString tofu("Tofu");
			if (!spam.ICompare(notifyContext->options[0])) {
				msgContext->SetBool("LearnAsSpam", true);
				msgContext->SetBool("LearnAsTofu", false);
			} else if (!tofu.ICompare(notifyContext->options[0])) {
				msgContext->SetBool("LearnAsSpam", false);
				msgContext->SetBool("LearnAsTofu", true);
			}
		} else if (!BmNotifySetListId.ICompare( notifyContext->method)) {
			// set list-id for mail according to notify-options:
			msgContext->SetString("ListId", notifyContext->options[0]);
		}
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_size()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_get_size( void* message_context, int* sizePtr) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext && sizePtr)
		*sizePtr = msgContext->mail->RawText().Length();
	BM_LOG3( BM_LogFilter, 
				BmString("Sieve-Addon: sieve_get_size called, answer = ")
					<< msgContext->mail->RawText().Length());
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_header()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_get_header( void* message_context, const char* header,
			  									 const char*** contentsPtr) {
	BM_LOG3( BM_LogFilter, 
				BmString("Sieve-Addon: sieve_get_header called for header ")
					<< header);
	static const char* fakes[2] = { NULL, NULL };
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext && contentsPtr && header) {
		*contentsPtr = NULL;
		BmString headerName( header);
		if (headerName.ICompare("Status") == 0) {
			fakes[0] = msgContext->mail->Status().String();
			*contentsPtr = fakes;
		} else if (headerName.ICompare("Account") == 0) {
			fakes[0] = msgContext->mail->AccountName().String();
			*contentsPtr = fakes;
		} else if (headerName.ICompare("Outbound") == 0) {
			fakes[0] = msgContext->mail->Outbound() ? "true" : "false";
			*contentsPtr = fakes;
		} else {
			if (!msgContext->headerInfos)
				msgContext->mail->Header()->GetAllFieldValues( *msgContext);
			for( int i=0; i<msgContext->headerInfoCount; ++i) {
				if (!msgContext->headerInfos[i].fieldName.ICompare( header)) {
					*contentsPtr = msgContext->headerInfos[i].values;
					for( int v=0; msgContext->headerInfos[i].values[v]; ++v) {
						BM_LOG3( BM_LogFilter, 
									BmString("Sieve-Addon: sieve_get_header returns value[")
										<<v<<"] = " <<msgContext->headerInfos[i].values[v]);
					}
					break;
				}
			}
		}
		return *contentsPtr != NULL ? SIEVE_OK : SIEVE_FAIL;
	}
	return SIEVE_FAIL;
}

/*------------------------------------------------------------------------------*\
	execute_error()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_execute_error( const char* msg, void*,
													 void* script_context, 
													 void* message_context) {
	BmString filterName = "<unknown>";
	BmString mailName = "<unknown>";
	if (script_context) {
		BmSieveFilter* filter = static_cast< BmSieveFilter*>( script_context);
		if (filter)
			filterName = filter->Name();
	}
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext)
		mailName = msgContext->mail->Name();
	BmString err("An error occurred during execution of a mail-filter.");
	err << "\nSieveFilter: " << filterName;
	err << "\nMail-ID: " << mailName;
	err << "\n\nError: " << (msg ? msg : "");
	BM_SHOWERR( err);
	return SIEVE_OK;
}



/********************************************************************************\
	BmGraphicalSieveFilter
\********************************************************************************/

const char * BM_MAILPART_OTHER  = "<Specify field>...";
const char * BM_MAILPART_BODY   = "<Message-Text>";

// the fields that can be selected directly:
BmString BM_MP_SEPARATOR = "---";
static const char* mailParts[] = {
	"Bcc",
	"Cc",
	"From",
	"Reply-To",
	"Subject",
	"To",
	BM_MP_SEPARATOR.String(),
	"List-Id",
	"Mailing-List",
	BM_MP_SEPARATOR.String(),
	"Status",
	"Account",
	BM_MP_SEPARATOR.String(),
	BM_MAILPART_OTHER,
//	BM_MAILPART_BODY,
	NULL
};

// address-fields:
static BmString BmAddressFieldNames = 
	"<Bcc><Resent-Bcc><Cc><List-Id><Resent-Cc><From><Resent-From><Reply-To>"
	"<Resent-Reply-To><Sender><Resent-Sender><To><Resent-To>";

// the address-parts:
const char* BM_ADDRPART_COMPLETE  = "(Complete)";
const char* BM_ADDRPART_ADDRESS   = "(Address)";
const char* BM_ADDRPART_DOMAIN    = "(Domain)";
const char* BM_ADDRPART_LOCALPART = "(Localpart)";

static const char* addrParts[] = {
	BM_ADDRPART_COMPLETE,
	BM_ADDRPART_ADDRESS,
	BM_ADDRPART_DOMAIN,
	BM_ADDRPART_LOCALPART,
	NULL
};
const char* BM_LONGEST_OP = "num-value is greater than or equal to";
static const char* operators[][3] = {
	{ "is", "", "is"},
	{ "is not", "not", "is"},
	{ "contains", "", "contains"},
	{ "does not contain", "not", "contains"},
	{ "starts with", "", "matches"},
	{ "does not start with", "not", "matches"},
	{ "ends with", "", "matches"},
	{ "does not end with", "not", "matches"},
	{ "matches regex", "", "regex"},
	{ "does not match regex", "not", "regex"},
	{ "value is less than", "", "value \"lt\""},
	{ "value is less than or equal to", "", "value \"le\""},
	{ "value is greater than or equal to", "", "value \"ge\""},
	{ "value is greater than", "", "value \"gt\""},
	{ "num-value is less than", "", "value \"lt\" :comparator \"i;ascii-numeric\""},
	{ "num-value is less than or equal to", "", "value \"le\" :comparator \"i;ascii-numeric\""},
	{ "num-value is greater than or equal to", "", "value \"ge\" :comparator \"i;ascii-numeric\""},
	{ "num-value is greater than", "", "value \"gt\" :comparator \"i;ascii-numeric\""},
	{ "count is less than", "", "count \"lt\" :comparator \"i;ascii-numeric\""},
	{ "count is less than or equal to", "", "count \"le\" :comparator \"i;ascii-numeric\""},
	{ "count is greater than or equal to", "", "count \"ge\" :comparator \"i;ascii-numeric\""},
	{ "count is greater than", "", "count \"gt\" :comparator \"i;ascii-numeric\""},
	{ NULL, NULL, NULL}
};

static const char* choices[] = {
	"all of",
	"any of",
	NULL
};

const char* const BmGraphicalSieveFilter::MSG_MATCH_COUNT = 		"bm:mc";
const char* const BmGraphicalSieveFilter::MSG_MATCH_ANYALL = 		"bm:aa";
const char* const BmGraphicalSieveFilter::MSG_MATCH_MAILPART = 	"bm:mp";
const char* const BmGraphicalSieveFilter::MSG_MATCH_ADDRPART = 	"bm:ap";
const char* const BmGraphicalSieveFilter::MSG_MATCH_FIELDNAME = 	"bm:fn";
const char* const BmGraphicalSieveFilter::MSG_MATCH_OPERATOR = 	"bm:op";
const char* const BmGraphicalSieveFilter::MSG_MATCH_VALUE = 		"bm:vl";
const char* const BmGraphicalSieveFilter::MSG_FILEINTO = 			"bm:fi";
const char* const BmGraphicalSieveFilter::MSG_FILEINTO_VALUE = 	"bm:fiv";
const char* const BmGraphicalSieveFilter::MSG_FILEINTO_ASK = 		"bm:fia";
const char* const BmGraphicalSieveFilter::MSG_DISCARD = 				"bm:ds";
const char* const BmGraphicalSieveFilter::MSG_SET_STATUS = 			"bm:sst";
const char* const BmGraphicalSieveFilter::MSG_SET_STATUS_VALUE = 	"bm:sstv";
const char* const BmGraphicalSieveFilter::MSG_SET_IDENTITY =		"bm:sid";
const char* const BmGraphicalSieveFilter::MSG_SET_IDENTITY_VALUE ="bm:sidv";
const char* const BmGraphicalSieveFilter::MSG_STOP_PROCESSING =	"bm:stop";
const char* const BmGraphicalSieveFilter::MSG_SET_SPAM_TOFU = 		"bm:ssp";
const char* const BmGraphicalSieveFilter::MSG_SET_SPAM_TOFU_VALUE = 	"bm:sspv";
const char* const BmGraphicalSieveFilter::MSG_SET_LIST_ID =			"bm:sli";
const char* const BmGraphicalSieveFilter::MSG_SET_LIST_ID_VALUE = "bm:sliv";

/*------------------------------------------------------------------------------*\
	BmGraphicalSieveFilter( name, archive)
		-	c'tor
		-	constructs a BmGraphicalSieveFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmGraphicalSieveFilter::BmGraphicalSieveFilter( const BmString& name, 
																const BMessage* archive) 
	:	inherited( name, archive)
	,	mMatchCount( 1)
	,	mMatchAnyAll( choices[0])
	,	mActionFileInto( false)
	,	mActionFileIntoAsk( false)
	,	mActionDiscard( false)
	,	mActionSetStatus( false)
	,	mActionSetIdentity( false)
	,	mActionSetSpamTofu( false)
	,	mActionSetListId( false)
	,	mStopProcessing( false)
{
	if (archive) {
		archive->FindInt16( MSG_MATCH_COUNT, &mMatchCount);
		if (mMatchCount <= 0) 
			mMatchCount = 1;
		mMatchAnyAll = archive->FindString( MSG_MATCH_ANYALL);
		if (!mMatchAnyAll.Length())
			mMatchAnyAll = choices[0];
		mActionFileInto = archive->FindBool( MSG_FILEINTO);
		mActionFileIntoValue = archive->FindString( MSG_FILEINTO_VALUE);
		archive->FindBool( MSG_FILEINTO_ASK, &mActionFileIntoAsk);
		mActionDiscard = archive->FindBool( MSG_DISCARD);
		archive->FindBool( MSG_SET_STATUS, &mActionSetStatus);
		mActionSetStatusValue = archive->FindString( MSG_SET_STATUS_VALUE);
		archive->FindBool( MSG_SET_IDENTITY, &mActionSetIdentity);
		mActionSetIdentityValue = archive->FindString( MSG_SET_IDENTITY_VALUE);
		archive->FindBool( MSG_STOP_PROCESSING, &mStopProcessing);
		archive->FindBool( MSG_SET_SPAM_TOFU, &mActionSetSpamTofu);
		mActionSetSpamTofuValue = archive->FindString( MSG_SET_SPAM_TOFU_VALUE);
		archive->FindBool( MSG_SET_LIST_ID, &mActionSetListId);
		mActionSetListIdValue = archive->FindString( MSG_SET_LIST_ID_VALUE);

		for( int i=0; i<mMatchCount; ++i) {
			mMatchMailPart[i] = archive->FindString( MSG_MATCH_MAILPART, i);
			mMatchAddrPart[i] = archive->FindString( MSG_MATCH_ADDRPART, i);
			mMatchFieldName[i] = archive->FindString( MSG_MATCH_FIELDNAME, i);
			mMatchOperator[i] = archive->FindString( MSG_MATCH_OPERATOR, i);
			mMatchValue[i] = archive->FindString( MSG_MATCH_VALUE, i);
		}
		for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
			if (!mMatchAddrPart[i].Length())
				mMatchAddrPart[i] = BM_ADDRPART_COMPLETE;
		}
		BuildScriptFromStrings();
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmGraphicalSieveFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmGraphicalSieveFilter::Archive( BMessage* archive, bool) const {
	status_t ret = (archive->AddInt16( MSG_VERSION, nArchiveVersion)
		|| archive->AddInt16( MSG_MATCH_COUNT, mMatchCount)
		||	archive->AddString( MSG_MATCH_ANYALL, mMatchAnyAll.String())
		|| archive->AddBool( MSG_FILEINTO, mActionFileInto)
		||	archive->AddString( MSG_FILEINTO_VALUE, mActionFileIntoValue.String())
		|| archive->AddBool( MSG_FILEINTO_ASK, mActionFileIntoAsk)
		|| archive->AddBool( MSG_DISCARD, mActionDiscard)
		|| archive->AddBool( MSG_SET_STATUS, mActionSetStatus)
		||	archive->AddString( MSG_SET_STATUS_VALUE, 
									  mActionSetStatusValue.String())
		|| archive->AddBool( MSG_SET_IDENTITY, mActionSetIdentity)
		||	archive->AddString( MSG_SET_IDENTITY_VALUE, 
									  mActionSetIdentityValue.String())
		|| archive->AddBool( MSG_SET_SPAM_TOFU, mActionSetSpamTofu)
		||	archive->AddString( MSG_SET_SPAM_TOFU_VALUE, 
									  mActionSetSpamTofuValue.String())
		|| archive->AddBool( MSG_STOP_PROCESSING, mStopProcessing)
		|| archive->AddBool( MSG_SET_LIST_ID, mActionSetListId)
		||	archive->AddString( MSG_SET_LIST_ID_VALUE, 
									  mActionSetListIdValue.String()));
	for( int i=0; ret==B_OK && i<mMatchCount; ++i) {
		ret = (archive->AddString( MSG_MATCH_MAILPART, 
											mMatchMailPart[i].String())
			 || archive->AddString( MSG_MATCH_ADDRPART, 
			 								mMatchAddrPart[i].String())
			 || archive->AddString( MSG_MATCH_FIELDNAME, 
			 								mMatchFieldName[i].String())
			 || archive->AddString( MSG_MATCH_OPERATOR, 
			 								mMatchOperator[i].String())
			 || archive->AddString( MSG_MATCH_VALUE, 
			 								mMatchValue[i].String()));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmGraphicalSieveFilter::SetupFromMailData( const BmString& subject, 
																const BmString& from, 
																const BmString& to) {
	mMatchCount = 0;
	if (from.Length()) {
		mMatchMailPart[mMatchCount] = "From";
		mMatchOperator[mMatchCount] = "is";
		mMatchValue[mMatchCount] = from;
		mMatchCount++;
	}
	if (subject.Length()) {
		mMatchMailPart[mMatchCount] = "To";
		mMatchOperator[mMatchCount] = "is";
		mMatchValue[mMatchCount] = to;
		mMatchCount++;
	}
	if (subject.Length()) {
		mMatchMailPart[mMatchCount] = "Subject";
		mMatchOperator[mMatchCount] = "contains";
		mMatchValue[mMatchCount] = subject;
		mMatchCount++;
	}
}

/*------------------------------------------------------------------------------*\
	IsAddrField()
		-	returns if the given field-name is an address-field.
\*------------------------------------------------------------------------------*/
bool BmGraphicalSieveFilter::IsAddrField( const BmString& addrField) {
	return BmAddressFieldNames.IFindFirst( BmString("<")<<addrField<<">") 
				!= B_ERROR;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmGraphicalSieveFilter::SanityCheck( BmString& complaint, 
														BmString& fieldName) {
	if (mActionFileInto && !mActionFileIntoValue.Length()) {
		complaint = "Please select a folder to file the matching mails into";
		fieldName = "fileIntoValue";
		return false;
	} else if (mActionSetStatus && !mActionSetStatusValue.Length()) {
		complaint 
			= "Please select the status that matching mails should be set to";
		fieldName = "setStatusValue";
		return false;
	} else if (mActionSetIdentity && !mActionSetIdentityValue.Length()) {
		complaint = "Please select the identity that matching mails should "
						"be associated with";
		fieldName = "setIdentityValue";
		return false;
	} else if (mActionSetSpamTofu && !mActionSetSpamTofuValue.Length()) {
		complaint 
			= "Please select what matching mails should be learned as (SPAM or TOFU)";
		fieldName = "setSpamTofuValue";
		return false;
	}
	int validMatchCount=mMatchCount;
	for( int i=0; i<mMatchCount; ++i) {
		if (!mMatchMailPart[i].Length() && !mMatchOperator[i].Length()) {
			validMatchCount--;
			continue;
		}
		if (!mMatchMailPart[i].Length()) {
			complaint = "Please select the mail-part that is to be filtered";
			fieldName = BmString("mailPart:") << i;
			return false;
		} else if (mMatchMailPart[i] == BM_MAILPART_OTHER 
		&& !mMatchFieldName[i].Length()) {
			complaint = "Please enter the name of the header-field that is to "
							"be filtered";
			fieldName = BmString("fieldName") << i;
			return false;
		} else if (!mMatchOperator[i].Length()) {
			complaint = "Please select the filter-operator";
			fieldName = BmString("operator") << i;
			return false;
		} else if (!mMatchValue[i].Length()) {
			complaint = "Please select the value that is to be used for the "
							"comparison";
			fieldName = BmString("value") << i;
			return false;
		}
	}
	if (!validMatchCount && mActionDiscard) {
		complaint = "You have created a filter that will throw away each and "
						"every mail!\n\nMy advice: don't do that >:o)";
		return false;
	}
	BuildScriptFromStrings();
	return inherited::SanityCheck( complaint, fieldName);
}

/*------------------------------------------------------------------------------*\
	BuildScriptFromStrings()
		-	
\*------------------------------------------------------------------------------*/
bool BmGraphicalSieveFilter::BuildScriptFromStrings() {
	vector<BmString> valueVect;
	BmString script("# generated by Beam's GUI-editor, please do not edit!\n");
	bool needRegex = false;
	bool needRelational = false;
	bool needAsciiNumeric = false;
	for( int i=0; i<mMatchCount; ++i) {
		if (mMatchOperator[i].IFindFirst("regex") != B_ERROR) {
			needRegex = true;
		}
		if (mMatchOperator[i].IFindFirst("less") != B_ERROR
		|| mMatchOperator[i].IFindFirst("greater") != B_ERROR) {
			needRelational = true;
		}
		if (mMatchOperator[i].IFindFirst("num-") != B_ERROR
		|| mMatchOperator[i].IFindFirst("count") != B_ERROR) {
			needAsciiNumeric = true;
		}
	}
	if (mActionFileInto)
		script << "require \"fileinto\";\n";
	if (mActionSetIdentity || mActionSetStatus || mActionSetSpamTofu 
	|| mStopProcessing || mActionSetListId)
		script << "require \"notify\";\n";
	if (needRegex)
		script << "require \"regex\";\n";
	if (needRelational)
		script << "require \"relational\";\n";
	if (needAsciiNumeric)
		script << "require \"comparator-i;ascii-numeric\";\n";
	BmString matchPart;
	for( int i=0; i<mMatchCount; ++i) {
		if (!mMatchMailPart[i].Length() && !mMatchOperator[i].Length())
			continue;
		BmString neg;
		BmString op;
		for( int o=0; operators[o][0]; ++o) {
			if (mMatchOperator[i] == operators[o][0]) {
				neg = operators[o][1];
				op = operators[o][2];
				break;
			}
		}
		bool otherField = mMatchMailPart[i] == BM_MAILPART_OTHER;
		bool needsAddressTest = IsAddrField( mMatchMailPart[i]) 
										&& mMatchAddrPart[i] != BM_ADDRPART_COMPLETE;
		BmString line = (neg.Length() ? (neg + " ") : BmString(""));
		if (needsAddressTest) {
			// use address test:
			if (mMatchAddrPart[i] == BM_ADDRPART_ADDRESS)
				line << "address :all ";
			else if (mMatchAddrPart[i] == BM_ADDRPART_DOMAIN)
				line << "address :domain ";
			else if (mMatchAddrPart[i] == BM_ADDRPART_LOCALPART)
				line << "address :localpart ";
		} else {
			// use header test:
			line << "header ";
		}
		// add operator and field-name:
		line	<< ":" << op << " "
				<< "\"" << (otherField 
									? mMatchFieldName[i] 
									: mMatchMailPart[i]) 
						  << "\" ";
		// add list of values:
		line << "[";
		split( "\n", mMatchValue[i], valueVect);
		for( uint32 v=0; v<valueVect.size(); ++v) {
			BmString val = valueVect[v];
			if (!val.Length())
				continue;						// ignore empty lines in value-vector
			if (mMatchOperator[i].IFindFirst("start") != B_ERROR)
				val.Append("*");
			else if (mMatchOperator[i].IFindFirst("end") != B_ERROR)
				val.Prepend("*");
			if ( v > 0)
				line << ",";
			line << "\"" <<  val << "\"";
		}
		line << "],\n";
		matchPart << line;
	}
	if (matchPart.Length()) {
		if (mMatchAnyAll == choices[0]) {
			// all of
			script << "if allof (" << matchPart << "true) {\n";
		} else {
			// any of
			script << "if anyof (" << matchPart << "false) {\n";
		}
	}
	if (mActionFileInto)
		script << "   fileinto \"" << mActionFileIntoValue << "\";\n";
	if (mActionSetIdentity)
		script << "   notify :method \"" << BmNotifySetIdentity 
				 << "\" :options \"" << mActionSetIdentityValue << "\";\n";
	if (mActionSetStatus)
		script << "   notify :method \"" << BmNotifySetStatus 
				 << "\" :options \"" << mActionSetStatusValue << "\";\n";
	if (mActionSetSpamTofu)
		script << "   notify :method \"" << BmNotifySetSpamTofu 
				 << "\" :options \"" << mActionSetSpamTofuValue << "\";\n";
	if (mStopProcessing)
		script << "   notify :method \"" << BmNotifyStopProcessing << "\";\n";
	if (mActionSetListId)
		script << "   notify :method \"" << BmNotifySetListId
				 << "\" :options \"" << mActionSetListIdValue << "\";\n";
	if (mActionDiscard)
		script << "   discard;\n";
	if (matchPart.Length()) {
		script << "}";
	}
	Content( script);
	return true;
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmGraphicalSieveFilter::ForeignKeyChanged( const BmString& key, 
																const BmString& oldVal, 
																const BmString& newVal) {
	if (key == BmFilterAddon::FK_FOLDER) {
		if (mActionFileIntoValue == oldVal)
			mActionFileIntoValue = newVal;
	} else if (key == BmFilterAddon::FK_IDENTITY) {
		if (mActionSetIdentityValue == oldVal)
			mActionSetIdentityValue = newVal;
	}
}


/*------------------------------------------------------------------------------*\
	InstantiateFilter()
		-	
\*------------------------------------------------------------------------------*/
extern "C"
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind) {
	if (!kind.ICompare( FILTER_SIEVE_SCRIPT))
		return new BmSieveFilter( name, archive);
	else if (!kind.ICompare( FILTER_SIEVE))
		return new BmGraphicalSieveFilter( name, archive);
	else
		return NULL;
}



/********************************************************************************\
	BmFilterScrollView
\********************************************************************************/

class BmFilterGroup : public MGroup, public BView
{
	public:
		BmFilterGroup()
		: BView( BRect( 0, 0, 99, 99), "filterGroup", B_FOLLOW_ALL, 
					B_WILL_DRAW) 
		{
		}
		virtual minimax layoutprefs();
		virtual BRect layout(BRect rect);
	private:
};

minimax BmFilterGroup::layoutprefs() {
	mpm = ct_mpm;
	mpm.mini.y = 0;
	int32 count = CountChildren();
	for( int32 i=0; i<count; ++i) {
		BView* kid = ChildAt( i);
		MView* mvkid = dynamic_cast<MView*>( kid);
		if (!mvkid)
			continue;
		minimax kid_mpm = mvkid->layoutprefs();
		mpm.mini.y += kid_mpm.mini.y;
	}
	mpm.maxi.y = mpm.mini.y;
	return mpm;
}

BRect BmFilterGroup::layout(BRect rect) {
	MoveTo( rect.left, rect.top);
	ResizeTo( rect.Width(), rect.Height());
	float yPos = 0.0;
	int32 count = CountChildren();
	for( int32 i=0; i<count; ++i) {
		BView* kid = ChildAt( i);
		MView* mvkid = dynamic_cast<MView*>( kid);
		if (!mvkid)
			continue;
		minimax kid_mpm = mvkid->mpm;
		BRect kidRect = mvkid->layout( BRect( rect.left, yPos, 
														  rect.right, yPos+kid_mpm.mini.y));
		yPos += kidRect.Height();
	}
	return rect;
}

class BmFilterScrollView: public MView, public BScrollView
{
	public:
		BmFilterScrollView(MView *target, border_style border=B_FANCY_BORDER)
		:	BScrollView( "scroller", dynamic_cast<BView*>(target), 
							 B_FOLLOW_NONE, B_WILL_DRAW|B_FRAME_EVENTS,
							false, true, border)
		,	kid( target)
		{
			vscr = ScrollBar( B_VERTICAL);
		}
		virtual minimax layoutprefs();
		virtual BRect layout(BRect rect);
	private:
		MView* kid;
		BScrollBar* vscr;
};

minimax BmFilterScrollView::layoutprefs() {
	kid->layoutprefs();
	return mpm = ct_mpm;
}

BRect BmFilterScrollView::layout(BRect rect) {
	BView* bv = dynamic_cast<BView*>( kid);
	bv->TargetedByScrollView( this);
	MoveTo( rect.left, rect.top);
	ResizeTo( rect.Width(), rect.Height());
	float inset = 2.0f;
	float viewWidth = rect.Width()-B_V_SCROLL_BAR_WIDTH-inset*2.0f;
	float viewHeight = rect.Height()-inset*2;
	float dataHeight = kid->mpm.mini.y;
	if (dataHeight < viewHeight)
		dataHeight = viewHeight;
	BRect kidRect = BRect( inset, inset, viewWidth-1, viewHeight-1);
	float value = vscr->Value();
	// reset scrolling state (otherwise we have a scrolling nightmare, but why?):
	vscr->SetValue( 0);
	kid->layout( kidRect);
	// set scrolling-position back to as it was before layout:
	vscr->SetValue( value);
	// update range and proportion:
	vscr->SetRange( 0, dataHeight - viewHeight);
	vscr->SetProportion( viewHeight / dataHeight);
	return rect;
}



/********************************************************************************\
	BmSieveFilterPrefs
\********************************************************************************/

const char* const BmSieveFilterPrefs::MSG_IDX = 		"bm:idx";

/*------------------------------------------------------------------------------*\
	BmSieveFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveFilterPrefs::BmSieveFilterPrefs( minimax minmax)
	:	inherited( minmax.mini.x, minmax.mini.y, minmax.maxi.x, minmax.maxi.y)
	,	mCurrFilterAddon( NULL)
{
	mFilterGroup = new BmFilterGroup();

	BmMenuControllerBase* spamTofuMenu;

	BView* bv = dynamic_cast<BView*>( mFilterGroup);
	if (bv) {
		bv->SetResizingMode( B_FOLLOW_ALL);
		bv->SetFlags( B_WILL_DRAW|B_FRAME_EVENTS);
		bv->ResizeTo( 200,40);
	}
	
	Space* rightSpace;

	VGroup* vgroup = 
		new VGroup( 
			new MBorder( M_LABELED_BORDER, 5, (char*)"Selection criteria",
				new VGroup(
					new HGroup(
						mAnyAllControl = new BmMenuControl( "Match if ", 
																		new BPopUpMenu( "")),
						new MStringView( (char*)" the following is true"),
						new Space( minimax(1,1,1e5,1)),
						mAddButton = new MButton( 
							"Add line", 
							new BMessage( BM_ADD_FILTER_LINE), 
							this, minimax(-1,-1,-1,-1)
						),
						mRemoveButton = new MButton( 
							"Remove marked lines", 
							new BMessage( BM_REMOVE_FILTER_LINE), 
							this, minimax(-1,-1,-1,-1)
						),
						0
					),
					mFilterScrollView = new BmFilterScrollView( 
						mFilterGroup,
						B_FANCY_BORDER
					),
					0
				)
			),
			new MBorder( M_LABELED_BORDER, 5, (char*)"Filter actions",
				mActionGroup = new VGroup(
					new HGroup(
						new VGroup(
							new HGroup(
								mFileIntoControl = new BmCheckControl( 
									"File into", 
									new BMessage(BM_FILEINTO_CHANGED), 
									this
								),
								mFileIntoValueControl = new BmMenuControl( 
									NULL,
									new BmMenuControllerBase( 
										"", this, 
										new BMessage( BM_FILEINTO_SELECTED), 
										&BmGuiRosterBase::RebuildFolderMenu
									)
								),
								mFileIntoAskControl = new BmCheckControl( 
									"Ask", 
									new BMessage(BM_FILEINTO_ASK_CHANGED), 
									this
								),
								0
							),
							new HGroup(
								mSetStatusControl = new BmCheckControl( 
									"Set status", 
									new BMessage(BM_SET_STATUS_CHANGED), 
									this
								),
								mSetStatusValueControl = new BmMenuControl( 
									NULL, 
									new BmMenuControllerBase( 
										"", this, 
										new BMessage( BM_SET_STATUS_SELECTED), 
										&BmGuiRosterBase::RebuildStatusMenu, 
										BM_MC_LABEL_FROM_MARKED
									)
								),
								0
							),
							new HGroup(
								mSetIdentityControl = new BmCheckControl( 
									"Set identity", 
									new BMessage(BM_SET_IDENTITY_CHANGED), 
									this
								),
								mSetIdentityValueControl = new BmMenuControl( 
									NULL, 
									new BmMenuControllerBase( 
										"", this, 
										new BMessage( BM_SET_IDENTITY_SELECTED), 
										&BmGuiRosterBase::RebuildIdentityMenu, 
										BM_MC_LABEL_FROM_MARKED
									)
								),
								0
							),
							new HGroup(
								mSetListIdControl = new BmCheckControl( 
									"Set List-Id", 
									new BMessage(BM_SET_LIST_ID_CHANGED),
									this
								),
								mSetListIdValueControl = new BmTextControl(
									(const char *)NULL
								),
								0
							),
							0
						),
						new Space( minimax( 20,0,20,1e5)),
						new VGroup(
							minimax(-1,-1,1e5,1e5,0.7f),
							new HGroup(
								mSetSpamTofuControl = new BmCheckControl( 
									"Learn as", 
									new BMessage(BM_SET_SPAM_TOFU_CHANGED), 
									this
								),
								mSetSpamTofuValueControl = new BmMenuControl( 
									NULL, 
									spamTofuMenu = new BmMenuControllerBase( 
										"", this, 
										new BMessage( BM_SET_SPAM_TOFU_SELECTED), 
										NULL, BM_MC_LABEL_FROM_MARKED
									)
								),
								0
							),
							new HGroup(
								mDiscardControl = new BmCheckControl( 
									"Move to Trash", 
									new BMessage(BM_DISCARD_CHANGED), 
									this
								),
								new Space(),
								0
							),
							new HGroup(
								mStopProcessingControl = new BmCheckControl( 
									"Stop processing if filter matches", 
									new BMessage(BM_STOP_PROCESSING_CHANGED), 
									this
								),
								new Space(),
								0
							),
							rightSpace = new Space(),
							0
						),
						0
					),
					0
				)
			),
			0
		);
	
	float width, maxWidth, height; 
	mFileIntoControl->GetPreferredSize( &maxWidth, &height);
	mFileIntoAskControl->ct_mpm = minimax(-1,-1,-1,-1);

	mSetStatusControl->GetPreferredSize( &width, &height);
	if (width>maxWidth)
		maxWidth = width;
	mSetIdentityControl->GetPreferredSize( &width, &height);
	if (width>maxWidth)
		maxWidth = width;
	mSetListIdControl->GetPreferredSize( &width, &height);
	if (width>maxWidth)
		maxWidth = width;
	mFileIntoControl->ct_mpm.maxi.x = mFileIntoControl->ct_mpm.mini.x
	= mSetStatusControl->ct_mpm.maxi.x = mSetStatusControl->ct_mpm.mini.x 
	= mSetIdentityControl->ct_mpm.maxi.x = mSetIdentityControl->ct_mpm.mini.x 
	= mSetListIdControl->ct_mpm.maxi.x = mSetListIdControl->ct_mpm.mini.x 
	= maxWidth+5;

	rightSpace->ct_mpm.maxi.y = rightSpace->ct_mpm.mini.y 
		= mSetListIdValueControl->ct_mpm.mini.y;

	mAnyAllControl->SetDivider( 70);
	mAnyAllControl->ct_mpm.maxi.x = 120;
	
	mFilterScrollView->ct_mpm = minimax( 0,0, 1e5,1e5);

	for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		mFilterLine[i] = 
			new HGroup(
				mMailPartControl[i] = new BmMenuControl( 
					NULL, new BPopUpMenu( ""),
					0, 0, BM_MAILPART_OTHER
				),
				new VGroup(
					mFieldSpecLayer[i] = new LayeredGroup(
						new Space(),
						mAddrPartControl[i] = new BmMenuControl( 
							NULL, new BPopUpMenu( ""), 0, 0, 
							BM_ADDRPART_LOCALPART
						),
						mFieldNameControl[i] = new BmTextControl( (const char *)NULL),
						NULL
					),
					0
				),
				mOperatorControl[i] = new BmMenuControl( 
					NULL, new BPopUpMenu( ""),
					0, 0, BM_LONGEST_OP
				),
				mValueControl[i] = new BmMultiLineTextControl( 
					NULL, false, 1, 
					0, true
				),
				new VGroup(
					mMarkControl[i] = new BmCheckControl( NULL, NULL,	this),
					new Space(),
					0
				),
				0
			);
		mMailPartControl[i]->ct_mpm.maxi.y = 1E5;
		mFieldSpecLayer[i]->ct_mpm.maxi.y = 1E5;
		mAddrPartControl[i]->ct_mpm.maxi.y = 1E5;
		mOperatorControl[i]->ct_mpm.maxi.y = 1E5;
		mFieldNameControl[i]->ct_mpm = minimax(80,-1,80,1E5);
		mValueControl[i]->SetTabAllowed( false);
		mValueControl[i]->ct_mpm = minimax(140,-1,1E5,1E5,5.0f);
		mMarkControl[i]->ct_mpm.weight = 0.1f;
		mFilterGroup->AddChild( mFilterLine[i]);
	}		
	mFilterGroup->AddChild( mSpaceAtBottom = new Space());
	mVisibleLines = BM_MAX_MATCH_COUNT;

	AddChild( dynamic_cast<BView*>( vgroup));
}

/*------------------------------------------------------------------------------*\
	~BmSieveFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveFilterPrefs::~BmSieveFilterPrefs() {
	TheBubbleHelper->SetHelp( mAnyAllControl, NULL);
	for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		TheBubbleHelper->SetHelp( mMailPartControl[i], NULL);
		TheBubbleHelper->SetHelp( mAddrPartControl[i], NULL);
		TheBubbleHelper->SetHelp( mFieldNameControl[i], NULL);
		TheBubbleHelper->SetHelp( mOperatorControl[i], NULL);
		TheBubbleHelper->SetHelp( mValueControl[i], NULL);
		TheBubbleHelper->SetHelp( mMarkControl[i], NULL);
	}
	TheBubbleHelper->SetHelp( mFileIntoControl, NULL);
	TheBubbleHelper->SetHelp( mFileIntoValueControl, NULL);
	TheBubbleHelper->SetHelp( mFileIntoAskControl, NULL);
	TheBubbleHelper->SetHelp( mSetStatusControl, NULL);
	TheBubbleHelper->SetHelp( mSetStatusValueControl, NULL);
	TheBubbleHelper->SetHelp( mSetIdentityControl, NULL);
	TheBubbleHelper->SetHelp( mSetIdentityValueControl, NULL);
	TheBubbleHelper->SetHelp( mDiscardControl, NULL);
	TheBubbleHelper->SetHelp( mStopProcessingControl, NULL);
	TheBubbleHelper->SetHelp( mSetSpamTofuControl, NULL);
	TheBubbleHelper->SetHelp( mSetSpamTofuValueControl, NULL);
	TheBubbleHelper->SetHelp( mSetListIdControl, NULL);
	TheBubbleHelper->SetHelp( mSetListIdValueControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::Initialize() {
	TheBubbleHelper->SetHelp( 
		mAnyAllControl, 
		"Here you can select if a mail matches the filter if \n"
		"any of the criteria matches (OR-operation) or if\n"
		"all if the criteria match (AND-operation)."
	);
	for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		TheBubbleHelper->SetHelp( 
			mMailPartControl[i], 
			"Please select the part of the mail (a header-field)\n"
			"that shall be matched."
		);
		TheBubbleHelper->SetHelp( 
			mAddrPartControl[i], 
			"This field is only shown if the selected header is an address-field.\n"
			"Here you can choose, which part of the address-field\n"
			"the filter should look at.\n"
			"Assuming a FROM header of 'Elliot Smith <esmith@test.org>',\n"
			"the choices are:\n"
			"   (complete): the complete address-field will be used,\n"
			"               i.e. 'Elliot Smith <esmith@test.org>'\n"
			"   (address): just the address-part (addr-spec) will be used,\n"
			"              i.e. 'esmith@test.org'\n"
			"   (domain): just the domain-part will be used, i.e. 'test.org'\n"
			"   (localpart): just the localpart (mailbox-name) will be used,\n"
			"                i.e. 'esmith'"
		);
		TheBubbleHelper->SetHelp( 
			mFieldNameControl[i], 
			"If you have chosen to specify the fieldname yourself,\n"
			"you can do so in this textfield."
		);
		TheBubbleHelper->SetHelp( 
			mOperatorControl[i], 
			"Here you can select what kind of match shall be done."
		);
		TheBubbleHelper->SetHelp( 
			mValueControl[i], 
			"Here you can enter the one or more values that\n"
			"the selected field will be matched against.\n"
			"If any of the given values matches, the filter-line\n"
			"is satisfied (values ar ORed)."
		);
		TheBubbleHelper->SetHelp( 
			mMarkControl[i], 
			"Check to mark this line."
		);
	}
	TheBubbleHelper->SetHelp( 
		mFileIntoControl, 
		"Check this to move matching mails into a specific folder."
	);
	TheBubbleHelper->SetHelp( 
		mFileIntoValueControl, 
		"Here you can select the folder to file mails into."
	);
	TheBubbleHelper->SetHelp( 
		mFileIntoAskControl, 
		"Check this if you want to be asked for acknowledgement\n"
		"before any outbound mail matching the filter is filed."
	);
	TheBubbleHelper->SetHelp( 
		mSetStatusControl, 
		"Check this to modify the status of matching mails."
	);
	TheBubbleHelper->SetHelp( 
		mSetStatusValueControl, 
		"Please select the new status here."
	);
	TheBubbleHelper->SetHelp( 
		mSetIdentityControl, 
		"Check this if you want to associate matching mails\n"
		"with a given identity (e.g. useful for mailing-lists)."
	);
	TheBubbleHelper->SetHelp( 
		mSetIdentityValueControl, 
		"Please select the associated identity here."
	);
	TheBubbleHelper->SetHelp( 
		mSetSpamTofuControl, 
		"Check this to learn matching mails as SPAM or TOFU."
	);
	TheBubbleHelper->SetHelp( 
		mSetSpamTofuValueControl, 
		"Please select the new classification here."
	);
	TheBubbleHelper->SetHelp( 
		mSetListIdControl, 
		"Check this if you want to associate matching mails\n"
		"with a specific mailing list."
	);
	TheBubbleHelper->SetHelp( 
		mSetIdentityValueControl, 
		"Please enter the List-Id here."
	);
	TheBubbleHelper->SetHelp( 
		mDiscardControl, 
		"Check this to directly move matching mails to trash."
	);
	TheBubbleHelper->SetHelp( 
		mStopProcessingControl, 
		"Check this if you want to stop filter-processing\n"
		"if this filter matches. Any subsequent filters\n"
		"of the active filter-chain will be ignored."
	);

	for( const char** itemP = choices; *itemP; ++itemP) {
		BMenuItem* item = new BMenuItem( *itemP, 
													new BMessage( BM_ANY_ALL_SELECTED));
		item->SetTarget( this);
		mAnyAllControl->Menu()->AddItem( item);
	}
	for( int16 i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		for( const char** itemP = mailParts; *itemP; ++itemP) {
			if (BM_MP_SEPARATOR == *itemP)
				mMailPartControl[i]->Menu()->AddSeparatorItem();
			else {
				BMessage* msg = new BMessage( BM_MAILPART_SELECTED);
				msg->AddInt16( MSG_IDX, i);
				BMenuItem* item = new BMenuItem( *itemP, msg);
				item->SetTarget( this);
				mMailPartControl[i]->Menu()->AddItem( item);
			}
		}
		for( const char** itemP = addrParts; *itemP; ++itemP) {
			BMessage* msg = new BMessage( BM_ADDRPART_SELECTED);
			msg->AddInt16( MSG_IDX, i);
			BMenuItem* item = new BMenuItem( *itemP, msg);
			item->SetTarget( this);
			mAddrPartControl[i]->Menu()->AddItem( item);
		}
		for( int o=0; operators[o][0]; ++o) {
			BMessage* msg = new BMessage( BM_OPERATOR_SELECTED);
			msg->AddInt16( MSG_IDX, i);
			BMenuItem* item = new BMenuItem( operators[o][0], msg);
			item->SetTarget( this);
			mOperatorControl[i]->Menu()->AddItem( item);
		}
		mFieldNameControl[i]->SetTarget( this);
		mValueControl[i]->SetTarget( this);
	}		

	BMenuItem* item 
		= new BMenuItem( "SPAM", new BMessage(BM_SET_SPAM_TOFU_SELECTED));
	item->SetTarget(this);
	mSetSpamTofuValueControl->Menu()->AddItem( item);
	item = new BMenuItem( "TOFU", new BMessage( BM_SET_SPAM_TOFU_SELECTED));
	item->SetTarget(this);
	mSetSpamTofuValueControl->Menu()->AddItem( item);
	mSetListIdValueControl->SetTarget( this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::Activate() {
}

/*------------------------------------------------------------------------------*\
	AddFilterLine()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::AddFilterLine() {
	if (mVisibleLines < BM_MAX_MATCH_COUNT) {
		mFilterGroup->AddChild( mFilterLine[mVisibleLines], mSpaceAtBottom);
		AdjustSizeOfValueControl( mValueControl[mVisibleLines++]);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveMarkedFilterLines()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::RemoveMarkedFilterLines() {
	int32 currIdx = 0;
	for( int32 i=0; i<mVisibleLines; ++i) {
		if (!mMarkControl[i]->Value()) {
			if (i != currIdx) {
				mMailPartControl[currIdx]->MenuItem()->SetLabel(
					mMailPartControl[i]->MenuItem()->Label()
				);
				mMailPartControl[currIdx]->MarkItem(
					mMailPartControl[i]->MenuItem()->Label()
				);
				mCurrFilterAddon->mMatchMailPart[currIdx] 
					= mCurrFilterAddon->mMatchMailPart[i];
				mFieldNameControl[currIdx]->SetText(
					mFieldNameControl[i]->Text()
				);
				mCurrFilterAddon->mMatchFieldName[currIdx] 
					= mCurrFilterAddon->mMatchFieldName[i];
				mOperatorControl[currIdx]->MarkItem(
					mOperatorControl[i]->MenuItem()->Label()
				);
				mOperatorControl[currIdx]->MenuItem()->SetLabel(
					mOperatorControl[i]->MenuItem()->Label()
				);
				mCurrFilterAddon->mMatchOperator[currIdx] 
					= mCurrFilterAddon->mMatchOperator[i];
				mValueControl[currIdx]->SetText(
					mValueControl[i]->Text()
				);
				mCurrFilterAddon->mMatchValue[currIdx] 
					= mCurrFilterAddon->mMatchValue[i];
			}
			currIdx++;
		} else
			mMarkControl[i]->SetValue( false);
	}
	while( mVisibleLines > currIdx) {
		--mVisibleLines;
		mMailPartControl[mVisibleLines]->ClearMark();
		mMailPartControl[mVisibleLines]->MenuItem()->SetLabel("");
		mCurrFilterAddon->mMatchMailPart[mVisibleLines] = "";
		mFieldNameControl[mVisibleLines]->SetText("");
		mCurrFilterAddon->mMatchFieldName[mVisibleLines] = "";
		mOperatorControl[mVisibleLines]->ClearMark();
		mOperatorControl[mVisibleLines]->MenuItem()->SetLabel("");
		mCurrFilterAddon->mMatchOperator[mVisibleLines] = "";
		mValueControl[mVisibleLines]->SetText("");
		mCurrFilterAddon->mMatchValue[mVisibleLines] = "";
		mFilterLine[mVisibleLines]->RemoveSelf();
	}
}

/*------------------------------------------------------------------------------*\
	Kind()
		-	
\*------------------------------------------------------------------------------*/
const char* BmSieveFilterPrefs::Kind() const { 
	return FILTER_SIEVE;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::AdjustScrollView() {
	// adjust size & ranges of scrollview:
	mFilterScrollView->layoutprefs();
	layoutprefs();
	layout( Frame());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::AdjustSizeOfValueControl( 
															BmMultiLineTextControl* control) {
	// adjust size of multiline-textview to accommodate its content:
	BFont font;
	control->GetFont( &font);
	font_height fh;
	font.GetHeight( &fh);
	int lineHeight = (int)(ceil(fh.ascent) + ceil(fh.descent) + ceil(fh.leading));
	int lineCount = 1;
	BmString txt( control->Text());
	for( int32 pos = 0; (pos = txt.FindFirst( "\n", pos)) != B_ERROR; ++pos)
		++lineCount;
	float height = float(lineHeight*lineCount+13);
	control->ct_mpm.mini.y = control->ct_mpm.maxi.y
	= control->mpm.mini.y = control->mpm.maxi.y = height;
	control->SetTextMargin( 2);
	control->TextView()->ScrollToOffset( 0);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_ADD_FILTER_LINE: {
			if (mVisibleLines<BM_MAX_MATCH_COUNT && mCurrFilterAddon) {
				AddFilterLine();
				mCurrFilterAddon->mMatchCount++;
				AdjustScrollView();
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_REMOVE_FILTER_LINE: {
			if (mVisibleLines>1 && mCurrFilterAddon) {
				RemoveMarkedFilterLines();
				mCurrFilterAddon->mMatchCount = int16(mVisibleLines);
				AdjustScrollView();
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_ANY_ALL_SELECTED: {
			if (mCurrFilterAddon) {
				BMenuItem* item = mAnyAllControl->Menu()->FindMarked();
				if (item)
					mCurrFilterAddon->mMatchAnyAll = item->Label();
				else
					mCurrFilterAddon->mMatchAnyAll = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_TEXTFIELD_MODIFIED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if (source == mSetListIdValueControl) {
					mCurrFilterAddon->mActionSetListIdValue
						= mSetListIdValueControl->Text();
					UpdateState();
					PropagateChange();
				} else {
					for( int idx=0; idx<mVisibleLines; ++idx) {
						if (source == mFieldNameControl[idx]) {
							mCurrFilterAddon->mMatchFieldName[idx] 
								= mFieldNameControl[idx]->Text();
							UpdateState();
							PropagateChange();
							break;
						}
					}
				}
			}
			break;
		}
		case BM_MULTILINE_TEXTFIELD_MODIFIED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmMultiLineTextControl* source 
					= dynamic_cast<BmMultiLineTextControl*>( srcView);
				for( int idx=0; idx<mVisibleLines; ++idx) {
					if (source == mValueControl[idx]) {
						AdjustSizeOfValueControl( source);
						mCurrFilterAddon->mMatchValue[idx] 
							= mValueControl[idx]->Text();
						AdjustScrollView();
						UpdateState();
						PropagateChange();
						break;
					}
				}
			}
			break;
		}
		case BM_MAILPART_SELECTED: {
			if (mCurrFilterAddon) {
				int16 idx = msg->FindInt16( MSG_IDX);
				if (idx>=0 && idx<mVisibleLines) {
					BMenuItem* item = mMailPartControl[idx]->Menu()->FindMarked();
					if (item)
						mCurrFilterAddon->mMatchMailPart[idx] = item->Label();
					else
						mCurrFilterAddon->mMatchMailPart[idx] = "";
					UpdateState();
					PropagateChange();
				}
			}
			break;
		}
		case BM_ADDRPART_SELECTED: {
			if (mCurrFilterAddon) {
				int16 idx = msg->FindInt16( MSG_IDX);
				if (idx>=0 && idx<mVisibleLines) {
					BMenuItem* item = mAddrPartControl[idx]->Menu()->FindMarked();
					if (item)
						mCurrFilterAddon->mMatchAddrPart[idx] = item->Label();
					else
						mCurrFilterAddon->mMatchAddrPart[idx] = "";
					UpdateState();
					PropagateChange();
				}
			}
			break;
		}
		case BM_OPERATOR_SELECTED: {
			if (mCurrFilterAddon) {
				int16 idx = msg->FindInt16( MSG_IDX);
				if (idx>=0 && idx<mVisibleLines) {
					BMenuItem* item = mOperatorControl[idx]->Menu()->FindMarked();
					if (item)
						mCurrFilterAddon->mMatchOperator[idx] = item->Label();
					else
						mCurrFilterAddon->mMatchOperator[idx] = "";
					UpdateState();
					PropagateChange();
				}
			}
			break;
		}
		case BM_FILEINTO_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mFileIntoControl->Value();
				mCurrFilterAddon->mActionFileInto = newVal;
				if (!newVal) {
					mCurrFilterAddon->mActionFileIntoValue = "";
					mCurrFilterAddon->mActionFileIntoAsk = false;
				}
				if (newVal && mCurrFilterAddon->mActionDiscard)
					mDiscardControl->SetValue( false);
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_FILEINTO_ASK_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mFileIntoControl->Value();
				mCurrFilterAddon->mActionFileIntoAsk = newVal;
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_FILEINTO_SELECTED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
				if (item) {
					BMenuItem* currItem = item;
					BMenu* currMenu = item->Menu();
					BmString path;
					while( currMenu && currItem 
					&& currItem!=mFileIntoValueControl->MenuItem()) {
						if (!path.Length())
							path.Prepend( BmString(currItem->Label()));
						else
							path.Prepend( BmString(currItem->Label()) << "/");
						currItem = currMenu->Superitem();
						currMenu = currMenu->Supermenu();
					}
					mCurrFilterAddon->mActionFileIntoValue = path;
					mFileIntoValueControl->ClearMark();
					item->SetMarked( true);
					mFileIntoValueControl->MenuItem()->SetLabel( path.String());
				} else {
					mCurrFilterAddon->mActionFileInto = "";
					mFileIntoValueControl->ClearMark();
				}
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_DISCARD_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mDiscardControl->Value();
				mCurrFilterAddon->mActionDiscard = newVal;
				if (newVal && mCurrFilterAddon->mActionFileInto)
					mFileIntoControl->SetValue( false);
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_STATUS_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetStatusControl->Value();
				mCurrFilterAddon->mActionSetStatus = newVal;
				if (!newVal)
					mCurrFilterAddon->mActionSetStatusValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_STATUS_SELECTED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
				if (item)
					mCurrFilterAddon->mActionSetStatusValue = item->Label();
				else
					mCurrFilterAddon->mActionSetStatusValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_IDENTITY_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetIdentityControl->Value();
				mCurrFilterAddon->mActionSetIdentity = newVal;
				if (!newVal)
					mCurrFilterAddon->mActionSetIdentityValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_IDENTITY_SELECTED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
				if (item)
					mCurrFilterAddon->mActionSetIdentityValue = item->Label();
				else
					mCurrFilterAddon->mActionSetIdentityValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_SPAM_TOFU_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetSpamTofuControl->Value();
				mCurrFilterAddon->mActionSetSpamTofu = newVal;
				if (!newVal)
					mCurrFilterAddon->mActionSetSpamTofuValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_SPAM_TOFU_SELECTED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
				if (item)
					mCurrFilterAddon->mActionSetSpamTofuValue = item->Label();
				else
					mCurrFilterAddon->mActionSetSpamTofuValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_LIST_ID_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetListIdControl->Value();
				mCurrFilterAddon->mActionSetListId = newVal;
				if (!newVal)
					mCurrFilterAddon->mActionSetListIdValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_STOP_PROCESSING_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mStopProcessingControl->Value();
				mCurrFilterAddon->mStopProcessing = newVal;
				UpdateState();
				PropagateChange();
			}
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	ShowFilter()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::ShowFilter( BmFilterAddon* addon) {
	mCurrFilterAddon = dynamic_cast< BmGraphicalSieveFilter*>( addon);

	if (mCurrFilterAddon) {
		while( mVisibleLines > mCurrFilterAddon->mMatchCount) {
			mVisibleLines--;
			mMailPartControl[mVisibleLines]->ClearMark();
			mMailPartControl[mVisibleLines]->MenuItem()->SetLabel("");
			mFieldNameControl[mVisibleLines]->SetText("");
			mOperatorControl[mVisibleLines]->ClearMark();
			mOperatorControl[mVisibleLines]->MenuItem()->SetLabel("");
			mValueControl[mVisibleLines]->SetText("");
			mFilterLine[mVisibleLines]->RemoveSelf();
		}
		while( mVisibleLines < mCurrFilterAddon->mMatchCount)
			AddFilterLine();

		mAnyAllControl->MarkItem( mCurrFilterAddon->mMatchAnyAll.String());

		for( int idx=0; idx<mVisibleLines; ++idx) {
			mMailPartControl[idx]->MarkItem( 
				mCurrFilterAddon->mMatchMailPart[idx].String());
			mAddrPartControl[idx]->MarkItem( 
				mCurrFilterAddon->mMatchAddrPart[idx].String());
			mFieldNameControl[idx]->SetTextSilently( 
				mCurrFilterAddon->mMatchFieldName[idx].String());
			mOperatorControl[idx]->MarkItem( 
				mCurrFilterAddon->mMatchOperator[idx].String());
			mValueControl[idx]->SetTextSilently( 
				mCurrFilterAddon->mMatchValue[idx].String());
			AdjustSizeOfValueControl( mValueControl[idx]);
		}
		AdjustScrollView();

		mFileIntoControl->SetValueSilently( mCurrFilterAddon->mActionFileInto);
		mFileIntoValueControl->MarkItem( 
			mCurrFilterAddon->mActionFileIntoValue.String());
		mFileIntoAskControl->SetValueSilently( 
			mCurrFilterAddon->mActionFileIntoAsk);
		mDiscardControl->SetValueSilently( mCurrFilterAddon->mActionDiscard);
		mSetStatusControl->SetValueSilently( mCurrFilterAddon->mActionSetStatus);
		mSetStatusValueControl->MarkItem( 
			mCurrFilterAddon->mActionSetStatusValue.String());
		mSetIdentityControl->SetValueSilently( 
			mCurrFilterAddon->mActionSetIdentity);
		mSetIdentityValueControl->MarkItem( 
			mCurrFilterAddon->mActionSetIdentityValue.String());
		mSetSpamTofuControl->SetValueSilently( 
			mCurrFilterAddon->mActionSetSpamTofu);
		mSetSpamTofuValueControl->MarkItem( 
			mCurrFilterAddon->mActionSetSpamTofuValue.String());
		mSetListIdControl->SetValueSilently(mCurrFilterAddon->mActionSetListId);
		mSetListIdValueControl->SetTextSilently( 
			mCurrFilterAddon->mActionSetListIdValue.String());
		mStopProcessingControl->SetValueSilently( 
			mCurrFilterAddon->mStopProcessing);
	}

	UpdateState();
}

/*------------------------------------------------------------------------------*\
	UpdateState()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::UpdateState() {
	if (mCurrFilterAddon) {
		mAddButton->SetEnabled( mVisibleLines < BM_MAX_MATCH_COUNT);
		for( int idx=0; idx<mVisibleLines; ++idx) {
			BmString mailPart = mCurrFilterAddon->mMatchMailPart[idx];
			if (!mailPart.ICompare( BM_MAILPART_OTHER))
				mFieldSpecLayer[idx]->ActivateLayer( 2);
			else if (mCurrFilterAddon->IsAddrField( mailPart))
				mFieldSpecLayer[idx]->ActivateLayer( 1);
			else
				mFieldSpecLayer[idx]->ActivateLayer( 0);
		}
		mFileIntoValueControl->SetEnabled( mFileIntoControl->Value());
		mFileIntoAskControl->SetEnabled( mFileIntoControl->Value());
		if (!mFileIntoControl->Value())
			mFileIntoValueControl->ClearMark();
		mSetStatusValueControl->SetEnabled( mSetStatusControl->Value());
		if (!mSetStatusControl->Value())
			mSetStatusValueControl->ClearMark();
		mSetIdentityValueControl->SetEnabled( mSetIdentityControl->Value());
		if (!mSetIdentityControl->Value())
			mSetIdentityValueControl->ClearMark();
		mSetSpamTofuValueControl->SetEnabled( mSetSpamTofuControl->Value());
		if (!mSetSpamTofuControl->Value())
			mSetSpamTofuValueControl->ClearMark();
		mSetListIdValueControl->SetEnabled( mSetListIdControl->Value());
		if (!mSetListIdControl->Value())
			mSetListIdValueControl->SetTextSilently("");
	}
}



/********************************************************************************\
	BmSieveScriptFilterPrefs
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmSieveScriptFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveScriptFilterPrefs::BmSieveScriptFilterPrefs( minimax minmax)
	:	inherited( minmax.mini.x, minmax.mini.y, minmax.maxi.x, minmax.maxi.y)
	,	mCurrFilterAddon( NULL)
{
	// 
	AddChild( dynamic_cast<BView*>( 
		new HGroup(
			new VGroup( 
				mContentControl = new BmMultiLineTextControl( 
					"", false, 10
				),
				0
			),
			new Space( minimax( 5, 0, 5, 1e5)),
			new VGroup( 
				mTestButton = new MButton( 
					"Check the SIEVE-script", 
					new BMessage( BM_TEST_FILTER), 
					this, minimax(-1,-1,-1,-1)
				),
				new Space(),
				0
			),
			new Space( minimax( 5, 0, 5, 1e5)),
			0
		)
	));
}

/*------------------------------------------------------------------------------*\
	~BmSieveScriptFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveScriptFilterPrefs::~BmSieveScriptFilterPrefs() {
	TheBubbleHelper->SetHelp( mContentControl, NULL);
	TheBubbleHelper->SetHelp( mTestButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveScriptFilterPrefs::Initialize() {
	TheBubbleHelper->SetHelp( 
		mContentControl, 
		"Here you can enter the content of this filter (a SIEVE script)."
	);
	TheBubbleHelper->SetHelp( 
		mTestButton, 
		"Here you can check the syntax of the SIEVE-script."
	);

	mContentControl->SetTarget( this);
}

/*------------------------------------------------------------------------------*\
	Kind()
		-	
\*------------------------------------------------------------------------------*/
const char* BmSieveScriptFilterPrefs::Kind() const { 
	return FILTER_SIEVE_SCRIPT;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSieveScriptFilterPrefs::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_MULTILINE_TEXTFIELD_MODIFIED: {
			if (mCurrFilterAddon) {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmMultiLineTextControl* source 
					= dynamic_cast<BmMultiLineTextControl*>( srcView);
				if ( source == mContentControl) {
					mCurrFilterAddon->Content( mContentControl->Text());
					PropagateChange();
				}
			}
			break;
		}
		case BM_TEST_FILTER: {
			if (mCurrFilterAddon) {
				if (!mCurrFilterAddon->CompileScript()) {
					BAlert* alert = new BAlert( 
						"Filter test", 
						mCurrFilterAddon->ErrorString().String(),
						"OK", NULL, NULL, B_WIDTH_AS_USUAL,
						B_INFO_ALERT
					);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go();
				}
			}
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveScriptFilterPrefs::ShowFilter( BmFilterAddon* addon) {
	mCurrFilterAddon = dynamic_cast< BmSieveFilter*>( addon);

	bool enabled = (mCurrFilterAddon != NULL);
	mContentControl->SetEnabled( enabled);

	if (!mCurrFilterAddon) {
		mContentControl->SetTextSilently( "");
		mTestButton->SetEnabled( false);
	} else {
		mContentControl->SetTextSilently( mCurrFilterAddon->Content().String());
		mTestButton->SetEnabled( true);
	}
	
}



/*------------------------------------------------------------------------------*\
	InstantiateFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
extern "C"
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind) {
	if (!kind.ICompare( FILTER_SIEVE))
		return new BmSieveFilterPrefs( minimax( int(minx), int(miny), int(maxx), 
															 int(maxy)));
	else if (!kind.ICompare( FILTER_SIEVE_SCRIPT))
		return new BmSieveScriptFilterPrefs( minimax( int(minx), int(miny), 
																	 int(maxx), int(maxy)));
	else
		return NULL;
}
