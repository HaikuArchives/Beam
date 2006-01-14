/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmFilterAddon_h
#define _BmFilterAddon_h

#include <Message.h>

#include "BmBase.h"
#include "BmString.h"

class BmMail;
struct IMPEXPBMBASE BmHeaderInfo {
	BmString fieldName;
	const char** values;
};
/*------------------------------------------------------------------------------*\
	BmMsgContext
		-	
\*------------------------------------------------------------------------------*/
struct IMPEXPBMBASE BmMsgContext {
	BmMsgContext();
	~BmMsgContext();

	// info fields, are set before filtering starts:
	BmMail* mail;
	int32 headerInfoCount;
	BmHeaderInfo *headerInfos;
	
	void ResetChanges();
	bool FieldHasChanged(const char* fieldName) const;

	void ResetData();
	bool HasField(const char* fieldName) const;

	void SetInt32(const char* fieldName, int32 value);
	int32 GetInt32(const char* fieldName) const;

	void SetString(const char* fieldName, const char* value);
	const char* GetString(const char* fieldName) const;
	
	void SetBool(const char* fieldName, bool value);
	bool GetBool(const char* fieldName) const;

	void SetDouble(const char* fieldName, double value);
	double GetDouble(const char* fieldName) const;

private:
	void NoteChange(const char* fieldName);

	// data message that contains input & output data:
	BMessage mDataMsg;
	// status message that notes changes to any of the fields:
	BMessage mStatusMsg;

	// Hide copy-constructor:
	BmMsgContext( const BmMsgContext&);
};



/*------------------------------------------------------------------------------*\
	BmFilterAddon 
		-	base class for all filter-addons, this is used as filter-addon-API
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmFilterAddon {

public:
	BmFilterAddon();
	virtual ~BmFilterAddon();
	
	// native methods:
	virtual bool Execute( BmMsgContext* msgContext, 
								 const BMessage* jobSpecs = NULL) = 0;
	virtual void Initialize()				{}
	virtual bool SanityCheck( BmString& complaint, BmString& fieldName) = 0;
	virtual status_t Archive( BMessage* archive, bool deep = true) const = 0;
	virtual BmString ErrorString() const = 0;

	virtual void ForeignKeyChanged( const BmString& /* key */, 
											  const BmString& /* oldVal */, 
											  const BmString& /* newVal */) 
											  		{}

	virtual void SetupFromMailData( const BmString& /* subject */, 
											  const BmString& /* from */, 
											  const BmString& /* To */)	  
											  		{}

	// foreign-key identifiers:
	static const char* const FK_FOLDER;
	static const char* const FK_IDENTITY;

private:

	// Hide copy-constructor:
	BmFilterAddon( const BmFilterAddon&);
};

typedef BmFilterAddon* (*BmInstantiateFilterFunc)( const BmString& name, 
																	const BMessage* archive,
																	const BmString& kind);

class BmFilterAddonPrefsView;
typedef BmFilterAddonPrefsView* (*BmInstantiateFilterPrefsFunc)( 
	float minX, float minY, float maxX, float maxY, const BmString& kind
);

#endif
