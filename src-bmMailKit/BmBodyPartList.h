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

#include "BmMailKit.h"

#include <Entry.h>

#include "BmDataModel.h"
#include "BmMailHeader.h"
#include "BmMemIO.h"

class BmMail;

/*------------------------------------------------------------------------------*\
	BmContentField
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmContentField {
	typedef map< BmString, BmString> BmParamMap;

public:
	// c'tors and d'tor:
	BmContentField()							{ mInitCheck = B_NO_INIT; }
	BmContentField( const BmString cfString);
	
	// native methods:
	void SetTo( const BmString cfString);
	void SetParam( BmString key, BmString value);

	// getters:
	inline status_t InitCheck() const	{ return mInitCheck; }
	inline const BmString& Value() const	{ return mValue; }
	const BmString& Param( BmString key) const;

	// operators:
	operator BmString() const;
							// returns contentfield completely formatted 
							// (ready to be sent)

private:
	BmString mValue;
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
class IMPEXPBMMAILKIT BmBodyPart : public BmListModelItem {
	typedef BmListModelItem inherited;

	static const int16 nArchiveVersion = 1;
	
	friend BmBodyPartList;

public:
	// c'tors and d'tor:
	BmBodyPart( BmBodyPartList* model, const BmString& msgtext, int32 s, int32 l,
					const BmString& defaultCharset,
					BmRef<BmMailHeader> mHeader=NULL, BmListModelItem* parent=NULL);
	BmBodyPart( BmBodyPartList* model, const entry_ref* ref, 
					const BmString& defaultCharset,
					BmListModelItem* parent=NULL);
	BmBodyPart( const BmBodyPart& bodyPart);
	~BmBodyPart();

	// class methods:
	static BmString NextObjectID();
	static bool MimeTypeIsPotentiallyHarmful( const BmString& realMT);

	// native methods:
	void SetTo( const BmString& msgtext, int32 s, int32 l, 
					const BmString& defaultCharset,
					BmRef<BmMailHeader> mHeader=NULL);
	void SetBodyText( const BmString& utf8Text, const BmString& charset);
	bool IsText() const;
	bool IsPlainText() const;
	bool ShouldBeShownInline()	const;
	entry_ref WriteToTempFile( BmString filename="");
	void WriteToFile( BFile& file);
	void SaveAs( const entry_ref& destDirRef, BmString filename);
	void SuggestCharset( const BmString& s) { mSuggestedCharset = s; }

	static BmString GenerateBoundary();

	// overrides of listmodelitem base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline bool IsMultiPart() const		{ return mIsMultiPart; }
	const BmString& DecodedData() const;
	inline int32 DecodedLength() const	{ return DecodedData().Length(); }
	inline status_t InitCheck() const	{ return mInitCheck; }

	inline const BmString ContentTypeAsString() const	
													{ return mContentType; }
	inline const BmString ContentDispositionAsString() const	
													{ return mContentDisposition; }

	inline const BmString& MimeType() const		
													{ return mContentType.Value(); }
	inline const BmString& Disposition() const	
													{ return mContentDisposition.Value(); }
	inline const BmString& FileName() const		
													{ return mFileName; }
	inline const BmString& TransferEncoding() const	
													{ return mContentTransferEncoding; }
	inline const BmString& ID() const	
													{ return mContentId; }
	inline const BmString& Description() const	
													{ return mContentDescription; }
	inline const BmString& Language() const		
													{ return mContentLanguage; }
	inline const BmString& TypeParam( BmString key) const		
													{ return mContentType.Param( key); }
	inline const BmString& DispositionParam( BmString key) const	
													{ return mContentDisposition
																	.Param( key); }
	inline int32 BodyLength() const		{ return mBodyLength; }
	inline const BmString& SuggestedCharset() const		
													{ return mSuggestedCharset; }
	inline const BmString& CurrentCharset() const		
													{ return mCurrentCharset; }
	inline bool HadErrorDuringConversion() const			
													{ return mHadErrorDuringConversion; }

	inline const entry_ref& EntryRef() const	
													{ return mEntryRef; }

	inline const bool Is7Bit() const		{ return mContentTransferEncoding
																	.ICompare( "7bit") == 0; }
	inline const bool Is8Bit() const		{ return mContentTransferEncoding
																	.ICompare( "8bit") == 0; }
	inline const bool IsBinary() const	{ return mContentTransferEncoding
																	.ICompare( "binary") == 0; }

	static int32 nBoundaryCounter;

private:
	bool ContainsRef( const entry_ref& ref) const;
	void PropagateHigherEncoding();
	int32 PruneUnneededMultiParts();
	int32 EstimateEncodedSize();
	void ConstructBodyForSending( BmStringOBuf &msgText);

	bool mIsMultiPart;
	BmContentField mContentType;
	BmString mContentTransferEncoding;
	BmString mContentId;
	BmContentField mContentDisposition;
	BmString mContentDescription;
	BmString mContentLanguage;
	BmString mFileName;

	mutable bool mHaveDecodedData;
	mutable BmString mDecodedData;
	int32 mStartInRawText;
	int32 mBodyLength;
	
	mutable BmString mCurrentCharset;
	BmString mSuggestedCharset;
	mutable bool mHadErrorDuringConversion;
	
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
class IMPEXPBMMAILKIT BmBodyPartList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion = 1;

public:
	// c'tors and d'tor
	BmBodyPartList( BmMail* mail);
	virtual ~BmBodyPartList();

	// native methods:
	void ParseMail();
	bool HasAttachments() const;
	void AddAttachmentFromRef( const entry_ref* ref,
										const BmString& defaultCharset);
	void PruneUnneededMultiParts();
	int32 EstimateEncodedSize();
	bool ConstructBodyForSending( BmStringOBuf& msgText);
	void SetEditableText( const BmString& utf8Text, const BmString& charset);
	const BmString& DefaultCharset()	const;

	//	overrides of listmodel base:
	bool StartJob();
	const BmString SettingsFileName()	{ return NULL; }
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	void RemoveItemFromList( BmListModelItem* item);

	// getters:
	inline status_t InitCheck()			{ return mInitCheck; }
	inline BmRef<BmBodyPart> EditableTextBody() const 
													{ return mEditableTextBody.Get(); }
	inline const BmString& Signature() const	
													{ return mSignature; }
	inline BmMail* Mail() const			{ return mMail; }
	bool IsMultiPart() const;

	// setters:
	inline void EditableTextBody( BmBodyPart* b) 
													{ mEditableTextBody = b; }
	inline void Signature( const BmString& s)		
													{ mSignature = s; }

private:
	BmMail* mMail;
	BmRef<BmBodyPart> mEditableTextBody;
	status_t mInitCheck;
	BmString mSignature;						// signature (as found in mail-text)

	// Hide copy-constructor and assignment:
	BmBodyPartList( const BmBodyPartList&);
	BmBodyPartList operator=( const BmBodyPartList&);
};


#endif
