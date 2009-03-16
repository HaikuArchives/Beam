/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "BmDaemon.h"

#include "BmBasics.h"
#include "BmString.h"

#include "BmImapNestedStringList.h"

static BmImapNestedStringList EmptyStringList;

/*------------------------------------------------------------------------------*\
	BmImapNestedStringList()
		-	contructor
\*------------------------------------------------------------------------------*/
BmImapNestedStringList::BmImapNestedStringList()
{
}

/*------------------------------------------------------------------------------*\
	~BmImapNestedStringList()
		-	destructor
\*------------------------------------------------------------------------------*/
BmImapNestedStringList::~BmImapNestedStringList()
{
}

/*------------------------------------------------------------------------------*\
	operator[]
		-	
\*------------------------------------------------------------------------------*/
const BmImapNestedStringList& 
BmImapNestedStringList::operator[](uint32 index) const
{
	if (index >= mChildren.size())
		return EmptyStringList;
	else
		return mChildren[index];
}

/*------------------------------------------------------------------------------*\
	Parse()
		-	parses a string list from the given data pointer and returns the
			position where the parsing has stopped (which is either at the end
			of the string or directly behind a closing parenthesis).
\*------------------------------------------------------------------------------*/
const char* BmImapNestedStringList::Parse(const char* data)
{
	mChildren.clear();
	if (!data)
		return NULL;

	if (*data == '(')
		return _Parse(data + 1);

	// parse error, we should just be seeing the initial "(" here
	return NULL;
}

/*------------------------------------------------------------------------------*\
	_Parse()
		-	
\*------------------------------------------------------------------------------*/
const char* BmImapNestedStringList::_Parse(const char* data)
{
	char c;
	while((c = *data) != '\0') {
		BmImapNestedStringList subList;
		switch(c) {
			case '(':
				data = subList.Parse(data);
				mChildren.push_back(subList);
				break;
			case ')':
				return data + 1;
			case '"':
				data = subList._ParseQuotedString(data);
				mChildren.push_back(subList);
				break;
			case ' ':
				data++;
				break;
			default:
				data = subList._ParseString(data);
				mChildren.push_back(subList);
				break;
		}
	}
	return data;
}

/*------------------------------------------------------------------------------*\
	_ParseQuotedString()
		-	parses a string enclosed by "" and returns the position following
			the closing quotation mark
\*------------------------------------------------------------------------------*/
const char* BmImapNestedStringList::_ParseQuotedString(const char* data)
{
	char c;
	const char* start = data;
	while((c = *data) != '\0') {
		if (c == '"') {
			mText.SetTo(start, data - start);
			return data + 1;
		}
		data++;
	}
	return data;
}

/*------------------------------------------------------------------------------*\
	_ParseString()
		-	parses a (non-quoted) string and returns the position of the char
			that follows the string (end-of-string, a space or a closing 
			parenthesis)
\*------------------------------------------------------------------------------*/
const char* BmImapNestedStringList::_ParseString(const char* data)
{
	char c;
	const char* start = data;
	while((c = *data) != '\0') {
		if (c == ' ' || c == ')') {
			mText.SetTo(start, data - start);
			return data;
		}
		data++;
	}
	return data;
}
