/*
	BmBodyPartList.h
		$Id$
*/

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
	BmContentField()						{ mInitCheck = B_NO_INIT; }
	BmContentField( const BString cfString);
	
	// native methods:
	void SetTo( const BString cfString);
	void SetParam( BString key, BString value);

	// getters:
	status_t InitCheck() const				{ return mInitCheck; }
	const BString& Value() const			{ return mValue; }
	const BString& Param( BString key) const;

	// operators:
	operator BString() const;
							// returns contentfield completely formatted (ready to be sent)

private:
	BString mValue;
	BmParamMap mParams;

	status_t mInitCheck;

	// Hide copy-constructor and assignment:
	BmContentField( const BmContentField&);
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
					BmMailHeader* mHeader=NULL, BmListModelItem* parent=NULL);
	BmBodyPart( BmBodyPartList* model, const entry_ref* ref, BmListModelItem* parent=NULL);
	~BmBodyPart();

	// class methods:
	static BString BmBodyPart::NextObjectID();

	// native methods:
	void SetTo( const BString& msgtext, int32 s, int32 l, BmMailHeader* mHeader=NULL);
	void SetBodyText( const BString& text, uint32 encoding);
	bool IsText() const;
	bool IsPlainText() const;
	bool ShouldBeShownInline()	const;
	bool ContainsRef( const entry_ref& ref) const;
	entry_ref WriteToTempFile( BString filename="");
	void PropagateHigherEncoding();
	void ConstructBodyForSending( BString &msgText);

	static BString GenerateBoundary();

	// overrides of listmodelitem base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	bool IsMultiPart() const				{ return mIsMultiPart; }
	const BString& DecodedData() const	{ return mDecodedData; }
	int32 DecodedLength() const			{ return mDecodedData.Length(); }
	status_t InitCheck() const				{ return mInitCheck; }

	const BString& Charset() const		{ return mContentType.Param( "charset"); }
	const BString& MimeType() const		{ return mContentType.Value(); }
	const BString& Disposition() const	{ return mContentDisposition.Value(); }
	const BString& FileName() const		{ return mFileName; }
	const BString& TransferEncoding() const	{ return mContentTransferEncoding; }
	const BString& ID() const				{ return mContentId; }
	const BString& Description() const	{ return mContentDescription; }
	const BString& Language() const		{ return mContentLanguage; }
	const BString& TypeParam( BString key) const		{ return mContentType.Param( key); }
	const BString& DispositionParam( BString key) const	{ return mContentDisposition.Param( key); }

	const entry_ref& EntryRef() const	{ return mEntryRef; }

	const bool Is7Bit() const				{ return mContentTransferEncoding.ICompare( "7bit") == 0; }
	const bool Is8Bit() const				{ return mContentTransferEncoding.ICompare( "8bit") == 0; }
	const bool IsBinary() const			{ return mContentTransferEncoding.ICompare( "binary") == 0; }

	static int32 nBoundaryCounter;

private:
	bool mIsMultiPart;
	BString mDecodedData;
	BmContentField mContentType;
	BString mContentTransferEncoding;
	BString mContentId;
	BmContentField mContentDisposition;
	BString mContentDescription;
	BString mContentLanguage;
	BString mFileName;
	
	entry_ref mEntryRef;

	status_t mInitCheck;

	static int32 nObjectID;

	// Hide copy-constructor and assignment:
	BmBodyPart( const BmBodyPart&);
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
	status_t InitCheck()						{ return mInitCheck; }
	BmBodyPart* EditableTextBody() 		{ return mEditableTextBody; }
	const BString& Signature() const		{ return mSignature; }

	// setters:
	void EditableTextBody( BmBodyPart* b) { mEditableTextBody = b; }
	void Signature( const BString& s)	{ mSignature = s; }

private:
	BmMail* mMail;
	BmBodyPart* mEditableTextBody;
	status_t mInitCheck;
	BString mSignature;						// signature (as found in mail-text)

	// Hide copy-constructor and assignment:
	BmBodyPartList( const BmBodyPartList&);
	BmBodyPartList operator=( const BmBodyPartList&);
};


#endif
