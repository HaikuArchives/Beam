/*
	BmSieveFilter.cpp

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


#include <Alert.h>
#include <Application.h>
#include <File.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>

#include <VGroup.h>
#include <HGroup.h>
#include <Space.h>
#include <MBorder.h>
#include <MButton.h>
#include <MScrollView.h>
#include <MStringView.h>

#include "regexx.hh"
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
#include "BmMenuControl.h"
#include "BmMultiLineTextControl.h"
#include "BmTextControl.h"

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

#define FILTER_SIEVE 			"Sieve"
#define FILTER_SIEVE_SCRIPT 	"Sieve-Script"

extern "C" const char* FilterKinds[] = {
	FILTER_SIEVE,
	FILTER_SIEVE_SCRIPT,
	NULL
};

static const BmString BmNotifySetIdentity = "SetIdentity";

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
bool BmSieveFilter::Execute( void* message_context) {
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
			return false;
		}
	}
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	BmString mailId;
	if (msgContext)
		mailId = msgContext->mailId;
	BM_LOG2( BM_LogFilter, BmString("Executing Sieve-filter <") << Name() 
									<< "> on mail with Id <" << mailId << ">");
	int res = sieve_execute_script( mCompiledScript, msgContext);
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

	// set lock to serialize SIEVE-calls:
	BAutolock lock( SieveLock());
	if (!lock.IsLocked()) {
		mLastErr = "Unable to get SIEVE-lock";
		return false;
	}

	mLastErr = mLastSieveErr = "";

	// create sieve interpreter:
	int res = sieve_interp_alloc( &mSieveInterp, this);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Name()) << ": Could not create SIEVE-interpreter";
		goto cleanup;
	}
	RegisterCallbacks( mSieveInterp);

	// create temporary file with script-contents (since SIEVE parses from file only):
	scriptFileName = "/tmp/bm_sieve_script";
	err = scriptBFile.SetTo( scriptFileName.String(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (err != B_OK) {
		mLastErr = BmString(Name()) << ":\nCould not create temporary file\n\t<" << scriptFileName << ">\n\n Result: " << strerror(err);
		goto cleanup;
	}
	scriptBFile.Write( mContent.String(), mContent.Length());
	scriptBFile.Unset();

	// open script file for sieve...
	scriptFile = fopen( scriptFileName.String(), "r");
	if (!scriptFile) {
		mLastErr = BmString(Name()) << ":\nCould not re-open file \n\t<" << scriptFileName << ">";
		goto cleanup;
	}
	// ...and compile the script:	
	if (mCompiledScript) {
		sieve_script_free( &mCompiledScript);
		mCompiledScript = NULL;
	}
	res = sieve_script_parse( mSieveInterp, scriptFile, this, &mCompiledScript);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Name()) << ":\nThe script could not be parsed correctly";
		goto cleanup;
	}
	ret = true;

cleanup:
	mLastErrVal = res;
	if (scriptFile)
		fclose( scriptFile);
	return ret;
}

/*------------------------------------------------------------------------------*\
	RegisterCallbacks()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilter::RegisterCallbacks( sieve_interp_t* interp) {
	sieve_register_redirect( interp, BmSieveFilter::sieve_redirect);
	sieve_register_keep( interp, BmSieveFilter::sieve_keep);
	sieve_register_fileinto( interp, BmSieveFilter::sieve_fileinto);
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
int BmSieveFilter::sieve_redirect( void* action_context, void* interp_context, 
			   							  void* script_context, void* message_context, 
			   							  const char** errmsg) {
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	SetMailFlags()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilter::SetMailFlags( sieve_imapflags_t* flags, BmMsgContext* msgContext) {
	if (msgContext && flags) {
		for( int i=0; i<flags->nflags; ++i) {
			BmString flag( flags->flag[i]);
			if (!flag.ICompare( "\\Seen"))
				msgContext->status = "Read";
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
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_keep_context* keepContext 
		= static_cast< sieve_keep_context*>( action_context);
	if (msgContext && keepContext)
		SetMailFlags( keepContext->imapflags, msgContext);
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_fileinto()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_fileinto( void* action_context, void*, 
			   				  			 void*, void* message_context, 
			   				 			 const char**) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	sieve_fileinto_context* fileintoContext 
		= static_cast< sieve_fileinto_context*>( action_context);
	if (msgContext && fileintoContext) {
		SetMailFlags( fileintoContext->imapflags, msgContext);
		msgContext->folderName = fileintoContext->mailbox;
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
		if (!BmNotifySetIdentity.ICompare( notifyContext->method)) {
			// set identity for mail according to notify-options:
			msgContext->identity = notifyContext->options[0];
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
		*sizePtr = msgContext->rawMsgText.Length();
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_header()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_get_header( void* message_context, const char* header,
			  									 const char*** contentsPtr) {
	static const char* dummy[] = { NULL };
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext && contentsPtr && header) {
		*contentsPtr = dummy;
		for( int i=0; i<msgContext->headerInfoCount; ++i) {
			if (!msgContext->headerInfos[i].fieldName.ICompare( header)) {
				*contentsPtr = msgContext->headerInfos[i].values;
				break;
			}
		}
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	execute_error()
		-	
\*------------------------------------------------------------------------------*/
int BmSieveFilter::sieve_execute_error( const char* msg, void*,
													void* script_context, void* message_context) {
	BmString filterName = "<unknown>";
	BmString mailName = "<unknown>";
	if (script_context) {
		BmSieveFilter* filter = static_cast< BmSieveFilter*>( script_context);
		if (filter)
			filterName = filter->Name();
	}
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	if (msgContext)
		mailName = msgContext->mailId;
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

#define BM_MAILPART_OTHER  "<Specify field>..."
#define BM_MAILPART_HEADER "<Message-Header>"
#define BM_MAILPART_BODY   "<Message-Text>"
static const char* mailParts[] = {
	"Bcc",
	"Cc",
	"Date",
	"From",
	"Reply-To",
	"Subject",
	"To",
	BM_MAILPART_OTHER,
//	BM_MAILPART_BODY,
	NULL
};

static const char* operators[][3] = {
	{ "is", "", "is"},
	{ "is not", "not", "is"},
	{ "contains", "", "contains"},
	{ "does not contain", "not", "contains"},
	{ "starts with", "", "matches"},
	{ "does not start with", "not", "matches"},
	{ "ends with", "", "matches"},
	{ "does not end with", "not", "matches"},
	{ " < ", "", "under"},
	{ " >= ", "not", "under"},
	{ " > ", "", "over"},
	{ " <= ", "not", "over"},
	{ "matches regex", "", "regex"},
	{ "does not match regex", "not", "regex"},
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
const char* const BmGraphicalSieveFilter::MSG_MATCH_FIELDNAME = 	"bm:fn";
const char* const BmGraphicalSieveFilter::MSG_MATCH_OPERATOR = 	"bm:op";
const char* const BmGraphicalSieveFilter::MSG_MATCH_VALUE = 		"bm:vl";
const char* const BmGraphicalSieveFilter::MSG_FILEINTO = 			"bm:fi";
const char* const BmGraphicalSieveFilter::MSG_FILEINTO_VALUE = 	"bm:fiv";
const char* const BmGraphicalSieveFilter::MSG_DISCARD = 				"bm:ds";
const char* const BmGraphicalSieveFilter::MSG_SET_STATUS = 			"bm:sst";
const char* const BmGraphicalSieveFilter::MSG_SET_STATUS_VALUE = 	"bm:sstv";
const char* const BmGraphicalSieveFilter::MSG_SET_IDENTITY =		"bm:sid";
const char* const BmGraphicalSieveFilter::MSG_SET_IDENTITY_VALUE ="bm:sidv";

/*------------------------------------------------------------------------------*\
	BmGraphicalSieveFilter( name, archive)
		-	c'tor
		-	constructs a BmGraphicalSieveFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmGraphicalSieveFilter::BmGraphicalSieveFilter( const BmString& name, const BMessage* archive) 
	:	inherited( name, archive)
	,	MatchCount( 1)
	,	MatchAnyAll( choices[0])
	,	ActionFileInto( false)
	,	ActionDiscard( false)
	,	ActionSetStatus( false)
	,	ActionSetIdentity( false)
{
	if (archive) {
		archive->FindInt16( MSG_MATCH_COUNT, &MatchCount);
		if (MatchCount <= 0) 
			MatchCount = 1;
		MatchAnyAll = archive->FindString( MSG_MATCH_ANYALL);
		if (!MatchAnyAll.Length())
			MatchAnyAll = choices[0];
		ActionFileInto = archive->FindBool( MSG_FILEINTO);
		ActionFileIntoValue = archive->FindString( MSG_FILEINTO_VALUE);
		ActionDiscard = archive->FindBool( MSG_DISCARD);
		ActionSetStatus = archive->FindBool( MSG_SET_STATUS);
		ActionSetStatusValue = archive->FindString( MSG_SET_STATUS_VALUE);
		ActionSetIdentity = archive->FindBool( MSG_SET_IDENTITY);
		ActionSetIdentityValue = archive->FindString( MSG_SET_IDENTITY_VALUE);

		for( int i=0; i<MatchCount; ++i) {
			MatchMailPart[i] = archive->FindString( MSG_MATCH_MAILPART, i);
			MatchFieldName[i] = archive->FindString( MSG_MATCH_FIELDNAME, i);
			MatchOperator[i] = archive->FindString( MSG_MATCH_OPERATOR, i);
			MatchValue[i] = archive->FindString( MSG_MATCH_VALUE, i);
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
		|| archive->AddInt16( MSG_MATCH_COUNT, MatchCount)
		||	archive->AddString( MSG_MATCH_ANYALL, MatchAnyAll.String())
		|| archive->AddBool( MSG_FILEINTO, ActionFileInto)
		||	archive->AddString( MSG_FILEINTO_VALUE, ActionFileIntoValue.String())
		|| archive->AddBool( MSG_DISCARD, ActionDiscard)
		|| archive->AddBool( MSG_SET_STATUS, ActionSetStatus)
		||	archive->AddString( MSG_SET_STATUS_VALUE, ActionSetStatusValue.String())
		|| archive->AddBool( MSG_SET_IDENTITY, ActionSetIdentity)
		||	archive->AddString( MSG_SET_IDENTITY_VALUE, ActionSetIdentityValue.String()));
	for( int i=0; ret==B_OK && i<MatchCount; ++i) {
		ret = (archive->AddString( MSG_MATCH_MAILPART, MatchMailPart[i].String())
			 || archive->AddString( MSG_MATCH_FIELDNAME, MatchFieldName[i].String())
			 || archive->AddString( MSG_MATCH_OPERATOR, MatchOperator[i].String())
			 || archive->AddString( MSG_MATCH_VALUE, MatchValue[i].String()));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmGraphicalSieveFilter::SanityCheck( BmString& complaint, BmString& fieldName) {
	if (ActionFileInto && !ActionFileIntoValue.Length()) {
		complaint = "Please select a folder to file the matching mails into";
		fieldName = "fileIntoValue";
		return false;
	} else if (ActionSetStatus && !ActionSetStatusValue.Length()) {
		complaint = "Please select the status that matching mails should be set to";
		fieldName = "setStatusValue";
		return false;
	} else if (ActionSetIdentity && !ActionSetIdentityValue.Length()) {
		complaint = "Please select the identity that matching mails should be associated with";
		fieldName = "setIdentityValue";
		return false;
	}
	int validMatchCount=MatchCount;
	for( int i=0; i<MatchCount; ++i) {
		if (!MatchMailPart[i].Length() && !MatchOperator[i].Length()) {
			validMatchCount--;
			continue;
		}
		if (!MatchMailPart[i].Length()) {
			complaint = "Please select the mail-part that is to be filtered";
			fieldName = BmString("mailPart:") << i;
			return false;
		} else if (MatchMailPart[i] == BM_MAILPART_OTHER && !MatchFieldName[i].Length()) {
			complaint = "Please enter the name of the header-field that is to be filtered";
			fieldName = BmString("fieldName") << i;
			return false;
		} else if (!MatchOperator[i].Length()) {
			complaint = "Please select the filter-operator";
			fieldName = BmString("operator") << i;
			return false;
		} else if (!MatchValue[i].Length()) {
			complaint = "Please select the value that is to be used for the comparison";
			fieldName = BmString("value") << i;
			return false;
		}
	}
	if (!validMatchCount && ActionDiscard) {
		complaint = "You have created a filter that will THROW AWAY EACH AND EVERY MAIL!\n\nMy advice: don't do that >:o)";
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
	BmString script("# generated by Beam's GUI-editor, please do not edit >:o)\n");
	bool needRegex = false;
	for( int i=0; i<MatchCount; ++i) {
		if (MatchOperator[i] == operators[12][0] 
		|| MatchOperator[i] == operators[13][0]) {
			needRegex = true;
		}
	}
	if (ActionFileInto)
		script << "require \"fileinto\";\n";
	if (ActionSetStatus)
		script << "require \"imapflags\";\n";
	if (ActionSetIdentity)
		script << "require \"notify\";\n";
	if (needRegex)
		script << "require \"regex\";\n";
	BmString matchPart;
	for( int i=0; i<MatchCount; ++i) {
		if (!MatchMailPart[i].Length() && !MatchOperator[i].Length())
			continue;
		BmString neg;
		BmString op;
		for( int o=0; operators[o][0]; ++o) {
			if (MatchOperator[i] == operators[o][0]) {
				neg = operators[o][1];
				op = operators[o][2];
				break;
			}
		}
		BmString val = MatchValue[i];
		if (MatchOperator[i].IFindFirst("start") != B_ERROR)
			val.Append("*");
		else if (MatchOperator[i].IFindFirst("end") != B_ERROR)
			val.Prepend("*");
		bool otherField = MatchMailPart[i] == BM_MAILPART_OTHER;
		BmString line = (neg.Length() ? (neg << " ") : BmString(""))
			<< "header " << ":" << op << " "
			<< "\"" << (otherField ? MatchFieldName[i] : MatchMailPart[i]) << "\" "
			<< "\"" <<  val << "\",\n";
		matchPart << line;
	}
	if (matchPart.Length()) {
		if (MatchAnyAll == choices[0]) {
			// all of
			script << "if allof (" << matchPart << "true) {\n";
		} else {
			// any of
			script << "if anyof (" << matchPart << "false) {\n";
		}
	}
	if (ActionFileInto)
		script << "   fileinto \"" << ActionFileIntoValue << "\";\n";
	if (ActionSetIdentity)
		script << "   notify :method \"SetIdentity\" :options \"" << ActionSetIdentityValue << "\";\n";
	if (ActionDiscard)
		script << "   discard;\n";
	if (matchPart.Length()) {
		script << "}";
	}
	mContent = script;
	return true;
}



/*------------------------------------------------------------------------------*\
	InstantiateFilter()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport)
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
	BmSieveFilterPrefs
\********************************************************************************/

const char* const BmSieveFilterPrefs::MSG_IDX = 		"bm:idx";

/*------------------------------------------------------------------------------*\
	BmSieveFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveFilterPrefs::BmSieveFilterPrefs( minimax minmax)
	:	inherited( minmax)
	,	mCurrFilterAddon( NULL)
{
	VGroup* vgroup = 
		new VGroup( 
			new MBorder( M_LABELED_BORDER, 5, (char*)"Selection Criteria",
				mFilterGroup = new VGroup(
					new HGroup(
						mAnyAllControl = new BmMenuControl( "Match if ", 
																		new BPopUpMenu( "")),
						new MStringView( (char*)" the following is true"),
						new Space( minimax(1,1,1e5,1)),
						mAddButton = new MButton( "Add Line", 
														  new BMessage( BM_ADD_FILTER_LINE), 
														  this, minimax(-1,-1,-1,-1)),
						mRemoveButton = new MButton( "Remove Line", 
															  new BMessage( BM_REMOVE_FILTER_LINE), 
															  this, minimax(-1,-1,-1,-1)),
						0
					),
					0
				)
			),
			new MBorder( M_LABELED_BORDER, 5, (char*)"Filter Actions",
				mActionGroup = new VGroup(
					new HGroup(
						new VGroup(
							new HGroup(
								mFileIntoControl = new BmCheckControl( "File into", 
																				 	new BMessage(BM_FILEINTO_CHANGED), 
																					this),
								mFileIntoValueControl = new BmMenuControl( NULL, new BPopUpMenu("")),
								0
							),
							new HGroup(
								mDiscardControl = new BmCheckControl( "Move to trash", 
																				 	new BMessage(BM_DISCARD_CHANGED), 
																					this),
								new Space(),
								0
							),
							0
						),
						new Space( minimax( 10,0,10,1e5)),
						new VGroup(
							new HGroup(
								mSetStatusControl = new BmCheckControl( "Set Status", 
																					 new BMessage(BM_SET_STATUS_CHANGED), 
																					 this),
								mSetStatusValueControl = new BmMenuControl( NULL, new BPopUpMenu("")),
								0
							),
							new HGroup(
								mSetIdentityControl = new BmCheckControl( "Set Identity", 
																					 new BMessage(BM_SET_IDENTITY_CHANGED), 
																					 this),
								mSetIdentityValueControl = new BmMenuControl( NULL, new BPopUpMenu("")),
								0
							),
							new Space(),
							0
						),
						0
					),
					0
				)
			),
			new Space(),
			0
		);
	
	float width, maxWidth, height; 
	mFileIntoControl->GetPreferredSize( &maxWidth, &height);
	mFileIntoControl->ct_mpm.maxi.x = mFileIntoControl->ct_mpm.mini.x = maxWidth+5;

	mSetStatusControl->GetPreferredSize( &maxWidth, &height);
	mSetIdentityControl->GetPreferredSize( &width, &height);
	if (width>maxWidth)
		maxWidth = width;
	mSetStatusControl->ct_mpm.maxi.x = mSetStatusControl->ct_mpm.mini.x 
	= mSetIdentityControl->ct_mpm.maxi.x = mSetIdentityControl->ct_mpm.mini.x 
	= maxWidth+5;

	mFileIntoValueControl->SetDivider( 12);
	mSetStatusValueControl->SetDivider( 12);
	mSetIdentityValueControl->SetDivider( 12);

	mAnyAllControl->SetDivider( 70);
	mAnyAllControl->ct_mpm.maxi.x = 120;

	for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		mFilterLine[i] = 
			new HGroup(
				mMailPartControl[i] = new BmMenuControl( NULL, new BPopUpMenu( "")),
				mFieldNameControl[i] = new BmTextControl( NULL),
				mOperatorControl[i] = new BmMenuControl( NULL, new BPopUpMenu( "")),
				mValueControl[i] = new BmTextControl( NULL),
				0
			);
		mMailPartControl[i]->SetDivider( 12);
		mMailPartControl[i]->ct_mpm = minimax(130,-1,130,1E5,0.5);
		mFieldNameControl[i]->SetDivider( 12);
		mFieldNameControl[i]->ct_mpm = minimax(40,-1,100,1E5);
		mOperatorControl[i]->SetDivider( 12);
		mOperatorControl[i]->ct_mpm = minimax(140,-1,140,1E5,0.5);
		mValueControl[i]->SetDivider( 12);

		mFilterGroup->AddChild( mFilterLine[i]);
	}		
	mVisibleLines = BM_MAX_MATCH_COUNT;

	AddChild( dynamic_cast<BView*>( vgroup));
}

/*------------------------------------------------------------------------------*\
	~BmSieveFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSieveFilterPrefs::~BmSieveFilterPrefs() {
//	TheBubbleHelper->SetHelp( mContentControl, NULL);
//	TheBubbleHelper->SetHelp( mTestButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::Initialize() {
//	TheBubbleHelper->SetHelp( mContentControl, "Here you can enter the content of this filter (a SIEVE script).");
//	TheBubbleHelper->SetHelp( mTestButton, "Here you can check the syntax of the SIEVE-script.");

	for( const char** itemP = choices; *itemP; ++itemP) {
		BMenuItem* item = new BMenuItem( *itemP, new BMessage( BM_ANY_ALL_SELECTED));
		item->SetTarget( this);
		mAnyAllControl->Menu()->AddItem( item);
	}
	for( int i=0; i<BM_MAX_MATCH_COUNT; ++i) {
		for( const char** itemP = mailParts; *itemP; ++itemP) {
			BMessage* msg = new BMessage( BM_MAILPART_SELECTED);
			msg->AddInt16( MSG_IDX, i);
			BMenuItem* item = new BMenuItem( *itemP, msg);
			item->SetTarget( this);
			mMailPartControl[i]->Menu()->AddItem( item);
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
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::Activate() {
	BMessage reply;
	BMessage msg( BM_FILL_MENU_FROM_LIST);
	BMenuItem* item;
	//
	BMessage folderTempl( BM_FILEINTO_SELECTED);
	msg.MakeEmpty();
	while( (item = mFileIntoValueControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	msg.AddString( MSG_LIST_NAME, BM_FOLDERLIST_NAME);
	msg.AddPointer( MSG_MENU_POINTER, mFileIntoValueControl->Menu());
	msg.AddPointer( MSG_MENU_TARGET, dynamic_cast<BHandler*>(this));
	msg.AddMessage( MSG_MSG_TEMPLATE, &folderTempl);
	be_app_messenger.SendMessage( &msg, &reply);
	//
	BMessage statusTempl( BM_SET_STATUS_SELECTED);
	msg.MakeEmpty();
	while( (item = mSetStatusValueControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	msg.AddString( MSG_LIST_NAME, BM_STATUSLIST_NAME);
	msg.AddPointer( MSG_MENU_POINTER, mSetStatusValueControl->Menu());
	msg.AddPointer( MSG_MENU_TARGET, dynamic_cast<BHandler*>(this));
	msg.AddMessage( MSG_MSG_TEMPLATE, &statusTempl);
	be_app_messenger.SendMessage( &msg, &reply);
	//
	BMessage identityTempl( BM_SET_IDENTITY_SELECTED);
	msg.MakeEmpty();
	while( (item = mSetIdentityValueControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	msg.AddString( MSG_LIST_NAME, BM_IDENTITYLIST_NAME);
	msg.AddPointer( MSG_MENU_POINTER, mSetIdentityValueControl->Menu());
	msg.AddPointer( MSG_MENU_TARGET, dynamic_cast<BHandler*>(this));
	msg.AddMessage( MSG_MSG_TEMPLATE, &identityTempl);
	be_app_messenger.SendMessage( &msg, &reply);
}

/*------------------------------------------------------------------------------*\
	AddFilterLine()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::AddFilterLine() { 
	if (mVisibleLines < BM_MAX_MATCH_COUNT) {
		mFilterGroup->AddChild( mFilterLine[mVisibleLines++]);
		layoutprefs();
		layout( Frame());
	}
}

/*------------------------------------------------------------------------------*\
	RemoveFilterLine()
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::RemoveFilterLine() { 
	if (mVisibleLines>1) {
		mVisibleLines--;
		mMailPartControl[mVisibleLines]->ClearMark();
		mMailPartControl[mVisibleLines]->MenuItem()->SetLabel("");
		mFieldNameControl[mVisibleLines]->SetText("");
		mOperatorControl[mVisibleLines]->ClearMark();
		mOperatorControl[mVisibleLines]->MenuItem()->SetLabel("");
		mValueControl[mVisibleLines]->SetText("");
		mFilterLine[mVisibleLines]->RemoveSelf();
		layoutprefs();
		layout( Frame());
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
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSieveFilterPrefs::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_ADD_FILTER_LINE: {
			if (mVisibleLines<BM_MAX_MATCH_COUNT && mCurrFilterAddon) {
				AddFilterLine();
				mCurrFilterAddon->MatchCount++;
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_REMOVE_FILTER_LINE: {
			if (mVisibleLines>1 && mCurrFilterAddon) {
				RemoveFilterLine();
				mCurrFilterAddon->MatchCount--;
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_ANY_ALL_SELECTED: {
			if (mCurrFilterAddon) {
				BMenuItem* item = mAnyAllControl->Menu()->FindMarked();
				if (item)
					mCurrFilterAddon->MatchAnyAll = item->Label();
				else
					mCurrFilterAddon->MatchAnyAll = "";
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
				for( int idx=0; idx<mVisibleLines; ++idx) {
					if (source == mFieldNameControl[idx]) {
						mCurrFilterAddon->MatchFieldName[idx] = mFieldNameControl[idx]->Text();
						UpdateState();
						PropagateChange();
					} else if (source == mValueControl[idx]) {
						mCurrFilterAddon->MatchValue[idx] = mValueControl[idx]->Text();
						UpdateState();
						PropagateChange();
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
						mCurrFilterAddon->MatchMailPart[idx] = item->Label();
					else
						mCurrFilterAddon->MatchMailPart[idx] = "";
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
						mCurrFilterAddon->MatchOperator[idx] = item->Label();
					else
						mCurrFilterAddon->MatchOperator[idx] = "";
					UpdateState();
					PropagateChange();
				}
			}
			break;
		}
		case BM_FILEINTO_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mFileIntoControl->Value();
				mCurrFilterAddon->ActionFileInto = newVal;
				if (!newVal)
					mCurrFilterAddon->ActionFileIntoValue = "";
				if (newVal && mCurrFilterAddon->ActionDiscard)
					mDiscardControl->SetValue( false);
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
					while( currMenu && currItem && currItem!=mFileIntoValueControl->MenuItem()) {
						if (!path.Length())
							path.Prepend( BmString(currItem->Label()));
						else
							path.Prepend( BmString(currItem->Label()) << "/");
						currItem = currMenu->Superitem();
						currMenu = currMenu->Supermenu();
					}
					mCurrFilterAddon->ActionFileIntoValue = path;
					mFileIntoValueControl->ClearMark();
					item->SetMarked( true);
					mFileIntoValueControl->MenuItem()->SetLabel( path.String());
				} else {
					mCurrFilterAddon->ActionFileInto = "";
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
				mCurrFilterAddon->ActionDiscard = newVal;
				if (newVal && mCurrFilterAddon->ActionFileInto)
					mFileIntoControl->SetValue( false);
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_STATUS_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetStatusControl->Value();
				mCurrFilterAddon->ActionSetStatus = newVal;
				if (!newVal)
					mCurrFilterAddon->ActionSetStatusValue = "";
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
					mCurrFilterAddon->ActionSetStatusValue = item->Label();
				else
					mCurrFilterAddon->ActionSetStatusValue = "";
				UpdateState();
				PropagateChange();
			}
			break;
		}
		case BM_SET_IDENTITY_CHANGED: {
			if (mCurrFilterAddon) {
				bool newVal = mSetIdentityControl->Value();
				mCurrFilterAddon->ActionSetIdentity = newVal;
				if (!newVal)
					mCurrFilterAddon->ActionSetIdentityValue = "";
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
					mCurrFilterAddon->ActionSetIdentityValue = item->Label();
				else
					mCurrFilterAddon->ActionSetIdentityValue = "";
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
		while( mVisibleLines > mCurrFilterAddon->MatchCount)
			RemoveFilterLine();
		while( mVisibleLines < mCurrFilterAddon->MatchCount)
			AddFilterLine();

		mAnyAllControl->MarkItem( mCurrFilterAddon->MatchAnyAll.String());

		for( int idx=0; idx<mVisibleLines; ++idx) {
			mMailPartControl[idx]->MarkItem( mCurrFilterAddon->MatchMailPart[idx].String());
			mFieldNameControl[idx]->SetTextSilently( mCurrFilterAddon->MatchFieldName[idx].String());
			mOperatorControl[idx]->MarkItem( mCurrFilterAddon->MatchOperator[idx].String());
			mValueControl[idx]->SetTextSilently( mCurrFilterAddon->MatchValue[idx].String());
		}

		mFileIntoControl->SetValueSilently( mCurrFilterAddon->ActionFileInto);
		mFileIntoValueControl->MarkItem( mCurrFilterAddon->ActionFileIntoValue.String(), true);
		mDiscardControl->SetValueSilently( mCurrFilterAddon->ActionDiscard);
		mSetStatusControl->SetValueSilently( mCurrFilterAddon->ActionSetStatus);
		mSetStatusValueControl->MarkItem( mCurrFilterAddon->ActionSetStatusValue.String());
		mSetIdentityControl->SetValueSilently( mCurrFilterAddon->ActionSetIdentity);
		mSetIdentityValueControl->MarkItem( mCurrFilterAddon->ActionSetIdentityValue.String());
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
		mRemoveButton->SetEnabled( mVisibleLines > 1);
		for( int idx=0; idx<mVisibleLines; ++idx) {
			mFieldNameControl[idx]->SetEnabled( 
				!mCurrFilterAddon->MatchMailPart[idx].ICompare( BM_MAILPART_OTHER));
		}
		mFileIntoValueControl->SetEnabled( mFileIntoControl->Value());
		if (!mFileIntoControl->Value())
			mFileIntoValueControl->ClearMark();
		mSetStatusValueControl->SetEnabled( mSetStatusControl->Value());
		if (!mSetStatusControl->Value())
			mSetStatusValueControl->ClearMark();
		mSetIdentityValueControl->SetEnabled( mSetIdentityControl->Value());
		if (!mSetIdentityControl->Value())
			mSetIdentityValueControl->ClearMark();
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
	:	inherited( minmax)
	,	mCurrFilterAddon( NULL)
{
	// 
	AddChild( dynamic_cast<BView*>( 
		new HGroup(
			new VGroup( 
				mContentControl = new BmMultiLineTextControl( "", false, 10),
				0
			),
			new Space( minimax( 5, 0, 5, 1e5)),
			new VGroup( 
				mTestButton = new MButton( "Check the SIEVE-script", new BMessage( BM_TEST_FILTER), this, minimax(-1,-1,-1,-1)),
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
	TheBubbleHelper->SetHelp( mContentControl, "Here you can enter the content of this filter (a SIEVE script).");
	TheBubbleHelper->SetHelp( mTestButton, "Here you can check the syntax of the SIEVE-script.");

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
				BmMultiLineTextControl* source = dynamic_cast<BmMultiLineTextControl*>( srcView);
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
					BAlert* alert = new BAlert( "Filter-Test", 
														 mCurrFilterAddon->ErrorString().String(),
												 		 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
												 		 B_INFO_ALERT);
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
extern "C" __declspec(dllexport) 
BmFilterAddonPrefsView* InstantiateFilterPrefs( minimax minmax, const BmString& kind) {
	if (!kind.ICompare( FILTER_SIEVE))
		return new BmSieveFilterPrefs( minmax);
	else if (!kind.ICompare( FILTER_SIEVE_SCRIPT))
		return new BmSieveScriptFilterPrefs( minmax);
	else
		return NULL;
}
