/*
	BmBodyPartList.h
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


#ifndef _BmBodyPartList_h
#define _BmBodyPartList_h

#include <Entry.h>

#include "BmDataModel.h"

class BmMail;
class BmMailHeader;

/*------------------------------------------------------------------------------*\
	BmContentField
		-	
\*------------------------------------------------------------------------------*/
class BmContentField {
	typedef map< BString, BString> BmParamMap;

public:
	// c'tors and d'tor:
	BmContentField()							{ mInitCheck = B_NO_INIT; }
	BmContentField( const BString cfString);
	
	// native methods:
	void SetTo( const BString cfString);
	void SetParam( BString key, BString value);

	// getters:
	inline status_t InitCheck() const	{ return mInitCheck; }
	inline const BString& Value() const	{ return mValue; }
	const BString& Param( BString key) const;

	// operators:
	operator BString() const;
							// returns contentfield completely formatted (ready to be sent)

private:
	BString mValue;
	BmParamMap mParams;

	status_t mInitCheck;

	// Hide assignment:
	BmContentField operator=( const BmContentField&);
};



class BmBodyPartList;
/*------------------------------------------------------------------------------*\
	BmBodyPart
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPart : public BmListModelItem {
	typedef BmListModelItem inherited;

	static const int16 nArchiveVersion = 1;

public:
	// c'tors and d'tor:
	BmBodyPart( BmBodyPartList* model, const BString& msgtext, int32 s, int32 l, 
					BmRef<BmMailHeader> mHeader=NULL, BmListModelItem* parent=NULL);
	BmBodyPart( BmBodyPartList* model, const entry_ref* ref, BmListModelItem* parent=NULL);
	BmBodyPart( const BmBodyPart& bodyPart);
	~BmBodyPart();

	// class methods:
	static BString BmBodyPart::NextObjectID();

	// native methods:
	void SetTo( const BString& msgtext, int32 s, int32 l, BmRef<BmMailHeader> mHeader=NULL);
	void SetBodyText( const BString& text, uint32 encoding);
	bool IsText() const;
	bool IsPlainText() const;
	bool ShouldBeShownInline()	const;
	bool ContainsRef( const entry_ref& ref) const;
	entry_ref WriteToTempFile( BString filename="");
	void SaveAs( const entry_ref& destDirRef, BString filename);
	void PropagateHigherEncoding();
	void ConstructBodyForSending( BString &msgText);
	void SuggestEncoding( int32 enc) 			{ mSuggestedEncoding = enc; }

	static BString GenerateBoundary();

	// overrides of listmodelitem base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline bool IsMultiPart() const				{ return mIsMultiPart; }
	inline const BString& DecodedData() const;
	inline int32 DecodedLength() const			{ return DecodedData().Length(); }
	inline status_t InitCheck() const			{ return mInitCheck; }

	inline const BString ContentTypeAsString() const	{ return mContentType; }
	inline const BString ContentDispositionAsString() const	{ return mContentDisposition; }

	inline const BString& Charset() const		{ return mContentType.Param( "charset"); }
	inline const BString& MimeType() const		{ return mContentType.Value(); }
	inline const BString& Disposition() const	{ return mContentDisposition.Value(); }
	inline const BString& FileName() const		{ return mFileName; }
	inline const BString& TransferEncoding() const	{ return mContentTransferEncoding; }
	inline const BString& ID() const				{ return mContentId; }
	inline const BString& Description() const	{ return mContentDescription; }
	inline const BString& Language() const		{ return mContentLanguage; }
	inline const BString& TypeParam( BString key) const		{ return mContentType.Param( key); }
	inline const BString& DispositionParam( BString key) const	{ return mContentDisposition.Param( key); }
	inline int32 BodyLength() const				{ return mBodyLength; }
	inline int32 SuggestedEncoding() const		{ return mSuggestedEncoding; }
	inline int32 CurrentEncoding() const		{ return mCurrentEncoding; }

	inline const entry_ref& EntryRef() const	{ return mEntryRef; }

	inline const bool Is7Bit() const				{ return mContentTransferEncoding.ICompare( "7bit") == 0; }
	inline const bool Is8Bit() const				{ return mContentTransferEncoding.ICompare( "8bit") == 0; }
	inline const bool IsBinary() const			{ return mContentTransferEncoding.ICompare( "binary") == 0; }

	static int32 nBoundaryCounter;

private:
	bool mIsMultiPart;
	BmContentField mContentType;
	BString mContentTransferEncoding;
	BString mContentId;
	BmContentField mContentDisposition;
	BString mContentDescription;
	BString mContentLanguage;
	BString mFileName;

	mutable BString mDecodedData;
	int32 mStartInRawText;
	int32 mBodyLength;
	
	mutable int32 mCurrentEncoding;
	int32 mSuggestedEncoding;
	
	entry_ref mEntryRef;

	status_t mInitCheck;

	static int32 nObjectID;

	// Hide assignment:
	BmBodyPart operator=( const BmBodyPart&);
};


struct entry_ref;
/*------------------------------------------------------------------------------*\
	BmBodyPartList
		-	class 
\*------------------------------------------------------------------------------*/
class BmBodyPartList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 1;

public:
	// c'tors and d'tor
	BmBodyPartList( BmMail* mail);
	virtual ~BmBodyPartList();

	// native methods:
	void ParseMail();
	bool HasAttachments() const;
	void AddAttachmentFromRef( const entry_ref* ref);
	bool ConstructBodyForSending( BString& msgText);
	void SetEditableText( const BString& text, uint32 encoding);
	uint32 DefaultEncoding()	const;

	//	overrides of listmodel base:
	bool StartJob();
	const BString SettingsFileName()		{ return NULL; }
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline status_t InitCheck()					{ return mInitCheck; }
	inline BmRef<BmBodyPart> EditableTextBody() const { return mEditableTextBody.Get(); }
	inline const BString& Signature() const	{ return mSignature; }
	inline const BmMail* Mail() const			{ return mMail; }
	bool IsMultiPart() const;

	// setters:
	inline void EditableTextBody( BmBodyPart* b) { mEditableTextBody = b; }
	inline void Signature( const BString& s)		{ mSignature = s; }

private:
	BmMail* mMail;
	BmRef<BmBodyPart> mEditableTextBody;
	status_t mInitCheck;
	BString mSignature;						// signature (as found in mail-text)

	// Hide copy-constructor and assignment:
	BmBodyPartList( const BmBodyPartList&);
	BmBodyPartList operator=( const BmBodyPartList&);
};


#endif
