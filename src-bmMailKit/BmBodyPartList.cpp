/*
	BmBodyPartList.cpp
		$Id$
*/

#include <Autolock.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include "regexx.hh"
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

	if (rx.exec( cfString, "^\\s*([^\\s;]+)\\s*(;.*)?\\s*$")) {
		// extract value:
		if (rx.match[0].atom.size() > 0) {
			if (cfString[rx.match[0].atom[0].start()] == '"') {
				// skip quotes during extraction:
				cfString.CopyInto( mValue, rx.match[0].atom[0].start()+1, rx.match[0].atom[0].Length()-2);
			} else {
				mValue = rx.match[0].atom[0];
			}
		}
		mValue.ToLower();
		BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mValue);
		// parse and extract parameters:
		BString params;
		if (rx.match[0].atom.size() > 1)
			params = rx.match[0].atom[1];
		if (rx.exec( params, ";\\s*(\\w+)\\s*=\\s*((?:\\\"[^\"]+\\\"?)|(?:[^;\\s]+))", Regexx::global)) {
			for( uint32 i=0; i<rx.match.size(); ++i) {
				BString key;
				BString val;
				if (rx.match[0].atom.size() > 0) {
					key = rx.match[i].atom[0];
					if (rx.match[0].atom.size() > 1) {
						if (params[rx.match[i].atom[1].start()] == '"') {
							// skip quotes during extraction:
							int skip = params[rx.match[i].atom[1].start()+rx.match[i].atom[1].Length()-1] == '"'
											? 2 : 1;
							params.CopyInto( val, rx.match[i].atom[1].start()+1, rx.match[i].atom[1].Length()-skip);
						} else {
							val = rx.match[i].atom[1];
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

/*------------------------------------------------------------------------------*\
	Param( key)
	-	sets the parameter to the given value:
\*------------------------------------------------------------------------------*/
void BmContentField::SetParam( BString key, BString value) {
	mParams[key.ToLower()] = value;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmContentField::operator BString() const {
	BString fieldString;
	fieldString << mValue;

	BmParamMap::const_iterator iter;
	if (!mParams.empty()) {
		fieldString << ";";	
		for( iter = mParams.begin(); iter != mParams.end(); ++iter) {
			fieldString << " " << iter->first << "=\"" << iter->second << '"';
		}
	}
	return fieldString;
}



/********************************************************************************\
	BmBodyPart
\********************************************************************************/

int32 BmBodyPart::nBoundaryCounter = 0;
int32 BmBodyPart::nObjectID = 0;

/*------------------------------------------------------------------------------*\
	BmBodyPart( msgtext, start, length, contentType)
	-	c'tor
\*------------------------------------------------------------------------------*/
BString BmBodyPart::NextObjectID() {
	BString s;
	char* buf=s.LockBuffer( 20);
	sprintf( buf, "%5ld", ++nObjectID);
	s.UnlockBuffer( -1);
	return s;
}

/*------------------------------------------------------------------------------*\
	BmBodyPart( msgtext, start, length, contentType)
	-	c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, const BString& msgtext, int32 start, 
								int32 length, BmRef<BmMailHeader> header, BmListModelItem* parent)
	:	inherited( BString("")<<NextObjectID(), model, parent)
	,	mIsMultiPart( false)
	,	mInitCheck( B_NO_INIT)
	,	mEntryRef()
{
	SetTo( msgtext, start, length, header);
}

/*------------------------------------------------------------------------------*\
	BmBodyPart()
	-	c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, const entry_ref* ref, BmListModelItem* parent)
	:	inherited( BString("")<<NextObjectID(), model, parent)
	,	mIsMultiPart( false)
	,	mInitCheck( B_NO_INIT)
	,	mEntryRef( *ref)
{
	try {
		status_t err;
		BFile file;
		BNodeInfo nodeInfo;
		off_t size;
		char mimetype[B_MIME_TYPE_LENGTH+1];
		(err=file.SetTo( ref, B_READ_ONLY)) == B_OK		
														|| BM_THROW_RUNTIME( BString("Couldn't create file for <") << ref->name << "> \n\nError:" << strerror(err));
		(err=file.GetSize( &size)) == B_OK
														|| BM_THROW_RUNTIME( BString("Couldn't get file-size for <") << ref->name << "> \n\nError:" << strerror(err));
		char* buf = mDecodedData.LockBuffer( size+1);
		file.Read( buf, size);
		buf[size] = '\0';
		mDecodedData.UnlockBuffer( size);
		
		(err=nodeInfo.SetTo( &file)) == B_OK
														|| BM_THROW_RUNTIME( BString("Couldn't create node-info for <") << ref->name << "> \n\nError:" << strerror(err));
		nodeInfo.GetType( mimetype);
		mFileName = ref->name;
		mContentType.SetTo( BString(mimetype)<<"; name=\"" << ref->name << '"');
		mContentDisposition.SetTo( BString( "attachment; filename=\"")<<ref->name<<'"');
		mContentTransferEncoding = BString(mimetype).ICompare( "text", 4) ? "base64" : "quoted-printable";
		mInitCheck = B_OK;
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BodyPart:\n\t") << err.what());
	}
}
	
/*------------------------------------------------------------------------------*\
	~BmBodyPart()
	-	d'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::~BmBodyPart() {
}

/*------------------------------------------------------------------------------*\
	SetTo( msgtext, start, length, contentType)
	-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SetTo( const BString& msgtext, int32 start, int32 length, 
								BmRef<BmMailHeader> header) {
	BString type;
	BString transferEncoding;
	BString id;
	BString disposition;
	BString description;
	BString language;
	const char* posInRawText = NULL;
	int32 bodyLength;
 	BmRef<BmListModel> bodyRef = mListModel.Get();
 	BmBodyPartList* body = dynamic_cast< BmBodyPartList*>( bodyRef.Get());

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
		posInRawText = msgtext.String()+pos+4;
		bodyLength = length - (pos+4-start);
		BM_LOG2( BM_LogMailParse, BString("MIME-Header found: ") << headerText);
		header = new BmMailHeader( headerText, NULL);
	} else {
		posInRawText = msgtext.String()+start;
		bodyLength = length;
	}
	// MIME-type
	BM_LOG2( BM_LogMailParse, "parsing Content-Type");
	type = header->GetFieldVal( BM_FIELD_CONTENT_TYPE);
	if (!type.Length() || type.ICompare("text")==0) {
		// set content-type to default if is empty or contains "text"
		// (which is illegal but used by some broken mail-clients, it seems...)
		type = "text/plain; charset=us-ascii";
	}
	mContentType.SetTo( type);
	if (type.ICompare("multipart", 9) == 0) {
		mIsMultiPart = true;
	}
	if (IsPlainText() && !body->EditableTextBody()) {
		body->EditableTextBody( this);
	}
	// transferEncoding
	BM_LOG2( BM_LogMailParse, "parsing Content-Transfer-Encoding");
	transferEncoding = header->GetFieldVal( BM_FIELD_CONTENT_TRANSFER_ENCODING);
	transferEncoding.RemoveSet( TheResources->WHITESPACE);
							// some broken (webmail)-clients produce stuff like "7 bit"...
	if (!transferEncoding.Length())
		transferEncoding = "7bit";
	mContentTransferEncoding = transferEncoding;
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentTransferEncoding);

	// MIME-Decoding:
	if (mIsMultiPart) {
		// decoding is unneccessary for multiparts, since they are never handled on 
		// their own (they are split into their subparts instead)
	} else {
		// decode body:
		BString bodyString( posInRawText, bodyLength);
		Decode( mContentTransferEncoding, bodyString, mDecodedData, false, IsText());
		if (body->EditableTextBody() == this) {
			// split off signature, if any:
			Regexx rx;
			for( int32 sigPos=0; 
				  (sigPos = mDecodedData.FindFirst( "\n--", sigPos)) != B_ERROR; 
				  ++sigPos) {
				sigPos++;
				BString sigStr( mDecodedData.String()+sigPos);
				BString sigRX = ThePrefs->GetString( "SignatureRX", "\\A---?\\s*\\n");
				if (rx.exec( sigStr, sigRX, Regexx::newline)) {
					sigStr.Remove( 0, rx.match[0].Length());
					body->Signature( sigStr);
					mDecodedData.Truncate( sigPos);
					break;
				}
			}
		}
	}
	// id
	BM_LOG2( BM_LogMailParse, "parsing Content-Id");
	mContentId = header->GetFieldVal( BM_FIELD_CONTENT_ID);
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentId);
	// disposition
	BM_LOG2( BM_LogMailParse, "parsing Content-Disposition");
	disposition = header->GetFieldVal( BM_FIELD_CONTENT_DISPOSITION);
	if (!disposition.Length())
		disposition = (IsPlainText() ? "inline" : "attachment");
	mContentDisposition.SetTo( disposition);
	// description
	BM_LOG2( BM_LogMailParse, "parsing Content-Description");
	mContentDescription = header->GetFieldVal( BM_FIELD_CONTENT_DESCRIPTION);
	BM_LOG2( BM_LogMailParse, BString("...found value: ")<<mContentDescription);
	// Language
	BM_LOG2( BM_LogMailParse, "parsing Content-Language");
	mContentLanguage = header->GetFieldVal( BM_FIELD_CONTENT_LANGUAGE);
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
		
	if (mIsMultiPart) {
		BString boundary = BString("--")+mContentType.Param("boundary");
		if (boundary.Length()==2) {
			BM_SHOWERR( "No boundary specified within multipart-message!");
			return;
		}
		int32 startPos = int32(strstr( posInRawText, boundary.String())-msgtext.String());
		if (startPos == B_ERROR) {
			BM_SHOWERR( BString("Boundary <")<<boundary<<"> not found within message.");
			return;
		}
		char* nPos;
		int32 count = 0;
		for( ; (nPos = strstr( msgtext.String()+startPos+boundary.Length(), boundary.String())); ++count) {
			int32 nextPos = nPos - msgtext.String();
//			int32 sPos = startPos+boundary.Length()+2;
			int32 sPos = startPos+boundary.Length();
			BM_LOG2( BM_LogMailParse, "Subpart of multipart found will be added to array");
			BmBodyPart *subPart = new BmBodyPart( (BmBodyPartList*)ListModel().Get(), msgtext, sPos, nextPos-sPos, NULL, this);
			AddSubItem( subPart);
			startPos = nextPos;
		}
		if (!count) {
			BM_SHOWERR( BString("Boundary <")<<boundary<<"> not found within message.");
			return;
		}
	}
	mInitCheck = B_OK;
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
	ContainsRef()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::ContainsRef( const entry_ref& ref) const {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); iter++) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
		if (bodyPart->EntryRef() == ref)
			return true;
	}
	return false;
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
		tempFile.Write( mDecodedData.String(), mDecodedData.Length());
		fileInfo.SetTo( &tempFile);
		fileInfo.SetType( MimeType().String());
		BEntry entry( &tempDir, filename.String());
		entry.GetRef( &eref);
	}
	return eref;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SetBodyText( const BString& text, uint32 encoding) {
	text.CopyInto( mDecodedData, 0, text.Length());
	mContentType.SetParam( "charset", EncodingToCharset( encoding));
	mContentTransferEncoding = NeedsEncoding( text) ? "quoted-printable" : "7bit";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmBodyPart::GenerateBoundary() {
	return BString("_=_BOUNDARY_")<<system_time()<<"_"<<++nBoundaryCounter<<"_=_";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::PropagateHigherEncoding() {
	if (!IsMultiPart())
		return;
	BString newTransferEncoding = "7-bit";
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmBodyPart* subPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
		subPart->PropagateHigherEncoding();
		if ( subPart->IsBinary()) {
			mContentTransferEncoding = "binary";
			return;
		}
		if ( subPart->Is8Bit())
			mContentTransferEncoding = "8bit";
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::ConstructBodyForSending( BString &msgText) {
	BString boundary;
	if (IsMultiPart()) {
		PropagateHigherEncoding();
		boundary = GenerateBoundary();
		mContentType.SetParam( "boundary", boundary);
	}
	msgText << BM_FIELD_CONTENT_TYPE << ": " << mContentType << "\r\n";
	msgText << BM_FIELD_CONTENT_TRANSFER_ENCODING << ": " << mContentTransferEncoding << "\r\n";
	msgText << BM_FIELD_CONTENT_DISPOSITION << ": " << mContentDisposition << "\r\n";
	if (mContentDescription.Length())
		msgText << BM_FIELD_CONTENT_DESCRIPTION << ": " << mContentDescription << "\r\n";
	if (mContentId.Length())
		msgText << BM_FIELD_CONTENT_ID << ": " << mContentId << "\r\n";
	if (mContentLanguage.Length())
		msgText << BM_FIELD_CONTENT_LANGUAGE << ": " << mContentLanguage << "\r\n";
	msgText << "\r\n";
	BString encodedData;
	BmEncoding::Encode( mContentTransferEncoding, mDecodedData, encodedData);
	msgText << encodedData;
	if (msgText[msgText.Length()-1] != '\n')
		msgText << "\r\n";
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		if (IsMultiPart())
			msgText << "--"<<boundary<<"\r\n";
		BmBodyPart* subPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
		subPart->ConstructBodyForSending( msgText);
	}
	if (IsMultiPart())
		msgText << "--"<<boundary<<"--\r\n";
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
	,	mEditableTextBody( NULL)
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
	ParseMail()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::ParseMail() {
	mEditableTextBody = NULL;
	Cleanup();
	if (mMail) {
		const BString& msgText = mMail->RawText();
		BmBodyPart* bodyPart = new BmBodyPart( this, msgText, mMail->HeaderLength()+2, 
															msgText.Length()-mMail->HeaderLength()-2, 
															mMail->Header());
		AddItemToList( bodyPart);
	}
	mInitCheck = B_OK;
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
		ParseMail();
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	Thaw();
	return InitCheck() == B_OK;
}

/*------------------------------------------------------------------------------*\
	HasAttachments()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::HasAttachments() const {
	if (empty())
		return false;
	BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( begin()->second.Get());
	return bodyPart ? bodyPart->IsMultiPart() : false;
}

/*------------------------------------------------------------------------------*\
	AddAttachmentFromRef()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::AddAttachmentFromRef( const entry_ref* ref) {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	BmBodyPart* parent = dynamic_cast< BmBodyPart*>( editableTextBody 
																		? editableTextBody->Parent()
																		: (BmBodyPart*)NULL);
	bool alreadyPresent = false;
	if (parent)
		alreadyPresent = parent->ContainsRef( *ref);
	else {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); !alreadyPresent && iter != end(); iter++) {
			BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
			if (bodyPart->EntryRef() == *ref)
				alreadyPresent = true;
		}
	}
	if (!alreadyPresent) {
		BmBodyPart* bodyPart = new BmBodyPart( this, ref, parent);
		if (bodyPart->InitCheck() == B_OK) {
			AddItemToList( bodyPart, parent);
		} else
			delete bodyPart;
	}
}

/*------------------------------------------------------------------------------*\
	SetEditableText()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::SetEditableText( const BString& text, uint32 encoding) {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	if (editableTextBody)
		editableTextBody->SetBodyText( text, encoding);
}

/*------------------------------------------------------------------------------*\
	DefaultEncoding()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmBodyPartList::DefaultEncoding() const {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	if (editableTextBody)
		return CharsetToEncoding( editableTextBody->TypeParam( "charset"));
	else 
		return BM_UNKNOWN_ENCODING; 
}

/*------------------------------------------------------------------------------*\
	IsMultiPart()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::IsMultiPart() const {
	if (size()) {
		BmBodyPart* firstBody = dynamic_cast<BmBodyPart*>( begin()->second.Get());
		if (firstBody)
			return firstBody->IsMultiPart();
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::ConstructBodyForSending( BString& msgText) {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	bool hasMultiPartTop = editableTextBody && editableTextBody->Parent();
	bool isMultiPart = hasMultiPartTop
								? editableTextBody->Parent()->size() > 1
								: size() > 1;
	BString boundary;
	if (isMultiPart && !hasMultiPartTop) {
		boundary = BmBodyPart::GenerateBoundary();
		msgText << BM_FIELD_CONTENT_TYPE << ": multipart/mixed; boundary=\""<<boundary<<"\"\r\n";
		msgText << BM_FIELD_CONTENT_TRANSFER_ENCODING << ": 7bit\r\n\r\n";
		msgText << "This is a multi-part message in MIME format.\r\n\r\n";
		msgText << "--"<<boundary<<"\r\n";
	}
	BmModelItemMap::const_iterator iter;
	if (hasMultiPartTop && !isMultiPart) {
		// The mail thinks it is a multipart-mail, while in fact it isn't (any more).
		// This means that the user has removed attachments from the mail, so that
		// now there is no need for the multipart at the top, so we skip it:
		BmRef<BmBodyPart> editableTextBody( EditableTextBody());
		BmBodyPart* topPart( dynamic_cast< BmBodyPart*>( editableTextBody->Parent()));
		for( iter = topPart->begin(); iter != topPart->end(); ++iter) {
			BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
			bodyPart->ConstructBodyForSending( msgText);
		}
	} else {
		// The mail is either a simple text-mail or it is/should be a multipart-mail:
		for( iter = begin(); iter != end(); ++iter) {
			BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
			bodyPart->ConstructBodyForSending( msgText);
			if (isMultiPart && !hasMultiPartTop) {
				BmModelItemMap::const_iterator next = iter;
				next++;
				if (next == end())
					msgText << "--"<<boundary<<"--\r\n";
				else
					msgText << "--"<<boundary<<"\r\n";
			}
		}
	}
	return true;
}
