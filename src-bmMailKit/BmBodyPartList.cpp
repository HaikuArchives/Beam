/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Application.h>
#include <Invoker.h>

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include "regexx.hh"
#include "split.hh"
using namespace regexx;


#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
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
BmContentField::BmContentField( const BmString cfString) {
	SetTo( cfString);
}

/*------------------------------------------------------------------------------*\
	SetTo( cfString)
	-	parses given content-field
\*------------------------------------------------------------------------------*/
void BmContentField::SetTo( const BmString cfString) {
	Regexx rx;

	if (rx.exec( cfString, "^\\s*([^\\s;]+)\\s*([;\\s].*)?\\s*$", 
					 Regexx::newline)) {
		// extract value:
		if (rx.match[0].atom.size() > 0) {
			if (cfString[rx.match[0].atom[0].start()] == '"') {
				// skip quotes during extraction:
				cfString.CopyInto( mValue, rx.match[0].atom[0].start()+1, 
										 rx.match[0].atom[0].Length()-2);
			} else {
				mValue = rx.match[0].atom[0];
			}
		}
		mValue.ToLower();
		BM_LOG2( BM_LogMailParse, BmString("...found value: ")<<mValue);
		// parse and extract parameters:
		BmString params;
		if (rx.match[0].atom.size() > 1)
			params = rx.match[0].atom[1];
		if (rx.exec( params, 
						 ";?\\s*(\\w+)\\s*=\\s*((?:\\\"[^\"]+\\\"?)|(?:[^;\\s]+))", 
						 Regexx::global)) {
			for( uint32 i=0; i<rx.match.size(); ++i) {
				BmString key;
				BmString val;
				if (rx.match[0].atom.size() > 0) {
					key = rx.match[i].atom[0];
					if (rx.match[0].atom.size() > 1) {
						if (params[rx.match[i].atom[1].start()] == '"') {
							// skip quotes during extraction:
							int skip = params[rx.match[i].atom[1].start()
													+rx.match[i].atom[1].Length()-1] == '"'
											? 2 : 1;
							params.CopyInto( val, rx.match[i].atom[1].start()+1, 
												  rx.match[i].atom[1].Length()-skip);
						} else {
							val = rx.match[i].atom[1];
						}
					}
					key.ToLower();
					mParams[key] = val;
					BM_LOG2( BM_LogMailParse, 
								BmString("...found param: ")<<key
									<<" with value: "<<val);
				}
			}
		}
	} else {
		BM_LOG(BM_LogMailParse, BmString("field-value <")<<cfString
							<<"> has unknown structure!");
		return;
	}
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	Param( key)
	-	returns parameter-value for given key (or empty string)
\*------------------------------------------------------------------------------*/
const BmString& BmContentField::Param( BmString key) const {
	static BmString nullStr;
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
void BmContentField::SetParam( BmString key, BmString value) {
	key.ToLower();
	mParams[key] = value;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmContentField::operator BmString() const {
	BmString fieldString;
	fieldString << mValue;

	BmParamMap::const_iterator iter;
	if (!mParams.empty()) {
		for( iter = mParams.begin(); iter != mParams.end(); ++iter) {
			fieldString << ";\r\n "
							<< iter->first 
							<< "=\"" 
							<< iter->second
							<< '"';
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
BmString BmBodyPart::NextObjectID() {
	BmString s;
	char* buf=s.LockBuffer( 5);
	sprintf( buf, "%5.5ld", ++nObjectID);
	s.UnlockBuffer( 5);
	return s;
}

/*------------------------------------------------------------------------------*\
	BmBodyPart( msgtext, start, length, contentType)
	-	c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, const BmString& msgtext, 
								int32 start, int32 length, 
								const BmString& defaultCharset,
								BmRef<BmMailHeader> header, BmListModelItem* parent)
	:	inherited( BmString("")<<NextObjectID(), model, parent)
	,	mIsMultiPart( false)
	,	mInitCheck( B_NO_INIT)
	,	mEntryRef()
	// info about mailtext will be overwritten in SetTo(), but we initialize 
	// with safe values:
	,	mStartInRawText( 0)
	,	mBodyLength( 0)
	,	mHaveDecodedData( false)
	,	mSuggestedCharset( defaultCharset)
	,	mCurrentCharset( defaultCharset)
	, 	mHadErrorDuringConversion( false)
{
	if (!mSuggestedCharset.Length())
		mCurrentCharset = mSuggestedCharset 
			= ThePrefs->GetBool( "ImportExportTextAsUtf8", true)
				? BmString("utf-8")
				: ThePrefs->GetString( "DefaultCharset");
	SetTo( msgtext, start, length, defaultCharset, header);
}

/*------------------------------------------------------------------------------*\
	BmBodyPart()
	-	c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( BmBodyPartList* model, const entry_ref* ref, 
								const BmString& defaultCharset,
								BmListModelItem* parent)
	:	inherited( BmString("")<<NextObjectID(), model, parent)
	,	mIsMultiPart( false)
	,	mInitCheck( B_NO_INIT)
	,	mEntryRef( *ref)
	,	mStartInRawText( 0)
	// we can't store info about mailtext, since there is no mailtext available:
	,	mBodyLength( 0)
	,	mHaveDecodedData( false)
	,	mSuggestedCharset( defaultCharset)
	,	mCurrentCharset( defaultCharset)
	, 	mHadErrorDuringConversion( false)
{
	if (!mSuggestedCharset.Length())
		mCurrentCharset = mSuggestedCharset 
			= ThePrefs->GetBool( "ImportExportTextAsUtf8", true)
				? BmString("utf-8")
				: ThePrefs->GetString( "DefaultCharset");
	try {
		BmString mimetype = DetermineMimeType( ref, false);
		if (mimetype.ICompare("text/x-email")==0) {
			// convert beos-own mail-mimetype into correct message/rfc822:
			mimetype = "message/rfc822";
		}
		mFileName = ref->name;
		mContentDisposition.SetTo( BmString( "attachment; filename=\"")
												<<ref->name<<'"');

		BmString filepath = BPath(ref).Path();
		if (mimetype.ICompare( "text/", 5)==0 
		&& !ThePrefs->GetBool( "ImportExportTextAsUtf8", true)) {
			BmString nativeString;
			FetchFile(filepath, nativeString);
			if (!IsCompatibleWithText( nativeString)) {
				BM_SHOWERR( "The attachment has been advertised as text, "
								"although it seems to contain binary data.\n\n"
								"The attachment will be added as generic file, "
								"just to be safe.");
				mimetype="application/octet-stream";
			} else {
				ConvertToUTF8( mCurrentCharset, nativeString, mDecodedData);
				mHaveDecodedData = true;
			}
		}
		if (!mHaveDecodedData) {
			FetchFile(filepath, mDecodedData);
			mCurrentCharset = mSuggestedCharset = "UTF-8";
			mHaveDecodedData = true;
		}

		mContentType.SetTo( mimetype<<"; name=\"" << ref->name << '"');
		if (mimetype.ICompare( "text/", 5) == 0)
			mContentTransferEncoding 
				= NeedsQuotedPrintableEncoding( mDecodedData, BM_MAX_BODY_LINE_LEN)
					? ThePrefs->GetBool( "Allow8BitMime", false) 
						? "8bit"
						: "quoted-printable"
					: "7bit";
		else if (mimetype.ICompare( "message/", 8) == 0)
			mContentTransferEncoding 
				= NeedsQuotedPrintableEncoding( mDecodedData, BM_MAX_BODY_LINE_LEN)
					? ThePrefs->GetBool( "Allow8BitMime", false) 
						? "8bit"
						: "7bit"
					: "7bit";
		else
			mContentTransferEncoding = "base64";

		if (mContentTransferEncoding=="7bit" || mContentTransferEncoding=="8bit"
		|| mContentTransferEncoding=="quoted-printable") {
			// we compute an encoded-size estimate for text, (we simply assume 
			// that UTF8-encoding yields about the same size as quoted-printable):
			mBodyLength = mDecodedData.Length();
		} else {
			// we compute an encoded-size estimate for base64:
			mBodyLength = (int)(mDecodedData.Length()*4.1)/3;
		}
		
		mInitCheck = B_OK;
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("BodyPart:\n\t") << err.what());
	}
}
	
/*------------------------------------------------------------------------------*\
	BmBodyPart( bodypart)
	-	copy c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( const BmBodyPart& in) 
	:	inherited( BmString("")<<NextObjectID(), NULL, NULL)
	,	mIsMultiPart( in.IsMultiPart())
	,	mContentType( in.ContentTypeAsString())
	,	mContentTransferEncoding( in.TransferEncoding())
	,	mContentId( in.Id())
	,	mContentDisposition( in.ContentDispositionAsString())
	,	mContentDescription( in.Description())
	,	mContentLanguage( in.Language())
	,	mFileName( in.FileName())
	,	mEntryRef( in.EntryRef())
	,	mInitCheck( in.InitCheck())
	// we can't store info about mailtext, since there is no mailtext available:
	,	mStartInRawText( 0)
	,	mBodyLength( 0)
	,	mHaveDecodedData( false)
	,	mSuggestedCharset( in.SuggestedCharset())
	,	mCurrentCharset( in.CurrentCharset())
	, 	mHadErrorDuringConversion( false)
{
	mDecodedData.SetTo( in.DecodedData());
	mHaveDecodedData = true;
	BmModelItemMap::const_iterator iter;
	for( iter = in.begin(); iter != in.end(); ++iter) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
		BmBodyPart* copiedBody = new BmBodyPart( *bodyPart);
		AddSubItem( copiedBody);
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
void BmBodyPart::SetTo( const BmString& msgtext, int32 start, int32 length, 
								const BmString& defaultCharset,
								BmRef<BmMailHeader> header) {
	BM_LOG2( BM_LogMailParse, 
				BmString("BodyPart::SetTo() start: ") << start 
					<< " len: " << length);
	BmString type;
	BmString transferEncoding;
	BmString id;
	BmString disposition;
 	BmRef<BmListModel> bodyRef = mListModel.Get();
 	BmBodyPartList* body = dynamic_cast< BmBodyPartList*>( bodyRef.Get());
 	
 	mHadErrorDuringConversion = false;
 	mParsingErrors.Truncate(0);
 	
 	if (length < 0)
 		length = 0;

	if (!header) {
		// this is not the main body, so we have to split the MIME-headers from
		// the MIME-bodypart:
		BmString headerText;
		if (!length) {
			mStartInRawText = start;
			mBodyLength = 0;
		} else {
			BM_LOG2( BM_LogMailParse, "looking for end-of-header...");
			int32 end = start+length;
			int32 pos = start;
			if (pos<end+1 && msgtext[pos] == '\r' && msgtext[pos+1] == '\n') {
				// ...first line is empty, meaning we have an empty header
				mStartInRawText = pos+2;
			} else {
				// try to find the empty line that separates header from body:
				pos = msgtext.FindFirst( "\r\n\r\n", start);
				if (pos < 0 || pos + 4 > end) {
					BmString str;
					msgtext.CopyInto( str, start, std::min( length, (int32)256));
					BmString s 
						= BmString("Couldn't determine borderline between "
									  "MIME-header and body in string <")<<str<<">.";
					BM_LOG( BM_LogMailParse, s);
				 	AddParsingError(s);
				 	pos = end;
					mStartInRawText = end;
				} else
					mStartInRawText = pos+4;
			}
			BM_LOG2( BM_LogMailParse, "...copying headerText...");
			msgtext.CopyInto( headerText, start, pos-start+2);
			BM_LOG2( BM_LogMailParse, "...done");
			mBodyLength = length - (mStartInRawText-start);
		}
		BM_LOG2( BM_LogMailParse, BmString("MIME-Header found: ") << headerText);
		header = new BmMailHeader( headerText, NULL);
	} else {
		mStartInRawText = start;
		mBodyLength = length;
	}
	// MIME-type
	BM_LOG2( BM_LogMailParse, "parsing Content-Type");
	type = header->GetFieldVal( BM_FIELD_CONTENT_TYPE);
	if (!type.Length() || type.ICompare("text")==0) {
		// set content-type to default if is empty or contains "text"
		// (which is illegal but used by some broken mail-clients, it seems...)
		if (ThePrefs->GetBool( "StrictCharsetHandling", false))
			// strict mode: no charset means: us-ascii:
			type = "text/plain; charset=us-ascii";
		else
			// more relaxed, no charset means: default charset
			type = BmString("text/plain; charset=")<<mSuggestedCharset;
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
	transferEncoding.RemoveSet( BM_WHITESPACE.String());
							// some broken (webmail)-clients produce stuff like
							// "7 bit"...
	transferEncoding.IReplaceAll( "bits", "bit");
							// others use '8bits' instead of '8bit'...
	transferEncoding.IReplaceAll( "7-bit", "7bit");
	transferEncoding.IReplaceAll( "8-bit", "8bit");
							// other broken (webmail)-clients produce stuff like 
							// "7-bit" (argh)
	if (!transferEncoding.Length())
		transferEncoding = "7bit";
	mContentTransferEncoding = transferEncoding;
	BM_LOG2( BM_LogMailParse, 
				BmString("...found value: ")<<mContentTransferEncoding);

	// determine charset of bodypart, trying to not make use of:
	// 	-	an empty charset
	//		-	the (dummy) charset "unknown-8bit"
	mCurrentCharset = mSuggestedCharset = mContentType.Param( "charset"); 
	if (!mCurrentCharset.Length() || !mCurrentCharset.ICompare("unknown-8bit"))
		mCurrentCharset = mSuggestedCharset = defaultCharset;
	if (!mCurrentCharset.Length() || !mCurrentCharset.ICompare("unknown-8bit"))
		mCurrentCharset = mSuggestedCharset 
			= ThePrefs->GetBool( "ImportExportTextAsUtf8", true)
				? BmString("utf-8")
				: ThePrefs->GetString( "DefaultCharset");

	// MIME-Decoding:
	if (mIsMultiPart) {
		// decoding is unneccessary for multiparts, since they are never 
		// handled on their own (they are split into their subparts instead)
		mHaveDecodedData = true;
	} else {
		// decode body:
		if (IsText() && body->EditableTextBody() == this) {
			// text data is decoded and then converted from it's native charset
			// into utf8:
			DecodeText();
		} else {
			// decoding of attachments is deferred until actually needed
		}
	}
	// id
	BM_LOG2( BM_LogMailParse, "parsing Content-Id");
	mContentId = header->GetFieldVal( BM_FIELD_CONTENT_ID);
	BM_LOG2( BM_LogMailParse, BmString("...found value: ")<<mContentId);
	// disposition
	BM_LOG2( BM_LogMailParse, "parsing Content-Disposition");
	disposition = header->GetFieldVal( BM_FIELD_CONTENT_DISPOSITION);
	if (!disposition.Length())
		disposition = (IsPlainText() ? "inline" : "attachment");
	mContentDisposition.SetTo( disposition);
	// description
	BM_LOG2( BM_LogMailParse, "parsing Content-Description");
	mContentDescription = header->GetFieldVal( BM_FIELD_CONTENT_DESCRIPTION);
	BM_LOG2( BM_LogMailParse, 
				BmString("...found value: ")<<mContentDescription);
	// Language
	BM_LOG2( BM_LogMailParse, "parsing Content-Language");
	mContentLanguage = header->GetFieldVal( BM_FIELD_CONTENT_LANGUAGE);
	mContentLanguage.ToLower();
	BM_LOG2( BM_LogMailParse, BmString("...found value: ")<<mContentLanguage);
	// determine a filename (if possible)
	mFileName = mContentDisposition.Param("filename");
	mFileName.ReplaceSet( "/~<>()`´\\\"'", "_");
	if (!mFileName.Length()) {
		mFileName = mContentType.Param("name");
		if (!mFileName.Length()) {
			mFileName = TheTempFileList.NextTempFilename();
		}
	}
		
	if (mIsMultiPart) {
		BmString boundary = BmString("--")+mContentType.Param("boundary");
		if (boundary.Length()==2) {
			BmString errStr("No boundary specified within multipart-message!");
			BM_LOG( BM_LogMailParse, errStr);
			AddParsingError( errStr);
			return;
		}
		char* startPos 
			= strstr( msgtext.String()+mStartInRawText, boundary.String());
		if (!startPos) {
			BmString errStr 
				= BmString("Boundary <")<<boundary<<"> not found within message.";
			BM_LOG( BM_LogMailParse, errStr);
			AddParsingError( errStr);
			return;
		}
		BmString checkStr;
		BmString foundBoundary;
		bool isLastBoundary = false;
		Regexx rx;
		char* nPos = startPos;
		int32 foundBoundaryLen;
							// length of current boundary that was found and matches the
							// given boundary
		int32 firstBoundaryLen=0;
							// length of first (given) boundary
		while( !isLastBoundary) {
			while( 1) {
				// in this loop we determine the next occurence of the current 
				// boundary, being careful not to accept boundaries that do not
				// start at the beginning of a line or which are followed by 
				// nonspace-characters.
				foundBoundaryLen = boundary.Length();
				// boundary may have stop-marks (meaning this should be the last
				// sub-mimepart), we skip those:
				if (*(nPos+foundBoundaryLen)=='-' 
				&& *(nPos+foundBoundaryLen+1)=='-')
					foundBoundaryLen+=2;
				// skip any space and tabs:
				while (*(nPos+foundBoundaryLen)==' ' 
				|| *(nPos+foundBoundaryLen)=='\t')
					foundBoundaryLen++;
				// now skip over \r\n if present:
				if (*(nPos+foundBoundaryLen)=='\r')
					foundBoundaryLen++;
				if (*(nPos+foundBoundaryLen)=='\n')
					foundBoundaryLen++;
				if (!firstBoundaryLen)
					firstBoundaryLen = foundBoundaryLen;
				BM_LOG2( BM_LogMailParse, "finding next boundary...");
				nPos = strstr( nPos+foundBoundaryLen, boundary.String());
				if (!nPos) {
					BM_LOG2( BM_LogMailParse, 
								"...done (no further boundary found)");
					break;
				}
				BM_LOG2( BM_LogMailParse, "...done (found next boundary)");
				if (*(nPos-1)=='\n') {
					BM_LOG2( BM_LogMailParse, "init of boundary check...");
					char* endOfLine = strchr( nPos, '\r');
					BM_LOG2( BM_LogMailParse, "...done (init of boundary check)");
					if (endOfLine) {
						int32 len=endOfLine-nPos;
						BM_LOG2( BM_LogMailParse, 
									BmString("setting checkStr to length ") << len);
						checkStr.SetTo( nPos, len);
					} else {
						BM_LOG2( BM_LogMailParse, "setting checkStr to remainder");
						checkStr.SetTo( nPos);
					}
					// checking for last boundary (with -- appended):
					BM_LOG2( BM_LogMailParse, "boundary check(1)...");
					if (rx.exec( checkStr, "^(.+?)--\\s*$", Regexx::newline)
					&& boundary.ICompare( rx.match[0].atom[0])==0) {
						isLastBoundary = true;
						break;
					}
					// checking if found boundary starts line and is just 
					// followed by whitespace (if anything at all):
					BM_LOG2( BM_LogMailParse, "...boundary check(2)...");
					if (rx.exec( checkStr, "^(.+?)\\s*$", Regexx::newline)
					&& boundary.ICompare( rx.match[0].atom[0])==0)
						break;
					BM_LOG2( BM_LogMailParse, "...done");
				}
			}
			if (nPos) {
				int32 startOffs = startPos-msgtext.String()+firstBoundaryLen;
				BM_LOG2( BM_LogMailParse, 
							"Subpart of multipart found will be added to array");
				int32 len = std::max((long)0,nPos-msgtext.String()-startOffs-2);
							// -2 in order to leave out \r\n before boundary
				BmBodyPart *subPart 
					= new BmBodyPart( (BmBodyPartList*)ListModel().Get(), 
										   msgtext, startOffs, len,
											defaultCharset, NULL, this);
				BmAutolockCheckGlobal lock( ListModel()->ModelLocker());
				if (!lock.IsLocked())
					BM_THROW_RUNTIME( "BmBodyPart::SetTo(): Unable to get lock");
				AddSubItem( subPart);
				startPos = nPos;
			} else {
				int32 startOffs = startPos-msgtext.String()+firstBoundaryLen;
				if (start+length > startOffs) {
					// the final boundary is missing, we include the remaining 
					// part as a sub-bodypart anyway:
					int32 nlPos=msgtext.FindFirst( "\r\n", startOffs);
					if (nlPos!=B_ERROR && nlPos<start+length) {
						BM_LOG2( BM_LogMailParse, 
									"Subpart of multipart found will be added to array");
						BmBodyPart *subPart 
							= new BmBodyPart( (BmBodyPartList*)ListModel().Get(), 
												   msgtext, startOffs, 
													start+length-startOffs, 
													defaultCharset, NULL, this);
						BmAutolockCheckGlobal lock( ListModel()->ModelLocker());
						if (!lock.IsLocked())
							BM_THROW_RUNTIME( 
								"BmBodyPart::SetTo(): Unable to get lock"
							);
						AddSubItem( subPart);
					}
				}
				break;
			}
		}
	}
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	AddParsingError()
	-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::AddParsingError( const BmString& errStr) const
{
	if (errStr.Length()) {
		if (mParsingErrors.Length())
			mParsingErrors << "\n\n";
		mParsingErrors << errStr;
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
	DecodeText()
	-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::DecodeText(const char* tryCharset) {
	if (tryCharset)
		mSuggestedCharset = tryCharset;
	DecodedData();
	BM_LOG2( BM_LogMailParse, "...splitting off signature...");
	// split off signature, if any:
	Regexx rx;
	BmString sigRX = ThePrefs->GetString( "SignatureRX");
	int32 count 
		= rx.exec( mDecodedData, sigRX, Regexx::newline|Regexx::global);
	if (count>0) {
		BmString sigStr( mDecodedData.String()+rx.match[count-1].start()
									+rx.match[count-1].Length());
		if (sigStr.CountLines() <= ThePrefs->GetInt("MaxLinesForSignature", 5)) {
			// split-off signature:
			mDecodedData.Truncate( rx.match[count-1].start());
		 	BmRef<BmListModel> bodyRef = mListModel.Get();
		 	BmBodyPartList* body = dynamic_cast< BmBodyPartList*>( bodyRef.Get());
		 	if (body)
				body->Signature( sigStr);
		}
	}
	BM_LOG2( BM_LogMailParse, "...done (decoding of text-part)");
}

/*------------------------------------------------------------------------------*\
	DecodedData()
	-	
\*------------------------------------------------------------------------------*/
const BmString& BmBodyPart::DecodedData() const {
	if (!mHaveDecodedData || mCurrentCharset != mSuggestedCharset) {
		mParsingErrors.Truncate(0);
		BmRef<BmListModel> listModel( ListModel());
		if (listModel) {
			BmBodyPartList* bodyPartList 
				= dynamic_cast< BmBodyPartList*>( listModel.Get());
			const BmMail* mail;
			if (bodyPartList && (mail=bodyPartList->Mail())!=NULL) {
				if (IsText()) {
					// text data is decoded and then converted from it's native
					// charset into utf8:
					BM_LOG2( BM_LogMailParse, 
								BmString( "(re-)converting bodytext of ") 
									<< mBodyLength << " bytes...");
					BmCharsetVect charsetVect;
					if (mSuggestedCharset != mCurrentCharset 
					|| !ThePrefs->GetBool("AutoCharsetDetectionInbound", true))
						// user suggested a charset, we try that:
						charsetVect.push_back( mSuggestedCharset);
					else
						// we try the native charset first and (in case of errors)
						// all preferred charsets:
						GetPreferredCharsets( charsetVect, mSuggestedCharset);
					BmString charset;
					for( uint32 i=0; i<charsetVect.size(); ++i) {
						BmStringIBuf text( mail->RawText().String()+mStartInRawText,
												 mBodyLength);
						BmMemFilterRef decoder 
							= FindDecoderFor( &text, mContentTransferEncoding);
						BmLinebreakDecoder linebreakDecoder( decoder.get());
						BmStringOBuf tempIO( mBodyLength, 1.2f);
						charset = charsetVect[i];
						BM_LOG2( BM_LogMailParse, 
									BmString( "trying charset ") << charset);
						BmUtf8Encoder textConverter( &linebreakDecoder, charset);
						BmMailtextCleaner mailtextCleaner( &textConverter);
						tempIO.Write( &mailtextCleaner);
						mHadErrorDuringConversion = textConverter.HadToDiscardChars() 
											|| textConverter.HadError();
						if (decoder->HaveStatusText())
							AddParsingError(decoder->StatusText());
						if (!mHadErrorDuringConversion || i==charsetVect.size()-1) {
							if (i>0) {
								AddParsingError(
									BmString("Autodetected charset (")
										<< charset << "), may need manual correction"
								);
							}
							mDecodedData.Adopt( tempIO.TheString());
							break;
						}
					}
					mCurrentCharset = mSuggestedCharset = charset;
				} else {
					BmStringIBuf text( mail->RawText().String()+mStartInRawText, 
											 mBodyLength);
					BmMemFilterRef decoder 
						= FindDecoderFor( &text, mContentTransferEncoding);
					BmStringOBuf tempIO( mBodyLength, 1.2f);
					BM_LOG2( BM_LogMailParse, 
								BmString( "decoding bodytext of ") << mBodyLength 
									<< " bytes...");
					tempIO.Write( decoder.get());
					mDecodedData.Adopt( tempIO.TheString());
					mHadErrorDuringConversion = false;
					if (decoder->HaveStatusText())
						AddParsingError(decoder->StatusText());
				}
				BM_LOG2( BM_LogMailParse, "done");
				mHaveDecodedData = true;
			}
		}
	}
	return mDecodedData; 
}

/*------------------------------------------------------------------------------*\
	ContainsRef()
	-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::ContainsRef( const entry_ref& ref) const {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
		if (bodyPart->EntryRef() == ref)
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	WriteToFile()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::WriteToFile( BFile& file) {
	if (IsText() && !ThePrefs->GetBool( "ImportExportTextAsUtf8", true)) {
		BmString convertedString;
		ConvertFromUTF8( mSuggestedCharset, DecodedData(), convertedString);
		file.Write( convertedString.String(), convertedString.Length());
	} else
		file.Write( DecodedData().String(), DecodedLength());
	BNodeInfo fileInfo;
	fileInfo.SetTo( &file);
	fileInfo.SetType( MimeType().String());
}

/*------------------------------------------------------------------------------*\
	WriteToTempFile()
		-	
\*------------------------------------------------------------------------------*/
entry_ref BmBodyPart::WriteToTempFile( BmString filename) {
	BPath tempPath;
	entry_ref eref;
	if (!filename.Length()) {
		filename = mFileName;
	}
	filename.ReplaceSet( "/`´:\"\\", "_");
	if (filename.Length() > B_FILE_NAME_LENGTH)
		filename.Truncate( B_FILE_NAME_LENGTH);
	if (find_directory( B_COMMON_TEMP_DIRECTORY, &tempPath, true) == B_OK) {
		BDirectory tempDir;
		BFile tempFile;
		status_t err;
		tempDir.SetTo( tempPath.Path());
		if ((err = tempFile.SetTo( 
			&tempDir, filename.String(), 
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		) != B_OK) {
			BM_SHOWERR( BmString("Could not create temporary file\n\t<") 
								<< filename << ">\n\n Result: " << strerror(err));
			return eref;
		}
		TheTempFileList.AddFile( BmString(tempPath.Path())<<"/"<<filename);
		WriteToFile( tempFile);
		BEntry entry( &tempDir, filename.String());
		BPath path;
		entry.GetPath( &path);
		err = entry.InitCheck();
		if (err==B_OK && path.InitCheck()==B_OK && path.Path())
			update_mime_info( path.Path(), false, true, false);
		entry.GetRef( &eref);
	}
	return eref;
}

/*------------------------------------------------------------------------------*\
	SaveAs()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SaveAs( const entry_ref& destDirRef, BmString filename) {
	BDirectory destDir;
	status_t err;
	if ((err=destDir.SetTo( &destDirRef)) == B_OK) {
		BFile destFile;
		if ((err = destFile.SetTo( 
			&destDir, filename.String(), 
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		) != B_OK) {
			BM_SHOWERR( BmString("Could not save attachment\n\t<") 
								<< filename << ">\n\n Result: " << strerror(err));
			return;
		}
		WriteToFile( destFile);
		BEntry entry;
		destDir.GetEntry( &entry);
		BPath path;
		entry.GetPath( &path);
		if (path.InitCheck()==B_OK) {
			BmString fullpath( path.Path());
			fullpath << "/" << filename;
			update_mime_info( fullpath.String(), false, true, false);
		}
	} else
		BM_SHOWERR( BmString("Could not save attachment\n\t<") << filename 
							<< ">\n\n Result: " << strerror(err));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SetBodyText( const BmString& utf8Text, 
										const BmString& charset) {
	mDecodedData = utf8Text;
	mSuggestedCharset = mCurrentCharset = charset;
	bool needsQP = NeedsQuotedPrintableEncoding( utf8Text, BM_MAX_BODY_LINE_LEN);
	mContentTransferEncoding = needsQP
											? (ThePrefs->GetBool( "Allow8BitMime", false)
												? "8bit"
												: "quoted-printable")
											: "7bit";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmBodyPart::GenerateBoundary() {
	return BmString("_=_BOUNDARY_") << system_time() << "_"
				<< ++nBoundaryCounter << "_=_";
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::PropagateHigherEncoding() {
	if (!IsMultiPart())
		return;
	BmString newTransferEncoding = "7bit";
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
	PruneUnnededMultiParts()
		-	
\*------------------------------------------------------------------------------*/
int32 BmBodyPart::PruneUnneededMultiParts() {
	int32 count=0;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter++->second.Get());
		if (bodyPart->IsMultiPart()) {
			int32 subCount = bodyPart->PruneUnneededMultiParts();
			if (subCount == 0) {
				// remove this multipart, it is empty:
				RemoveSubItem( bodyPart);
				continue;
			} else if (subCount == 1) {
				// replace this multipart with its only child:
				BmRef< BmListModelItem> childRef( bodyPart->begin()->second.Get());
				RemoveSubItem( bodyPart);
				AddSubItem( childRef.Get());
			}
		}
		count++;
	}
	return count;
}

/*------------------------------------------------------------------------------*\
	EstimateEncodedSize()
		-	
\*------------------------------------------------------------------------------*/
int32 BmBodyPart::EstimateEncodedSize() {
	int32 size = 1024 + (IsMultiPart() ? 0 : mBodyLength);
							// 1024 makes room for header-fields
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter++->second.Get());
		size += bodyPart->EstimateEncodedSize();
	}
	return size;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::ConstructBodyForSending( BmStringOBuf &msgText) {
	BmString boundary;
	if (IsMultiPart()) {
		PropagateHigherEncoding();
		boundary = GenerateBoundary();
		mContentType.SetParam( "boundary", boundary);
	}
	BmRef<BmListModel> listModel( ListModel());
	const BmMail* mail = NULL;
	BmBodyPartList* bodyPartList = NULL;
	if (listModel) {
		bodyPartList = dynamic_cast< BmBodyPartList*>( listModel.Get());
		if (bodyPartList)
			mail = bodyPartList->Mail();
	}
	bool haveEncodedText 
		= bodyPartList && mail && mStartInRawText 
			&& bodyPartList->EditableTextBody() != this;
	BmString encodedTextStr;
	if (IsText()) {
		if (!haveEncodedText) {
			// need to convert the text from utf-8 to native charset:
			BM_LOG2( BM_LogMailParse, 
						BmString( "encoding/converting bodytext of ") 
										  << DecodedLength() << " bytes to " 
										  << mCurrentCharset);
			BmCharsetVect charsetVect;
			if (ThePrefs->GetBool("AutoCharsetDetectionOutbound", true)) {
				// we try the native charset first and (in case of errors)
				// all preferred charsets:
				GetPreferredCharsets(charsetVect, mCurrentCharset, true);
			} else
				charsetVect.push_back(mCurrentCharset);
			BmString charset;
			for( uint32 i=0; i<charsetVect.size(); ++i) {
				charset = charsetVect[i];
				BM_LOG2( BM_LogMailParse, BmString( "trying charset ") << charset);
				BmStringIBuf text( DecodedData());
				BmUtf8Decoder textConverter( &text, charset);
				BmMemFilterRef encoder 
					= FindEncoderFor( &textConverter, mContentTransferEncoding);
				BmStringOBuf encodedText(DecodedLength());
				encodedText.Write( encoder.get());
				if (textConverter.HadError() || textConverter.HadToDiscardChars()) {
					if (i+1 == charsetVect.size()) {
						// last round, autodetection of charset failed, so we bail:
						BmString errText
							= (bodyPartList->EditableTextBody() == this)
								?	BmString("The mailtext contains characters that ")
									<< "could not be converted to the selected charset ("
									<< mCurrentCharset << ").\n\n"
									<< "Please select the correct charset or remove "
									<< "the offending characters."
								:	BmString("The attachment ") << FileName()
									<< " contains characters that "
									<< "could not be converted to the selected charset ("
									<< mCurrentCharset << ").\n\n"
									<< "Please re-add the attachment with the correct "
									<< "charset.";
						throw BM_text_error( 
							errText, "", textConverter.FirstDiscardedPos()
						);
					}
				} else {
					encodedTextStr.Adopt( encodedText.TheString());
					mCurrentCharset = mSuggestedCharset = charset;
					break;
				}
			}
			BM_LOG2( BM_LogMailParse, "...done (bodytext)");
		}
		mContentType.SetParam( "charset", mCurrentCharset);
	}
	msgText << BM_FIELD_CONTENT_TYPE << ": " << mContentType << "\r\n";
	msgText << BM_FIELD_CONTENT_TRANSFER_ENCODING << ": " 
			  << mContentTransferEncoding << "\r\n";
	msgText << BM_FIELD_CONTENT_DISPOSITION << ": " << mContentDisposition 
			  << "\r\n";
	if (mContentDescription.Length())
		msgText << BM_FIELD_CONTENT_DESCRIPTION << ": " << mContentDescription 
				  << "\r\n";
	if (mContentId.Length())
		msgText << BM_FIELD_CONTENT_ID << ": " << mContentId << "\r\n";
	if (mContentLanguage.Length())
		msgText << BM_FIELD_CONTENT_LANGUAGE << ": " << mContentLanguage 
				  << "\r\n";
	msgText << "\r\n";

	if (IsMultiPart())
		msgText << "This is a multi-part message in MIME format.\r\n\r\n";
	else {
		uint32 newStartPos = msgText.CurrPos();
	
		if (haveEncodedText) {
			// we already have the encoded text, we simply copy that:
			BM_LOG2( BM_LogMailParse, 
						BmString( "copying bodytext of ") << mBodyLength 
							<< " bytes...");
			mBodyLength = msgText.Write( mail->RawText().String()+mStartInRawText, 
												  mBodyLength);
			BM_LOG2( BM_LogMailParse, "...done (bodytext)");
		} else {
			if (IsText()) {
				// copy encoded text into message:
				BmStringIBuf text( encodedTextStr);
				mBodyLength = msgText.Write(&text);
			} else {
				// encode buffer:
				BmStringIBuf text( DecodedData());
				BM_LOG2( BM_LogMailParse, 
							BmString( "encoding bodytext of ") << DecodedLength() 
								<< " bytes...");
				BmMemFilterRef encoder 
					= FindEncoderFor( &text, mContentTransferEncoding);
				mBodyLength = msgText.Write( encoder.get());
				BM_LOG2( BM_LogMailParse, "...done (bodytext)");
			}
		}
		mStartInRawText = newStartPos;
		uint32 len = msgText.CurrPos();
		if (len && msgText.ByteAt( len-1) != '\n')
			msgText << "\r\n";
	}
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPart::MimeTypeIsPotentiallyHarmful( const BmString& realMT) {
	vector<BmString> trustInfos;
	BmString ti = ThePrefs->GetString( "MimeTypeTrustInfo");
	split( BmPrefs::nListSeparator, ti, trustInfos);
	int32 numTrustInfos = trustInfos.size();
	BmString mt;
	BmString trust;
	BmString trustInfo;
	int32 colonPos;
	for( int32 i=0; i<numTrustInfos; ++i) {
		trustInfo = trustInfos[i];
		colonPos = trustInfo.FindFirst( ":");
		if (colonPos >= B_OK) {
			trustInfo.CopyInto( mt, 0, colonPos);
			if ( realMT.ICompare( mt, mt.Length()) == 0) {
				trustInfo.CopyInto( trust, colonPos+1, 1);
				if (trust.ICompare( "T") != 0)
					return true;
				break;
			}
		}
	}
	return false;
}



/********************************************************************************\
	BmBodyPartList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmBodyPartList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmBodyPartList::BmBodyPartList( BmMail* mail)
	:	inherited( BmString("BodyPartList_") << mail->ModelName(), 
					  BM_LogMailParse)
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
	if (mMail && mMail->HeaderLength() >= 2) {
		const BmString& msgText = mMail->RawText();
		BmBodyPart* bodyPart 
			= new BmBodyPart( this, msgText, mMail->HeaderLength()+2, 
									MAX(msgText.Length()-mMail->HeaderLength()-2, 0), 
									mMail->DefaultCharset(),	mMail->Header());
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
	} catch (BM_error &e) {
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
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	if (empty())
		return false;
	if (!mEditableTextBody)
		return true;
	if (size()==1) {
		BmBodyPart* firstBody 
			= dynamic_cast<BmBodyPart*>( begin()->second.Get());
		if (firstBody)
			return firstBody->IsMultiPart() && firstBody->size()>1;
	}
	return size()>1;
}

/*------------------------------------------------------------------------------*\
	AddAttachmentFromRef()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::AddAttachmentFromRef( const entry_ref* ref, 
														 const BmString& charset) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	BmRef<BmListModelItem> parentRef;
	if (editableTextBody)
		parentRef = editableTextBody->Parent();
	BmBodyPart* parent = dynamic_cast< BmBodyPart*>( parentRef.Get());
	bool alreadyPresent = false;
	if (parent)
		alreadyPresent = parent->ContainsRef( *ref);
	else {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); !alreadyPresent && iter != end(); ++iter) {
			BmBodyPart* bodyPart 
				= dynamic_cast< BmBodyPart*>( iter->second.Get());
			if (bodyPart->EntryRef() == *ref)
				alreadyPresent = true;
		}
	}
	if (!alreadyPresent) {
		BmBodyPart* bodyPart = new BmBodyPart( this, ref, charset, parent);
		if (bodyPart->InitCheck() == B_OK) {
			AddItemToList( bodyPart, parent);
		} else
			delete bodyPart;
	}
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList()
		-	calls PruneUnnededMultiParts() in order to keep the bodypart-list
			at its minimal optimum
\*------------------------------------------------------------------------------*/
void BmBodyPartList::RemoveItemFromList( BmListModelItem* item) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	Freeze();
	inherited::RemoveItemFromList( item);
	PruneUnneededMultiParts();
	Thaw();
	TellModelItemRemoved( item);
}

/*------------------------------------------------------------------------------*\
	SetEditableText()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::SetEditableText( const BmString& utf8Text, 
												  const BmString& charset) {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	if (editableTextBody)
		editableTextBody->SetBodyText( utf8Text, charset);
}

/*------------------------------------------------------------------------------*\
	DefaultCharset()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmBodyPartList::DefaultCharset() const {
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	if (editableTextBody)
		return editableTextBody->SuggestedCharset();
	return BmEncoding::DefaultCharset; 
}

/*------------------------------------------------------------------------------*\
	IsMultiPart()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::IsMultiPart() const {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	if (size()==1) {
		BmBodyPart* firstBody 
			= dynamic_cast<BmBodyPart*>( begin()->second.Get());
		if (firstBody)
			return firstBody->IsMultiPart();
	}
	return size()>1;
}

/*------------------------------------------------------------------------------*\
	PruneUnnededMultiParts()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartList::PruneUnneededMultiParts() {
	vector< BmRef< BmListModelItem> > removedRefVect;
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":PruneUnnededMultiParts(): Unable to get lock"
		);
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter++->second.Get());
		if (bodyPart->IsMultiPart()) {
			int32 subCount = bodyPart->PruneUnneededMultiParts();
			if (subCount == 0) {
				// remove this multipart, it is empty:
				removedRefVect.push_back(bodyPart);
				inherited::RemoveItemFromList( bodyPart);
			} else if (subCount == 1) {
				// replace this multipart with its only child:
				BmRef< BmListModelItem> childRef( bodyPart->begin()->second.Get());
				removedRefVect.push_back(bodyPart);
				inherited::RemoveItemFromList( bodyPart);
				AddItemToList( childRef.Get());
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	EstimateEncodedSize()
		-	
\*------------------------------------------------------------------------------*/
int32 BmBodyPartList::EstimateEncodedSize() {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":EstimateEncodedSize(): Unable to get lock"
		);
	int32 size = 0;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ) {
		BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter++->second.Get());
		size += bodyPart->EstimateEncodedSize();
	}
	return size;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartList::ConstructBodyForSending( BmStringOBuf& msgText) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":ConstructBodyForSending(): Unable to get lock"
		);
	BmRef<BmBodyPart> editableTextBody( EditableTextBody());
	bool hasMultiPartTop = editableTextBody && editableTextBody->Parent();
	bool isMultiPart = hasMultiPartTop
								? editableTextBody->Parent()->size() > 1
								: size() > 1;
	BmString boundary;
	if (isMultiPart && !hasMultiPartTop) {
		boundary = BmBodyPart::GenerateBoundary();
		msgText << BM_FIELD_CONTENT_TYPE << ": multipart/mixed; boundary=\""
				  << boundary<<"\"\r\n";
		msgText << BM_FIELD_CONTENT_TRANSFER_ENCODING << ": 7bit\r\n\r\n";
		msgText << "This is a multi-part message in MIME format.\r\n\r\n";
		msgText << "--"<<boundary<<"\r\n";
	}
	BmModelItemMap::const_iterator iter;
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
	return true;
}
