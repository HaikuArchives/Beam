/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmSignature_h
#define _BmSignature_h

#include "BmMailKit.h"

#include <Archivable.h>
#include "BmString.h"

#include "BmDataModel.h"

class BmSignatureList;

/*------------------------------------------------------------------------------*\
	BmSignature 
		-	holds information about one signature
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmSignature : public BmListModelItem {
	typedef BmListModelItem inherited;
	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_DYNAMIC;
	static const char* const MSG_CONTENT;
	static const char* const MSG_CHARSET;
	static const int16 nArchiveVersion;

public:
	BmSignature( const char* name, BmSignatureList* model);
	BmSignature( BMessage* archive, BmSignatureList* model);
	virtual ~BmSignature();
	
	// native methods:
	BmString GetSignatureString();

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const BmString &Content() const	{ return mContent; }
	inline bool Dynamic() const 				{ return mDynamic; }
	inline const BmString &Name() const		{ return Key(); }
	inline const BmString &Charset() const	{ return mCharset; }

	// setters:
	inline void Content( const BmString &s) { mContent = s; TellModelItemUpdated( UPD_ALL); }
	inline void Dynamic( bool b) 				{ mDynamic = b;  TellModelItemUpdated( UPD_ALL); }
	inline void Charset( const BmString &s) { mCharset = s; TellModelItemUpdated( UPD_ALL); }

private:
	BmSignature();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmSignature( const BmSignature&);
	BmSignature operator=( const BmSignature&);

	BLocker mAccessLock;				// needed serialize access

	BmString mContent;				// the signature data as entered by user
	bool mDynamic;						// if mContents is static text or a script-call
	BmString mCharset;				// character-set of this sig
};


/*------------------------------------------------------------------------------*\
	BmSignatureList 
		-	holds list of all Signatures
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmSignatureList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmSignatureList* CreateInstance();
	BmSignatureList();
	~BmSignatureList();
	
	// native methods:
	BmString GetSignatureStringFor( const BmString sigName);
	
	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef<BmSignatureList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmSignatureList( const BmSignatureList&);
	BmSignatureList operator=( const BmSignatureList&);
	
};

#define TheSignatureList BmSignatureList::theInstance

#endif
