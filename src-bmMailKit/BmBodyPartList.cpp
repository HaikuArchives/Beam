/*
	BmBodyPartList.cpp
		$Id$
*/

#include <Autolock.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmBodyPartList.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

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
		if (rx.exec( params, ";\\s*(\\w+)\\s*=\\s*((?:\\\"[^\"]+\\\")|(?:[\\S]+))", Regexx::global)) {
			for( uint32 i=0; i<rx.match.size(); ++i) {
				BString key;
				BString val;
				if (rx.match[0].atom.size() > 0) {
					params.CopyInto( key, rx.match[i].atom[0].start(), rx.match[i].atom[0].Length());
					if (rx.match[0].atom.size() > 1) {
						if (params[rx.match[i].atom[1].start()] == '"') {
							// skip quotes during extraction:
							params.CopyInto( val, rx.match[i].atom[1].start()+1, rx.match[i].atom[1].Length()-2);
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



/********************************************************************************\
	BmBodyPart
\********************************************************************************/

int32 BmBodyPart::nCounter = 0;

/*------------------------------------------------------------------------------*\
	BmBodyPart()
	-	default c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmListModelItem* parent)
	:	inherited( BString("")<<++nCounter, parent)
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
BmBodyPart::BmBodyPart( const BString& msgtext, int32 start, int32 length, 
								BmMailHeader* header, BmListModelItem* parent)
	:	inherited( BString("")<<++nCounter, parent)
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
		if (mContentTransferEncoding.Length())
			mDecodedLength = BmEncoding::DecodedLength( mContentTransferEncoding, 
																	  mPosInRawText, mLength);
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
	mFileName = mContentDisposition.mParams["filename"];
	if (!mFileName.Length()) {
		mFileName = mContentType.mParams["name"];
	}
	// remove temporary header:
	if (deleteHeader)
		delete header;
		
	if (type.ICompare("multipart", 9) == 0) {
		mIsMultiPart = true;
		BString boundary = BString("--")+mContentType.mParams["boundary"];
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
			BmBodyPart *subPart = new BmBodyPart( msgtext, sPos, nextPos-sPos, NULL, this);
			mSubItemMap[subPart->Key()] = subPart;
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
	return mContentType.mValue.ICompare("text/",5) == 0;
}

/*------------------------------------------------------------------------------*\
	IsPlainText()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::IsPlainText() const {
	return mContentType.mValue.ICompare("text/plain") == 0;
}

/*------------------------------------------------------------------------------*\
	IsText()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::ShouldBeShownInline()	const {
	return (IsPlainText() && mContentDisposition.mValue.ICompare("inline")==0);
}

/*------------------------------------------------------------------------------*\
	DecodedData( msgtext, start, length, contentType)
	-	
\*------------------------------------------------------------------------------*/
BString BmBodyPart::DecodedData() const {
	BString buf( mPosInRawText, mLength);
	if (mContentTransferEncoding.Length())
		BmEncoding::Decode( mContentTransferEncoding, buf, false, IsText());
	if (buf.Length() && buf[buf.Length()-1] != '\n')
		buf << "\r\n";
	return buf;
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
void BmBodyPartList::StartJob() {
	// try to parse bodypart-structure of mail...
	
	try {
		if (InitCheck() == B_OK) {
			return;
		}
		if (mMail) {
			const BString& msgText = mMail->RawText();
			BmBodyPart* bodyPart = new BmBodyPart( msgText, mMail->HeaderLength()+2, 
																msgText.Length()-mMail->HeaderLength()-2, 
																mMail->Header());
			mModelItemMap[bodyPart->Key()] = bodyPart;
			mMail = NULL;
		}
		mInitCheck = B_OK;
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
}

