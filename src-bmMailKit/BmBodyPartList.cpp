/*
	BmBodyPartList.cpp
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
#include <Invoker.h>

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

	if (rx.exec( cfString, "^\\s*([^\\s;]+)\\s*([;\\s].*)?\\s*$")) {
		// extract value:
		if (rx.match[0].atom.size() > 0) {
			if (cfString[rx.match[0].atom[0].start()] == '"') {
				// skip quotes during extraction:
				cfString.CopyInto( mValue, rx.match[0].atom[0].start()+1, rx.match[0].atom[0].Length()-2);
			} else {
				mValue = rx.match[0].atom[0];
			}
		}
		BmToLower( mValue);
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
					BmToLower( key);
					mParams[key] = val;
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
	BmToLower( key);
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
	BmToLower( key);
	mParams[key] = value;
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
	char* buf=s.LockBuffer( 10);
	sprintf( buf, "%5.5ld", ++nObjectID);
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
		char mt[B_MIME_TYPE_LENGTH+1];
		(err=file.SetTo( ref, B_READ_ONLY)) == B_OK		
														|| BM_THROW_RUNTIME( BString("Couldn't create file for <") << ref->name << "> \n\nError:" << strerror(err));
		(err=file.GetSize( &size)) == B_OK
														|| BM_THROW_RUNTIME( BString("Couldn't get file-size for <") << ref->name << "> \n\nError:" << strerror(err));
		(err=nodeInfo.SetTo( &file)) == B_OK
														|| BM_THROW_RUNTIME( BString("Couldn't create node-info for <") << ref->name << "> \n\nError:" << strerror(err));
		if (nodeInfo.GetType( mt)!=B_OK) {
			// no mimetype info yet, we ask BeOS to determine mimetype and then try again:
			BEntry entry( ref);
			BPath path;
			entry.GetPath( &path);
			status_t res=entry.InitCheck();
			if (res==B_OK && path.InitCheck()==B_OK && path.Path()) {
				update_mime_info( path.Path(), false, true, false);
				nodeInfo.GetType( mt);
			}
		}
		BString mimetype( mt);
		if (mimetype.ICompare("text/x-email")==0) {
			// convert beos-own mail-mimetype into correct message/rfc822:
			mimetype = "message/rfc822";
		}
		mFileName = ref->name;
		mContentType.SetTo( mimetype<<"; name=\"" << ref->name << '"');
		mContentDisposition.SetTo( BString( "attachment; filename=\"")<<ref->name<<'"');
		if (mimetype.ICompare( "text/", 5) == 0)
			mContentTransferEncoding = NeedsEncoding( mDecodedData) 
													? ThePrefs->GetBool( "Allow8BitMime", false) 
														? "8bit"
														: "quoted-printable"
													: "7bit";
		else if (mimetype.ICompare( "message/", 8) == 0)
			mContentTransferEncoding = NeedsEncoding( mDecodedData) 
													? ThePrefs->GetBool( "Allow8BitMime", false) 
														? "8bit"
														: "7bit"
													: "7bit";
		else
			mContentTransferEncoding = "base64";

		if (IsText() && !ThePrefs->GetBool( "ImportTextAsUtf8", false)) {
			BString convertedString;
			char* buf = convertedString.LockBuffer( size+1);
			file.Read( buf, size);
			buf[size] = '\0';
			convertedString.UnlockBuffer( size);
			ConvertToUTF8( ThePrefs->GetInt( "DefaultEncoding"), 
								convertedString, mDecodedData);
		} else {
			char* buf = mDecodedData.LockBuffer( size+1);
			file.Read( buf, size);
			buf[size] = '\0';
			mDecodedData.UnlockBuffer( size);
		}
		
		mInitCheck = B_OK;
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BodyPart:\n\t") << err.what());
	}
}
	
/*------------------------------------------------------------------------------*\
	BmBodyPart( bodypart)
	-	copy c'tor
\*------------------------------------------------------------------------------*/
BmBodyPart::BmBodyPart( const BmBodyPart& in) 
	:	inherited( BString("")<<NextObjectID(), NULL, NULL)
	,	mIsMultiPart( in.IsMultiPart())
	,	mContentType( in.ContentTypeAsString())
	,	mContentTransferEncoding( in.TransferEncoding())
	,	mContentId( in.ID())
	,	mContentDisposition( in.ContentDispositionAsString())
	,	mContentDescription( in.Description())
	,	mContentLanguage( in.Language())
	,	mFileName( in.FileName())
	,	mEntryRef( in.EntryRef())
	,	mInitCheck( in.InitCheck())
{
	int32 len = in.DecodedData().Length();
	if (len) {
		char* buf = mDecodedData.LockBuffer( len+1);
		if (buf) {
			memcpy( buf, in.DecodedData().String(), len);
			buf[len] = 0;
			mDecodedData.UnlockBuffer( len);
		}
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
	transferEncoding.IReplaceAll( "bits", "bit");
							// others use '8bits' instead of '8bit'...
	transferEncoding.IReplaceAll( "7-bit", "7bit");
	transferEncoding.IReplaceAll( "8-bit", "8bit");
							// other broken (webmail)-clients produce stuff like "7-bit" (argh)
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
		if (IsText()) {
			// text data is decoded and then converted from it's encoding into utf8:
			BString decodedString;
			Decode( mContentTransferEncoding, bodyString, decodedString, false);
			ConvertToUTF8( CharsetToEncoding( Charset()), decodedString, mDecodedData);
		} else {
			// non-text data is just decoded:
			Decode( mContentTransferEncoding, bodyString, mDecodedData, false);
		}
		if (body->EditableTextBody() == this) {
			// split off signature, if any:
			Regexx rx;
			BString sigRX = ThePrefs->GetString( "SignatureRX");
			int32 count = rx.exec( mDecodedData, sigRX, Regexx::newline|Regexx::global);
			if (count>0) {
				BString sigStr( mDecodedData.String()+rx.match[count-1].start()+rx.match[count-1].Length());
				mDecodedData.Truncate( rx.match[count-1].start());
				body->Signature( sigStr);
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
	BmToLower( mContentLanguage);
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
		char* startPos = strstr( posInRawText, boundary.String());
		if (!startPos) {
			BM_SHOWERR( BString("Boundary <")<<boundary<<"> not found within message.");
			return;
		}
		char* nPos = startPos;
		BString checkStr;
		BString foundBoundary;
		bool isLastBoundary = false;
		Regexx rx;
		while( !isLastBoundary) {
			while( 1) {
				// in this loop we determine the next occurence of the current boundary,
				// being careful not to accept boundaries that do not start at the beginning
				// of a line or which are followed by nonspace-characters.
				nPos = strstr( nPos+boundary.Length(), boundary.String());
				if (!nPos)
					break;
				if (*(nPos-1)=='\n') {
					char* endOfLine = strchr( nPos, '\r');
					if (endOfLine)
						checkStr.SetTo( nPos, endOfLine-nPos);
					else
						checkStr.SetTo( nPos);
					// checking for last boundary (with -- appended):
					if (rx.exec( checkStr, "^(.+?)--\\s*$", Regexx::newline)
					&& boundary.ICompare( rx.match[0].atom[0])==0) {
						isLastBoundary = true;
						break;
					}
					// checking if found boundary starts line and is just followed by whitespace
					// (if anything at all):
					if (rx.exec( checkStr, "^(.+?)\\s*$", Regexx::newline)
					&& boundary.ICompare( rx.match[0].atom[0])==0)
						break;
				}
			}
			if (nPos) {
				int32 startOffs = startPos-msgtext.String()+boundary.Length();
				BM_LOG2( BM_LogMailParse, "Subpart of multipart found will be added to array");
				BmBodyPart *subPart = new BmBodyPart( (BmBodyPartList*)ListModel().Get(), 
																  msgtext, startOffs, 
																  nPos-msgtext.String()-startOffs,
																  NULL, this);
				AddSubItem( subPart);
				startPos = nPos;
			} else {
				int32 startOffs = startPos-msgtext.String()+boundary.Length();
				if (start+length > startOffs) {
					// the final boundary is missing, we include the remaining part as a 
					// sub-bodypart anyway:
					int32 nlPos=msgtext.FindFirst( "\r\n", startOffs+2);
					if (nlPos!=B_ERROR && nlPos<start+length) {
						BM_LOG2( BM_LogMailParse, "Subpart of multipart found will be added to array");
						BmBodyPart *subPart = new BmBodyPart( (BmBodyPartList*)ListModel().Get(), 
																		  msgtext, startOffs, 
																		  start+length-startOffs, 
																		  NULL, this);
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
		if (IsText() && !ThePrefs->GetBool( "ExportTextAsUtf8", false)) {
			BString convertedString;
			ConvertFromUTF8( CharsetToEncoding( Charset()), mDecodedData, convertedString);
			tempFile.Write( convertedString.String(), convertedString.Length());
		} else
			tempFile.Write( mDecodedData.String(), mDecodedData.Length());
		fileInfo.SetTo( &tempFile);
		fileInfo.SetType( MimeType().String());
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
void BmBodyPart::SaveAs( const entry_ref& destDirRef, BString filename) {
	BDirectory destDir;
	status_t err;
	if ((err=destDir.SetTo( &destDirRef)) == B_OK) {
		BFile destFile;
		BNodeInfo fileInfo;
		if ((err = destFile.SetTo( &destDir, filename.String(), 
										  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
			BM_SHOWERR( BString("Could not save attachment\n\t<") << filename << ">\n\n Result: " << strerror(err));
			return;
		}
		if (IsText() && !ThePrefs->GetBool( "ExportTextAsUtf8", false)) {
			BString convertedString;
			ConvertFromUTF8( CharsetToEncoding( Charset()), mDecodedData, convertedString);
			destFile.Write( convertedString.String(), convertedString.Length());
		} else
			destFile.Write( mDecodedData.String(), mDecodedData.Length());
		fileInfo.SetTo( &destFile);
		fileInfo.SetType( MimeType().String());
		BEntry entry;
		destDir.GetEntry( &entry);
		BPath path;
		entry.GetPath( &path);
		if (path.InitCheck()==B_OK) {
			BString fullpath( path.Path());
			fullpath << "/" << filename;
			update_mime_info( fullpath.String(), false, true, false);
		}
	} else
		BM_SHOWERR( BString("Could not save attachment\n\t<") << filename << ">\n\n Result: " << strerror(err));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPart::SetBodyText( const BString& text, uint32 encoding) {
	ConvertToUTF8( encoding, text, mDecodedData);
	mContentType.SetParam( "charset", EncodingToCharset( encoding));
	mContentTransferEncoding = NeedsEncoding( text) 
											? (ThePrefs->GetBool( "Allow8BitMime", false)
												? "8bit"
												: "quoted-printable")
											: "7bit";
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
	BString newTransferEncoding = "7bit";
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
	if (IsMultiPart())
		msgText << "This is a multi-part message in MIME format.\r\n\r\n";
	BString encodedString;
	if (IsText()) {
		BString convertedString;
		ConvertFromUTF8( CharsetToEncoding( Charset()), mDecodedData, convertedString);
		Encode( mContentTransferEncoding, convertedString, encodedString);
	} else
		Encode( mContentTransferEncoding, mDecodedData, encodedString);
	msgText << encodedString;
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
	if (!mEditableTextBody)
		return true;
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
