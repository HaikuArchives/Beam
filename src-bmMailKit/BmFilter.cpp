/*
	BmFilter.cpp

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


#include <Message.h>

#include "BmBasics.h"
#include "BmPrefs.h"
#include "BmFilter.h"
#include "BmMailFilter.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmFilter
\********************************************************************************/

const char* const BmFilter::MSG_NAME = 		"bm:name";
const char* const BmFilter::MSG_CONTENT = 	"bm:content";
const int16 BmFilter::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	BmFilter()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilter::BmFilter( const char* name, BmFilterList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mCompiledScript( NULL)
{
}

/*------------------------------------------------------------------------------*\
	BmFilter( archive)
		-	c'tor
		-	constructs a BmFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmFilter::BmFilter( BMessage* archive, BmFilterList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
	,	mCompiledScript( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mContent = FindMsgString( archive, MSG_CONTENT);
}

/*------------------------------------------------------------------------------*\
	~BmFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilter::~BmFilter() {
	if (mCompiledScript)
		sieve_script_free( &mCompiledScript);
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmFilter::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_CONTENT, mContent.String()));
	return ret;
}

/*------------------------------------------------------------------------------*\
	CompileScript()
		-	
\*------------------------------------------------------------------------------*/
bool BmFilter::CompileScript() {
	bool ret = false;
	sieve_interp_t* sieveInterp = NULL;
	FILE* scriptFile = NULL;
	BmString scriptFileName;
	status_t err;
	BFile scriptBFile;

	mLastErr = mLastSieveErr = "";

	// create sieve interpreter:
	int res = sieve_interp_alloc( &sieveInterp, this);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Key()) << ": Could not create SIEVE-interpreter";
		goto cleanup;
	}
	RegisterCallbacks( sieveInterp);

	// create temporary file with script-contents (since SIEVE parses from file only):
	scriptFileName = TheTempFileList.NextTempFilenameWithPath();
	err = scriptBFile.SetTo( scriptFileName.String(), B_CREATE_FILE | B_WRITE_ONLY);
	if (err != B_OK) {
		mLastErr = BmString(Key()) << ":\nCould not create temporary file\n\t<" << scriptFileName << ">\n\n Result: " << strerror(err);
		goto cleanup;
	}
	scriptBFile.Write( mContent.String(), mContent.Length());
	scriptBFile.Unset();

	// open script file for sieve...
	scriptFile = fopen( scriptFileName.String(), "r");
	if (!scriptFile) {
		mLastErr = BmString(Key()) << ":\nCould not re-open file \n\t<" << scriptFileName << ">";
		goto cleanup;
	}
	// ...and compile the script:	
	res = sieve_script_parse( sieveInterp, scriptFile, this, &mCompiledScript);
	if (res != SIEVE_OK) {
		mLastErr = BmString(Key()) << ":\nThe script could not be parsed correctly";
		goto cleanup;
	}
	ret = true;

cleanup:
	mLastErrVal = res;
	if (scriptFile)
		fclose( scriptFile);
	if (scriptFileName.Length())
		TheTempFileList.RemoveFile( scriptFileName);
	if (sieveInterp)
		sieve_interp_free( &sieveInterp);
	return ret;
}

/*------------------------------------------------------------------------------*\
	RegisterCallbacks()
		-	
\*------------------------------------------------------------------------------*/
void BmFilter::RegisterCallbacks( sieve_interp_t* interp) {
	sieve_register_redirect( interp, BmMailFilter::sieve_redirect);
	sieve_register_keep( interp, BmMailFilter::sieve_keep);
	sieve_register_fileinto( interp, BmMailFilter::sieve_fileinto);

	sieve_register_size( interp, BmMailFilter::sieve_get_size);
	sieve_register_header( interp, BmMailFilter::sieve_get_header);

	sieve_register_parse_error( interp, BmFilter::sieve_parse_error);
}

/*------------------------------------------------------------------------------*\
	sieve_parse_error()
		-	
\*------------------------------------------------------------------------------*/
int BmFilter::sieve_parse_error( int lineno, const char* msg, 
											void* interp_context, void* script_context) {
	if (script_context) {
		BmFilter* filter = static_cast< BmFilter*>( script_context);
		if (filter)
			filter->mLastSieveErr = BmString("Line ")<<lineno<<": "<<msg;
	}
	return SIEVE_OK;
}



/********************************************************************************\
	BmFilterList
\********************************************************************************/

BmRef< BmFilterList> BmFilterList::theInboundInstance( NULL);
BmRef< BmFilterList> BmFilterList::theOutboundInstance( NULL);

const int16 BmFilterList::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	CreateInboundInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmFilterList* BmFilterList::CreateInboundInstance() {
	if (!theInboundInstance)
		theInboundInstance = new BmFilterList( "FilterList_Inbound");
	return theInboundInstance.Get();
}

/*------------------------------------------------------------------------------*\
	CreateOutboundInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmFilterList* BmFilterList::CreateOutboundInstance() {
	if (!theOutboundInstance)
		theOutboundInstance = new BmFilterList( "FilterList_Outbound");
	return theOutboundInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmFilterList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmFilterList::BmFilterList( const char* name)
	:	inherited( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmFilterList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmFilterList::~BmFilterList() {
	if (theInboundInstance == this)
		theInboundInstance = NULL;
	else if (theOutboundInstance == this)
		theOutboundInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the signature-list
\*------------------------------------------------------------------------------*/
const BmString BmFilterList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/" 
				<< "Filters_" << (TheOutboundFilterList==this ? "outbound" : "inbound");
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the signature-list from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogUtil, BmString("Start of InstantiateItems() for FilterList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find signature nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmFilter* newFilter = new BmFilter( &msg, this);
		BM_LOG3( BM_LogUtil, BmString("Filter <") << newFilter->Name() << "," << newFilter->Key() << "> read");
		AddItemToList( newFilter);
	}
	BM_LOG2( BM_LogUtil, BmString("End of InstantiateItems() for FilterList"));
	mInitCheck = B_OK;
}

