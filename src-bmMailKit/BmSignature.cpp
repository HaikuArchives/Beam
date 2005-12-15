/*
	BmSignature.cpp

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

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmSignature.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSignature
\********************************************************************************/

const char* const BmSignature::MSG_NAME = 		"bm:name";
const char* const BmSignature::MSG_DYNAMIC = 	"bm:dynamic";
const char* const BmSignature::MSG_CONTENT = 	"bm:content";
const char* const BmSignature::MSG_CHARSET = 	"bm:charset";
const int16 BmSignature::nArchiveVersion = 3;

/*------------------------------------------------------------------------------*\
	BmSignature()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmSignature::BmSignature( const char* name, BmSignatureList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mAccessLock( (BmString("access_sig_") << name).Truncate(B_OS_NAME_LENGTH).String())
	,	mDynamic( false)
	,	mCharset( "utf-8")
{
}

/*------------------------------------------------------------------------------*\
	BmSignature( archive)
		-	c'tor
		-	constructs a BmSignature from a BMessage
\*------------------------------------------------------------------------------*/
BmSignature::BmSignature( BMessage* archive, BmSignatureList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
	,	mAccessLock( (BmString("access_sig_") 
						<< FindMsgString( archive, MSG_NAME)).Truncate(B_OS_NAME_LENGTH).String())
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mContent = FindMsgString( archive, MSG_CONTENT);
	mDynamic = FindMsgBool( archive, MSG_DYNAMIC);
	if (version > 1) {
		mCharset = FindMsgString( archive, MSG_CHARSET);
	} else {
		// map from old beos-number to new ibiconv-string:
		int16 encoding = FindMsgInt16( archive, "bm:encoding");
		mCharset = BmEncoding::ConvertFromBeosToLibiconv( encoding);
	}
	if (version < 3) {
		// from version 3, we use lowercase-charsets:
		mCharset.ToLower();
	}
}

/*------------------------------------------------------------------------------*\
	~BmSignature()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmSignature::~BmSignature() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSignature into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmSignature::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_CONTENT, mContent.String())
		||	archive->AddBool( MSG_DYNAMIC, mDynamic)
		||	archive->AddString( MSG_CHARSET, mCharset.String()));
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetSignatureString()
		-	returns the contents of this signature
		-	if this sig is dynamic (and thus takes its contents from the output of a 
			shell-command), the external command is executed, the results are fetched
			and returned
		-	always returns UTF8-encoded string
\*------------------------------------------------------------------------------*/
BmString BmSignature::GetSignatureString() {
	BmAutolockCheckGlobal lock( mAccessLock);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			BmString(Key()) << "-destructor: Unable to get access-lock"
		);
	if (!mContent.Length())
		return "";
	if (mDynamic) {
		
		BmString scriptFileName = TheTempFileList.NextTempFilenameWithPath();
		BFile scriptFile;
		status_t err = scriptFile.SetTo( scriptFileName.String(), 
													B_CREATE_FILE | B_WRITE_ONLY);
		if (err != B_OK) {
			BM_SHOWERR( BmString("Could not create temporary file\n\t<") << scriptFileName << ">\n\n Result: " << strerror(err));
			return "";
		}
		scriptFile.Write( mContent.String(), mContent.Length());
		scriptFile.Unset();
		BmString sigFileName = TheTempFileList.NextTempFilenameWithPath();
		BmString errFileName = TheTempFileList.NextTempFilenameWithPath();
		BmString sysStr = BmString("/bin/sh <")+scriptFileName+" >"<<sigFileName<<" 2>"<<errFileName;
		BM_LOG2( BM_LogApp, BmString("Dynamic signature, executing script: ")<<sysStr);
		int result = system( sysStr.String());
		BmString sigString;
		bool fileOk = FetchFile( sigFileName, sigString);
		if (!fileOk || result) {
			BmString error;
			FetchFile( errFileName, error);
			BM_SHOWERR( BmString("An error occurred, when trying to fetch dynamic signature <")
						   << Key() << ">\n\nError: " << error);
		} else if (!sigString.Length()) {
			BM_SHOWERR( BmString("There was an empty result, when trying to fetch dynamic signature <")
						   << Key() << ">\n");
		}
		TheTempFileList.RemoveFile( sigFileName);
		TheTempFileList.RemoveFile( errFileName);
		TheTempFileList.RemoveFile( scriptFileName);
		BmString utf8;
		ConvertToUTF8( mCharset, sigString, utf8);
		return utf8;
	} else
		return mContent;
}



/********************************************************************************\
	BmSignatureList
\********************************************************************************/

BmRef< BmSignatureList> BmSignatureList::theInstance( NULL);

const int16 BmSignatureList::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmSignatureList* BmSignatureList::CreateInstance() {
	if (!theInstance)
		theInstance = new BmSignatureList();
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmSignatureList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmSignatureList::BmSignatureList()
	:	inherited( "SignatureList", BM_LogApp) 
{
}

/*------------------------------------------------------------------------------*\
	~BmSignatureList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmSignatureList::~BmSignatureList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the signature-list
\*------------------------------------------------------------------------------*/
const BmString BmSignatureList::SettingsFileName() {
	return BmString( BeamRoster->SettingsPath()) << "/" << "Signatures";
}

/*------------------------------------------------------------------------------*\
	GetSignatureStringFor( sigName)
		-	returns the signature-string for the signature of the given name
		-	if no signature with given name exists, an empty string is returned
\*------------------------------------------------------------------------------*/
BmString BmSignatureList::GetSignatureStringFor( const BmString sigName) {
	BmRef<BmListModelItem> sigRef = FindItemByKey( sigName);
	BmSignature* sig = dynamic_cast< BmSignature*>( sigRef.Get());
	if (!sig)
		return "";
	return sig->GetSignatureString();
}

/*------------------------------------------------------------------------------*\
	InstantiateItem( archive)
		-	instantiates one signature from the given archive
\*------------------------------------------------------------------------------*/
void BmSignatureList::InstantiateItem( BMessage* archive) {
	BmSignature* newSig = new BmSignature( archive, this);
	BM_LOG3( BM_LogApp, BmString("Signature <") << newSig->Name() << "," << newSig->Key() << "> read");
	AddItemToList( newSig);
}

