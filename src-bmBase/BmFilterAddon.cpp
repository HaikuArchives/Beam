/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "BmFilterAddon.h"

/********************************************************************************\
	BmFilterAddon
\********************************************************************************/

const char* const BmFilterAddon::FK_FOLDER =   "bm:folder";
const char* const BmFilterAddon::FK_IDENTITY = "bm:identity";

/*------------------------------------------------------------------------------*\
	BmFilterAddon()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilterAddon::BmFilterAddon() 
{
}

/*------------------------------------------------------------------------------*\
	~BmFilterAddon()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilterAddon::~BmFilterAddon()
{
}



/********************************************************************************\
	BmMsgContext
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMsgContext()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmMsgContext::BmMsgContext()
	:	mail( NULL)
	,	headerInfos( NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmMsgContext()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMsgContext::~BmMsgContext() {
	if (headerInfos) {
		for( int i=0; i<headerInfoCount; ++i)
			delete [] headerInfos[i].values;
		delete [] headerInfos;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::ResetChanges()
{
	mStatusMsg.MakeEmpty();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMsgContext::FieldHasChanged(const char* fieldName) const
{
	bool dummy;
	return mStatusMsg.FindBool(fieldName, &dummy) == B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::ResetData()
{
	mDataMsg.MakeEmpty();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::NoteChange(const char* fieldName)
{
	bool dummy;
	if (mStatusMsg.FindBool(fieldName, &dummy) != B_OK)
		mStatusMsg.AddBool(fieldName, true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMsgContext::HasField(const char* fieldName) const
{
	type_code type;
	return mDataMsg.GetInfo(fieldName, &type) == B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::SetInt32(const char* fieldName, int32 value)
{
	mDataMsg.RemoveName(fieldName);
	mDataMsg.AddInt32(fieldName, value);
	NoteChange(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMsgContext::GetInt32(const char* fieldName) const
{
	return mDataMsg.FindInt32(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::SetString(const char* fieldName, const char* value)
{
	mDataMsg.RemoveName(fieldName);
	mDataMsg.AddString(fieldName, value);
	NoteChange(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const char* BmMsgContext::GetString(const char* fieldName) const
{
	return mDataMsg.FindString(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::SetBool(const char* fieldName, bool value)
{
	mDataMsg.RemoveName(fieldName);
	mDataMsg.AddBool(fieldName, value);
	NoteChange(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMsgContext::GetBool(const char* fieldName) const
{
	return mDataMsg.FindBool(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMsgContext::SetDouble(const char* fieldName, double value)
{
	mDataMsg.RemoveName(fieldName);
	mDataMsg.AddDouble(fieldName, value);
	NoteChange(fieldName);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
double BmMsgContext::GetDouble(const char* fieldName) const
{
	return mDataMsg.FindDouble(fieldName);
}

