/*
	BmBodyPartList.cpp
		$Id$
*/

#include <Autolock.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

/********************************************************************************\
	BmContentField
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmContentField( ctString)
	-	c'tor
\*------------------------------------------------------------------------------*/
BmContentField::BmContentField( const BString cfString) {
	SetTo( cfString);
}

/*------------------------------------------------------------------------------*\
	SetTo( cfString)
	-	parses given content-field
\*------------------------------------------------------------------------------*/
void BmContentField::SetTo( const BString cfString) {
	Regexx rx;

	if (rx.exec( cfString, "^\\s*([^\\s;]+)\\s*(;.+)?\\s*$")) {
		// extract value:
		if (rx.match[0].atom.size() > 0) {
			if (cfString[rx.match[0].atom[0].start()] == '"') {
				// skip quotes during extraction:
				cfString.CopyInto( mValue, rx.match[0].atom[0].start()+1, rx.match[0].atom[0].Length()-2);
			} else {
				cfString.CopyInto( mValue, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
			}
		}
		mValue.ToLower();
		BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mValue);
		// parse and extract parameters:
		BString params;
		if (rx.match[0].atom.size() > 1)
			cfString.CopyInto( params, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		if (rx.exec( params, ";\\s*(\\w+)\\s*=\\s*((?:\\\"[^\"]+\\\"?)|(?:[^;\\s]+))", Regexx::global)) {
			for( uint32 i=0; i<rx.match.size(); ++i) {
				BString key;
				BString val;
				if (rx.match[0].atom.size() > 0) {
					params.CopyInto( key, rx.match[i].atom[0].start(), rx.match[i].atom[0].Length());
					if (rx.match[0].atom.size() > 1) {
						if (params[rx.match[i].atom[1].start()] == '"') {
							// skip quotes during extraction:
							int skip = params[rx.match[i].atom[1].start()+rx.match[i].atom[1].Length()-1] == '"'
											? 2 : 1;
							params.CopyInto( val, rx.match[i].atom[1].start()+1, rx.match[i].atom[1].Length()-skip);
						} else {
							params.CopyInto( val, rx.match[i].atom[1].start(), rx.match[i].atom[1].Length());
						}
					}
					mParams[key.ToLower()] = val;
					BM_LOG2( BM_LogMailParse, BString("...found param: ")<<key<<" with value: "<<val);
				}
			}
		}
	} else {
		BM_SHOWERR( BString("field-value <")<<cfString<<"> has unknown structure!");
		return;
	}
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	Param( key)
	-	returns parameter-value for given key (or empty string)
\*------------------------------------------------------------------------------*/
const BString& BmContentField::Param( BString key) const {
	static BString nullStr;
	key.ToLower();
	BmParamMap::const_iterator iter = mParams.find( key);
	if (iter != mParams.end()) {
		return iter->second;
	} else
		return nullStr;
}


/********************************************************************************\
	BmBodyPart
\********************************************************************************/

int32 BmBodyPart::nCounter = 0;

/*------------------------------------------------------------------------------*\
	BmBodyPart()
	-	default c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, BmListModelItem* parent)
	:	inherited( BString("")<<++nCounter, model, parent)
	,	mIsMultiPart( false)
	,	mPosInRawText( NULL)
	,	mLength( 0)
	,	mDecodedLength( 0)
{
}

/*------------------------------------------------------------------------------*\
	BmBodyPart( msgtext, start, length, contentType)
	-	c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, const BString& msgtext, int32 start, 
								int32 length, BmMailHeader* header, BmListModelItem* parent)
	:	inherited( BString("")<<++nCounter, model, parent)
	,	mIsMultiPart( false)
	,	mPosInRawText( NULL)
	,	mLength( 0)
	,	mDecodedLength( 0)
{
	SetTo( msgtext, start, length, header);
}

/*------------------------------------------------------------------------------*\
	SetTo( msgtext, start, length, contentType)
	-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SetTo( const BString& msgtext, int32 start, int32 length, 
								BmMailHeader* header)
{
	BString type;
	BString encoding;
	BString id;
	BString disposition;
	BString description;
	BString language;
	bool deleteHeader = false;

	if (!header) {
		// this is not the main body, so we have to split the MIME-headers from
		// the MIME-bodypart:
		BString headerText;
		int32 pos = msgtext.FindFirst( "\r\n\r\n", start);
		if (pos == B_ERROR) {
			BString str;
			msgtext.CopyInto( str, start, 256);
			BM_SHOWERR( BString("Couldn't determine borderline between MIME-header and body in string <")<<str<<">.");
			return;
		}
		msgtext.CopyInto( headerText, start, pos-start+2);
		mPosInRawText = msgtext.String()+pos+4;
		mLength = length - (pos+4-start);
		BM_LOG2( BM_LogMailParse, BString("MIME-Header found: ") << headerText);
		header = new BmMailHeader( headerText, NULL);
		deleteHeader = true;
	} else {
		mPosInRawText = msgtext.String()+start;
		mLength = length;
	}
	// MIME-type
	BM_LOG2( BM_LogMailParse, "parsing Content-Type");
	type = header->GetFieldVal("Content-Type");
	if (!type.Length() || type.ICompare("text")==0)
		type = "text/plain; charset=us-ascii";
	mContentType.SetTo( type);
	// encoding
	BM_LOG2( BM_LogMailParse, "parsing Content-Transfer-Encoding");
	encoding = header->GetFieldVal("Content-Transfer-Encoding");
	if (!encoding.Length())
		encoding = "7bit";
	mContentTransferEncoding = encoding;
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentTransferEncoding);
	// decoded length
	if (!mIsMultiPart) {
		// unneccessary for multiparts, since they are never handled on their own 
		// (they are split into their subparts instead)
		if (ThePrefs->ShowDecodedLength() && mContentTransferEncoding.Length())
			mDecodedLength = BmEncoding::DecodedLength( mContentTransferEncoding, 
																	  mPosInRawText, mLength);
		else
			mDecodedLength = mLength;
	}
	// id
	BM_LOG2( BM_LogMailParse, "parsing Content-Id");
	mContentId = header->GetFieldVal("Content-Id");
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentId);
	// disposition
	BM_LOG2( BM_LogMailParse, "parsing Content-Disposition");
	disposition = header->GetFieldVal("Content-Disposition");
	if (!disposition.Length())
		disposition = (IsPlainText() ? "inline" : "attachment");
	mContentDisposition.SetTo( disposition);
	// description
	BM_LOG2( BM_LogMailParse, "parsing Content-Description");
	mContentDescription = header->GetFieldVal("Content-Description");
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentDescription);
	// Language
	BM_LOG2( BM_LogMailParse, "parsing Content-Language");
	mContentLanguage = header->GetFieldVal("Content-Language");
	mContentLanguage.ToLower();
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentLanguage);
	// determine a filename (if possible)
	mFileName = mContentDisposition.Param("filename");
	if (!mFileName.Length()) {
		mFileName = mContentType.Param("name");
		if (!mFileName.Length()) {
			mFileName = TheTempFileList.NextTempFilename();
		}
	}
	// remove temporary header:
	if (deleteHeader)
		delete header;
		
	if (type.ICompare("multipart", 9) == 0) {
		mIsMultiPart = true;
		BString boundary = BString("--")+mContentType.Param("boundary");
		if (boundary.Length()==2) {
			BM_SHOWERR( "No boundary specified within multipart-message!");
			return;
		}
		int32 startPos = int32(strstr( mPosInRawText, boundary.String())-msgtext.String());
		if (startPos == B_ERROR) {
			BM_SHOWERR( BString("Boundary <")<<boundary<<"> not found within message.");
			return;
		}
		char* nPos;
		int32 count = 0;
		for( ; (nPos = strstr( msgtext.String()+startPos+boundary.Length(), boundary.String())); ++count) {
			int32 nextPos = nPos - msgtext.String();
			int32 sPos = startPos+boundary.Length()+2;
			BM_LOG2( BM_LogMailParse, "Subpart of multipart found will be added to array");
			BmBodyPart *subPart = new BmBodyPart( (BmBodyPartList*)ListModel(), msgtext, sPos, nextPos-sPos, NULL, this);
			AddSubItem( subPart);
			startPos = nextPos;
		}
		if (!count) {
			BM_SHOWERR( BString("Boundary <")<<boundary<<"> not found within message.");
			return;
		}
	}
}

/*------------------------------------------------------------------------------*\
	IsText()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::IsText() const {
	return MimeType().ICompare("text/",5) == 0;
}

/*------------------------------------------------------------------------------*\
	IsPlainText()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::IsPlainText() const {
	return MimeType().ICompare("text/plain") == 0;
}

/*------------------------------------------------------------------------------*\
	IsText()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::ShouldBeShownInline()	const {
	return (IsPlainText() && Disposition().ICompare("inline")==0);
}

/*------------------------------------------------------------------------------*\
	DecodedTextData( msgtext, start, length, contentType)
	-	
\*------------------------------------------------------------------------------*/
void* BmBodyPart::DecodedData( int32* dataLen) const {
	BString buf( mPosInRawText, mLength);
	if (mContentTransferEncoding.Length()) {
		int32 decodedSize = 0;
		void* decodedData = Decode( mContentTransferEncoding, buf, false, IsText(), decodedSize);
		if (dataLen)
			*dataLen = decodedSize;
		return decodedData;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	WriteToTempFile()
		-	
\*------------------------------------------------------------------------------*/
entry_ref BmBodyPart::WriteToTempFile( BString filename) {
	BPath tempPath;
	entry_ref eref;
	if (!filename.Length()) {
		filename = mFileName;
	}
	if (find_directory( B_COMMON_TEMP_DIRECTORY, &tempPath, true) == B_OK) {
		BDirectory tempDir;
		BFile tempFile;
		BNodeInfo fileInfo;
		status_t err;
		tempDir.SetTo( tempPath.Path());
		if ((err = tempFile.SetTo( &tempDir, filename.String(), 
										  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
			BM_LOGERR( BString("Could not create temporary file\n\t<") << filename << ">\n\n Result: " << strerror(err));
			return eref;
		}
		TheTempFileList.AddFile( BString(tempPath.Path())<<"/"<<filename);
		int32 dataLen;
		void* data = DecodedData( &dataLen);
		tempFile.Write( data, dataLen);
		free( data);
		fileInfo.SetTo( &tempFile);
		fileInfo.SetType( MimeType().String());
		BEntry entry( &tempDir, filename.String());
		entry.GetRef( &eref);
	}
	return eref;
}


/********************************************************************************\
	BmBodyPartList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmBodyPartList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmBodyPartList::BmBodyPartList( BmMail* mail)
	:	inherited( BString("BodyPartList_") << mail->ModelName())
	,	mMail( mail)
	,	mInitCheck( B_NO_INIT)
{
}

/*------------------------------------------------------------------------------*\
	~BmBodyPartList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmBodyPartList::~BmBodyPartList() {
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::StartJob() {
	// try to parse bodypart-structure of mail...
	
	if (InitCheck() == B_OK) {
		return true;
	}

	Freeze();									// we shut up for better performance
	try {
		if (mMail) {
			const BString& msgText = mMail->RawText();
			BmBodyPart* bodyPart = new BmBodyPart( this, msgText, mMail->HeaderLength()+2, 
																msgText.Length()-mMail->HeaderLength()-2, 
																mMail->Header());
			AddItemToList( bodyPart);
			mMail = NULL;
		}
		mInitCheck = B_OK;
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	Thaw();
	return InitCheck() == B_OK;
}
