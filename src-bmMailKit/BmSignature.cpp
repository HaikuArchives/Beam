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
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSignature
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmSignature()
		-	
\*------------------------------------------------------------------------------*/
BmSignature::BmSignature( const char* name, BmSignatureList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mDynamic( false)
	,	mEncoding( ThePrefs->GetInt("DefaultEncoding"))
{
}

/*------------------------------------------------------------------------------*\
	BmSignature( archive)
		-	constructs a BmSignature from a BMessage
\*------------------------------------------------------------------------------*/
BmSignature::BmSignature( BMessage* archive, BmSignatureList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mContent = FindMsgString( archive, MSG_CONTENT);
	mDynamic = FindMsgBool( archive, MSG_DYNAMIC);
	mEncoding = FindMsgInt16( archive, MSG_ENCODING);
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
		||	archive->AddInt16( MSG_ENCODING, mEncoding));
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetSignatureString()
		-	always returns UTF8
\*------------------------------------------------------------------------------*/
BString BmSignature::GetSignatureString() {
	if (!mContent.Length())
		return "";
	if (mDynamic) {
		BString sigFileName = TheTempFileList.NextTempFilenameWithPath();
		BString errFileName = TheTempFileList.NextTempFilenameWithPath();
		BString sysStr = mContent+" >"<<sigFileName<<" 2>"<<errFileName;
		BM_LOG2( BM_LogUtil, BString("Dynamic signature, executing script: ")<<sysStr);
		int result = system( sysStr.String());
		BString sigString;
		bool fileOk = FetchFile( sigFileName, sigString);
		if (!fileOk || result) {
			BString error;
			FetchFile( errFileName, error);
			BM_SHOWERR( BString("An error occurred, when trying to fetch dynamic signature <")
						   << Key() << ">\n\nError: " << error);
		} else if (!sigString.Length()) {
			BM_SHOWERR( BString("There was an empty result, when trying to fetch dynamic signature <")
						   << Key() << ">\n");
		}
		TheTempFileList.RemoveFile( sigFileName);
		TheTempFileList.RemoveFile( errFileName);
		BString utf8;
		ConvertToUTF8( mEncoding, sigString, utf8);
		return utf8;
	} else
		return mContent;
}



/********************************************************************************\
	BmSignatureList
\********************************************************************************/


BmRef< BmSignatureList> BmSignatureList::theInstance( NULL);

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
	:	inherited( "SignatureList") 
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
		-	
\*------------------------------------------------------------------------------*/
const BString BmSignatureList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Signatures";
}

/*------------------------------------------------------------------------------*\
	GetSignatureStringFor( sigName)
		-	
\*------------------------------------------------------------------------------*/
BString BmSignatureList::GetSignatureStringFor( const BString sigName) {
	BmRef<BmListModelItem> sigRef = FindItemByKey( sigName);
	BmSignature* sig = dynamic_cast< BmSignature*>( sigRef.Get());
	if (!sig)
		return "";
	return sig->GetSignatureString();
}

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmSignatureList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogUtil, BString("Start of InstantiateItems() for SignatureList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find signature nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmSignature* newSig = new BmSignature( &msg, this);
		BM_LOG3( BM_LogUtil, BString("Signature <") << newSig->Name() << "," << newSig->Key() << "> read");
		AddItemToList( newSig);
	}
	BM_LOG2( BM_LogUtil, BString("End of InstantiateItems() for SignatureList"));
	mInitCheck = B_OK;
}

