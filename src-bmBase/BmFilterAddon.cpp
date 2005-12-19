/*
	BmFilterAddon.cpp

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

