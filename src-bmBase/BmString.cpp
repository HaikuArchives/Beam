//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		String.cpp
//	Author(s):		Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//
//	Modified for Beam:		Oliver Tappe (beam@hirschkaefer.de)
//
//	Description:	String class supporting common string operations.  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algobase.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------

#include <Debug.h>
#include <UTF8.h>

#include "BmString.h"
#include "BmMemIO.h"

#define KEEP_CASE false
#define IGNORE_CASE true

// stuff missing from libc:
char *
strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return NULL;
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return (char *)s;
}

/*---- Construction --------------------------------------------------------*/
BmString::BmString()
	:_privateData(NULL)	
{
}


BmString::BmString(const char* str)
	:_privateData(NULL)
{
	if (str)
		_Init(str, strlen(str));
}


BmString::BmString(const BmString &string)
	:_privateData(NULL)			
{
	_Init(string.String(), string.Length());
}


BmString::BmString(const char *str, int32 maxLength)
	:_privateData(NULL)		
{
	if (str) {
		int32 len = 0;
		while( len<maxLength && str[len])
			len++;
		_Init(str, min(len, maxLength));
	}
}


/*---- Destruction ---------------------------------------------------------*/
BmString::~BmString()
{
	if (_privateData)
		free(_privateData - sizeof(int32));
}


/*---- Access --------------------------------------------------------------*/
int32
BmString::CountChars() const
{
	int32 count = 0;
	const char *ptr = String();

	while (*ptr++)
	{
		count++;

		// Jump to next UTF8 character
		 for ( ; (*ptr & 0xc0) == 0x80; ptr++);
	}

	return count;
}


/*---- Assignment ----------------------------------------------------------*/
BmString&
BmString::operator=(const BmString &string)
{
	_DoAssign(string.String(), string.Length());
	return *this;
}


BmString&
BmString::operator=(const char *str)
{
	if (str)
		_DoAssign(str, strlen(str));	
	else
		_GrowBy(-Length()); // Empties the string
	
	return *this;
}


BmString&
BmString::operator=(char c)
{
	_DoAssign(&c, 1);
	return *this;
}


BmString&
BmString::SetTo(const char *str, int32 length)
{
	if (str) {
		int32 len=0;
		while( len<length && str[len])
			len++;
		_DoAssign(str, min(length, len));
	} else 
		_GrowBy(-Length()); // Empties the string
	
	return *this;
}


BmString&
BmString::SetTo(const BmString &from)
{
	_DoAssign(from.String(), from.Length());
	return *this;
}


BmString&
BmString::Adopt(BmString &from)
{
	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = from._privateData;
	from._privateData = NULL;

	return *this;
}


BmString&
BmString::SetTo(const BmString &string, int32 length)
{
	_DoAssign(string.String(), min(length, string.Length()));
	return *this;
}


BmString&
BmString::Adopt(BmString &from, int32 length)
{
	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = from._privateData;
	from._privateData = NULL;

	if (length < Length())
		_privateData = _GrowBy(length - Length()); //Negative

	return *this;
}


BmString&
BmString::SetTo(char c, int32 count)
{
	_GrowBy(count - Length());
	memset(_privateData, c, count);
	
	return *this;	
}


/*---- Substring copying ---------------------------------------------------*/
BmString &BmString::CopyInto(BmString &into, int32 fromOffset, int32 length) const
{
	if (&into != this)
		into.SetTo(String() + fromOffset, length);
	return into;
}


void
BmString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	if (into) {
		int32 len = Length() - fromOffset;
		len = min(len, length);
		memcpy(into, _privateData + fromOffset, len);
	}
}


/*---- Appending -----------------------------------------------------------*/
BmString&
BmString::operator+=(const char *str)
{
	if (str)
		_DoAppend(str, strlen(str));
	return *this;
}


BmString&
BmString::operator+=(char c)
{
	_DoAppend(&c, 1);
	return *this;
}


BmString&
BmString::Append(const BmString &string, int32 length)
{
	_DoAppend(string.String(), min(length, string.Length()));
	return *this;
}


BmString&
BmString::Append(const char *str, int32 length)
{
	if (str) {
		int32 len=0;
		while( len<length && str[len])
			len++;
		_DoAppend(str, min(len, length));
	}	
	return *this;
}


BmString&
BmString::Append(char c, int32 count)
{
	int32 len = Length();
	_GrowBy(count);
	memset(_privateData + len, c, count);

	return *this;
}


/*---- Prepending ----------------------------------------------------------*/
BmString&
BmString::Prepend(const char *str)
{
	if (str)
		_DoPrepend(str, strlen(str));
	return *this;
}


BmString&
BmString::Prepend(const BmString &string)
{
	if (&string != this)
		_DoPrepend(string.String(), string.Length());
	return *this;
}


BmString&
BmString::Prepend(const char *str, int32 length)
{
	if (str) {
		int32 len=0;
		while( len<length && str[len])
			len++;
		_DoPrepend(str, min(len, length));		
	}
	return *this;
}


BmString&
BmString::Prepend(const BmString &string, int32 len)
{
	if (&string != this)
		_DoPrepend(string.String(), min(len, string.Length()));
	return *this;
}


BmString&
BmString::Prepend(char c, int32 count)
{
	_OpenAtBy(0, count);
	memset(_privateData, c, count);
	
	return *this;
}


/*---- Inserting ----------------------------------------------------------*/
BmString&
BmString::Insert(const char *str, int32 pos)
{
	if (str) {
		if (pos < 0) {
			str -= pos;
			pos = 0;
		}
		int32 len = (int32)strlen(str);
		_privateData = _OpenAtBy(pos, len);
		memcpy(_privateData + pos, str, len);
	}
	return *this;
}


BmString&
BmString::Insert(const char *str, int32 length, int32 pos)
{
	if (str) {
		if (pos < 0) {
			str -= pos;
			pos = 0;
		}
		int32 len=0;
		while( len<length && str[len])
			len++;
		len = min(len, length);
		_privateData = _OpenAtBy(pos, len);
		memcpy(_privateData + pos, str, len);
	}
	return *this;
}


BmString&
BmString::Insert(const char *str, int32 fromOffset, int32 length, int32 pos)
{
	if (str) {
		int32 len=0;
		while( len<length && str[fromOffset+len])
			len++;
		len = min(len, length);
		_privateData = _OpenAtBy(pos, len);
		memcpy(_privateData + pos, str + fromOffset, len);
	}
	return *this;
}


BmString&
BmString::Insert(const BmString &string, int32 pos)
{
	if (&string != this)
		Insert(string.String(), pos); //TODO: Optimize
	return *this;				  
}


BmString&
BmString::Insert(const BmString &string, int32 length, int32 pos)
{
	if (&string != this)
		Insert(string.String(), length, pos); //TODO: Optimize
	return *this;
}


BmString&
BmString::Insert(const BmString &string, int32 fromOffset, int32 length, int32 pos)
{
	if (&string != this)
		Insert(string.String(), fromOffset, length, pos); //TODO: Optimize
	return *this;
}


BmString&
BmString::Insert(char c, int32 count, int32 pos)
{
	_OpenAtBy(pos, count);
	memset(_privateData + pos, c, count);
	
	return *this;
}


/*---- Removing -----------------------------------------------------------*/
BmString&
BmString::Truncate(int32 newLength, bool lazy)
{
	if (newLength < 0)
		return *this;
		
	if (newLength < Length()) {
		if (lazy)
			_SetLength( newLength);
		else
			_privateData = _GrowBy(newLength - Length()); //Negative	
		_privateData[Length()] = '\0';
	}
	return *this;
}


BmString&
BmString::Remove(int32 from, int32 length)
{
	_ShrinkAtBy(from, length);
	return *this;
}


BmString&
BmString::RemoveFirst(const BmString &string)
{
	int32 pos = _ShortFindAfter(string.String(), string.Length());
	
	if (pos >= 0)
		_privateData = _ShrinkAtBy(pos, string.Length());
	
	return *this;
}


BmString&
BmString::RemoveLast(const BmString &string)
{
	int32 pos = _FindBefore(string.String(), Length(), string.Length());
	
	if (pos >= 0)
		_privateData = _ShrinkAtBy(pos, string.Length());
		
	return *this;
}


BmString&
BmString::RemoveAll(const BmString &string)
{
	return _DoReplace( string.String(), "", 0x7FFFFFFF, 0, KEEP_CASE);
}


BmString&
BmString::RemoveFirst(const char *str)
{
	if (str) {
		int32 len = (int32)strlen(str);
		int32 pos = _ShortFindAfter(str, len);
		if (pos >= 0)
			_ShrinkAtBy(pos, len);
	}
	return *this;
}


BmString&
BmString::RemoveLast(const char *str)
{
	if (str) {
		int32 len = strlen(str);
		int32 pos = _FindBefore(str, Length(), len);
		if (pos >= 0)
			_ShrinkAtBy(pos, len);
	}
	return *this;
}


BmString&
BmString::RemoveAll(const char *str)
{
	return _DoReplace( str, "", 0x7FFFFFFF, 0, KEEP_CASE);
}


BmString&
BmString::RemoveSet(const char *setOfCharsToRemove)
{
	char* buf;
	if (setOfCharsToRemove && (buf=LockBuffer( Length())) != NULL) {
		char* oldPos = buf;
		char* newPos = buf;
		int32 len;
		int32 lenToGo = Length();
		while( (len = strcspn( oldPos, setOfCharsToRemove)) < lenToGo) {
			if (oldPos>newPos)
				memmove( newPos, oldPos, len);
			newPos += len++;
			oldPos += len;
			lenToGo -= len;
		}
		if (oldPos>newPos)
			memmove( newPos, oldPos, lenToGo);
		newPos += lenToGo;
		*newPos = 0;
		UnlockBuffer( newPos-buf);
	}
	return *this;
}

BmString&
BmString::MoveInto(BmString &into, int32 from, int32 length)
{
	int32 len = Length() - from;
	
	len = min(len, length);
	
	into.SetTo(String() + from, length);
	
	if (from + length <= Length())
		_privateData = _ShrinkAtBy(from, len);

	return *this;
}


void
BmString::MoveInto(char *into, int32 from, int32 length)
{
	if (into) {
		memcpy(into, String() + from, length);
		if (from + length <= Length())
			_privateData = _ShrinkAtBy(from, length);
		into[length] = '\0';
	}		 
}


/*---- Compare functions ---------------------------------------------------*/
bool
BmString::operator<(const char *string) const
{
	if (!string)
		return false;
	return strcmp(String(), string) < 0;
}


bool
BmString::operator<=(const char *string) const
{
	if (!string)
		return false;
	return strcmp(String(), string) <= 0;
}


bool
BmString::operator==(const char *string) const
{
	if (!string)
		return false;
	return strcmp(String(), string) == 0;
}


bool
BmString::operator>=(const char *string) const
{
	if (!string)
		return true;
	return strcmp(String(), string) >= 0;
}


bool
BmString::operator>(const char *string) const
{
	if (!string)
		return true;
	return strcmp(String(), string) > 0;
}


/*---- strcmp-style compare functions --------------------------------------*/
int
BmString::Compare(const BmString &string) const
{
	return strcmp(String(), string.String());
}


int
BmString::Compare(const char *string) const
{
	if (!string)
		return 1;
	return strcmp(String(), string);
}


int
BmString::Compare(const BmString &string, int32 n) const
{
	return strncmp(String(), string.String(), n);
}


int
BmString::Compare(const char *string, int32 n) const
{
	if (!string)
		return 1;
	return strncmp(String(), string, n);
}


int
BmString::ICompare(const BmString &string) const
{
	return strcasecmp(String(), string.String());
}


int
BmString::ICompare(const char *str) const
{
	if (!str)
		return 1;
	return strcasecmp(String(), str);
}


int
BmString::ICompare(const BmString &string, int32 n) const
{
	return strncasecmp(String(), string.String(), n);
}


int
BmString::ICompare(const char *str, int32 n) const
{
	if (!str)
		return 1;
	return strncasecmp(String(), str, n);
}


/*---- Searching -----------------------------------------------------------*/
int32
BmString::FindFirst(const BmString &string) const
{
	return _ShortFindAfter(string.String(), string.Length());
}


int32
BmString::FindFirst(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _ShortFindAfter(string, strlen(string));
}


int32
BmString::FindFirst(const BmString &string, int32 fromOffset) const
{
	return _FindAfter(string.String(), fromOffset, string.Length());
}


int32
BmString::FindFirst(const char *string, int32 fromOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _FindAfter(string, fromOffset, strlen(string));
}


int32
BmString::FindFirst(char c) const
{
	char tmp[2] = { c, '\0' };
	
	return _ShortFindAfter(tmp, 1);	
}


int32
BmString::FindFirst(char c, int32 fromOffset) const
{
	char tmp[2] = { c, '\0' };
	
	return _FindAfter(tmp, fromOffset, 1);	
}


int32
BmString::FindLast(const BmString &string) const
{
	return _FindBefore(string.String(), Length(), string.Length());
}


int32
BmString::FindLast(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _FindBefore(string, Length(), strlen(string));
}


int32
BmString::FindLast(const BmString &string, int32 beforeOffset) const
{
	return _FindBefore(string.String(), beforeOffset, string.Length()); 
}


int32
BmString::FindLast(const char *string, int32 beforeOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _FindBefore(string, beforeOffset, strlen(string));
}


int32
BmString::FindLast(char c) const
{
	char tmp[2] = { c, '\0' };
	
	return _FindBefore(tmp, Length(), 1);
}


int32
BmString::FindLast(char c, int32 beforeOffset) const
{
	char tmp[2] = { c, '\0' };
	
	return _FindBefore(tmp, beforeOffset, 1);	
}


int32
BmString::IFindFirst(const BmString &string) const
{
	return _IFindAfter(string.String(), 0, string.Length());
}


int32
BmString::IFindFirst(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _IFindAfter(string, 0, strlen(string));
}


int32
BmString::IFindFirst(const BmString &string, int32 fromOffset) const
{
	return _IFindAfter(string.String(), fromOffset, string.Length());
}


int32
BmString::IFindFirst(const char *string, int32 fromOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _IFindAfter(string, fromOffset, strlen(string));
}


int32
BmString::IFindLast(const BmString &string) const
{
	return _IFindBefore(string.String(), Length(), string.Length());
}


int32
BmString::IFindLast(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _IFindBefore(string, Length(), strlen(string));
}


int32
BmString::IFindLast(const BmString &string, int32 beforeOffset) const
{
	return _IFindBefore(string.String(), beforeOffset, string.Length());
}


int32
BmString::IFindLast(const char *string, int32 beforeOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _IFindBefore(string, beforeOffset, strlen(string));
}


/*---- Replacing -----------------------------------------------------------*/
BmString&
BmString::ReplaceFirst(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _ShortFindAfter(tmp, 1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BmString&
BmString::ReplaceLast(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _FindBefore(tmp, Length(), 1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BmString&
BmString::ReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	int32 pos = B_ERROR;
	char tmp[2] = { replaceThis, '\0' };
	
	for (;;) {
		pos = _FindAfter(tmp, fromOffset, 1);
		if (pos < 0)
			break;
		_privateData[pos] = withThis;
		fromOffset = pos;
	}
	
	return *this;
}


BmString&
BmString::Replace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	char tmp[2] = { replaceThis, '\0' };
	
	if (maxReplaceCount <= 0)
		return *this;
	
	for (int32 pos ; maxReplaceCount > 0 ; maxReplaceCount--) {
		
		pos = _FindAfter(tmp, fromOffset, 1);
		if (pos < 0)
			break;
		
		_privateData[pos] = withThis;
		fromOffset = pos;
		
	}
	return *this;
}


BmString&
BmString::ReplaceFirst(const char *replaceThis, const char *withThis)
{
	return _DoReplace( replaceThis, withThis, 1, 0, KEEP_CASE);
}


BmString&
BmString::ReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
		
	int32 firstStringLength = strlen(replaceThis);	
	int32 pos = _FindBefore(replaceThis, Length(), firstStringLength);
	
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - firstStringLength;
		
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BmString&
BmString::ReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	return _DoReplace( replaceThis, withThis, 0x7FFFFFFF, fromOffset, KEEP_CASE);
}


BmString&
BmString::Replace(const char *findThis, const char *replaceWith, int32 maxReplaceCount, int32 fromOffset)
{
	return _DoReplace( findThis, replaceWith, maxReplaceCount, fromOffset, KEEP_CASE);
}


BmString&
BmString::IReplaceFirst(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _IFindAfter(tmp, 0, 1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;

	return *this;
}


BmString&
BmString::IReplaceLast(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };	
	int32 pos = _IFindBefore(tmp, Length(), 1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BmString&
BmString::IReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	char tmp[2] = { replaceThis, '\0' };
	
	for (int32 pos;;) {
		pos = _IFindAfter(tmp, fromOffset, 1);
		if (pos < 0)
			break;
		_privateData[pos] = withThis;
		fromOffset = pos;
	}
	return *this;
}


BmString&
BmString::IReplace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	char tmp[2] = { replaceThis, '\0' };
	
	if (_privateData == NULL || maxReplaceCount <= 0)
		return *this;
		
	for (int32 pos ; maxReplaceCount > 0 ; maxReplaceCount--) {
		
		pos = _IFindAfter(tmp, fromOffset, 1);
		if (pos < 0)
			break;
		
		_privateData[pos] = withThis;
		fromOffset = pos;
	}
	return *this;
}


BmString&
BmString::IReplaceFirst(const char *replaceThis, const char *withThis)
{
	return _DoReplace( replaceThis, withThis, 1, 0, IGNORE_CASE);
}


BmString&
BmString::IReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
		
	int32 firstStringLength = strlen(replaceThis);		
	int32 pos = _IFindBefore(replaceThis, Length(), firstStringLength);
	
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - firstStringLength;
		
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BmString&
BmString::IReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	return _DoReplace( replaceThis, withThis, 0x7FFFFFFF, fromOffset, IGNORE_CASE);
}


BmString&
BmString::IReplace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	return _DoReplace( replaceThis, withThis, maxReplaceCount, fromOffset, IGNORE_CASE);
}


BmString&
BmString::ReplaceSet(const char *setOfChars, char with)
{
	int32 offset = 0;
	int32 length = Length();
	
	for (int32 pos;;) {
		pos = strcspn(String() + offset, setOfChars);
		if (pos >= length)
			break;
		
		offset += pos;
		
		if (offset >= Length())
			break;

		_privateData[offset] = with;
	}

	return *this;
}


BmString&
BmString::ReplaceSet(const char *setOfChars, const char *with)
{
	if (setOfChars==NULL || with==NULL)
		return *this; //TODO: do something smart
	
	int32 offset = 0;
	int32 withLen = strlen(with);
	
	for (int32 pos;;) {
		pos = strcspn(String() + offset, setOfChars);
		if (pos >= Length())
			break;
		
		offset += pos;
		
		if (offset >= Length())
			break;
		
		_OpenAtBy(offset, withLen - 1);
		memcpy(_privateData + offset, with, withLen);
		offset += withLen;
	}
	
	return *this;
}


/*---- Unchecked char access -----------------------------------------------*/
char &
BmString::operator[](int32 index)
{
	return _privateData[index];
}


/*---- Fast low-level manipulation -----------------------------------------*/
char*
BmString::LockBuffer(int32 maxLength)
{
	_SetUsingAsCString(true); //debug
	
	int32 len = Length();
	
	if (maxLength > len || (!maxLength && !len))
		_privateData = _GrowBy(maxLength - len);

	return _privateData;
}


BmString&
BmString::UnlockBuffer(int32 length, bool lazy)
{
	_SetUsingAsCString(false); //debug
	
	int32 len = length;

	if (len < 0)
		len = strlen(_privateData);

	if (len != Length()) {
		if (lazy && len < Length())
			Truncate( len, true);
		else
			_privateData = _GrowBy(len - Length());
	}
		
	return *this;
}


/*---- Uppercase<->Lowercase ------------------------------------------------*/
BmString&
BmString::ToLower()
{
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = tolower(_privateData[count]);
	
	return *this;
}


BmString&
BmString::ToUpper()
{			
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = toupper(_privateData[count]);
	
	return *this;
}


BmString&
BmString::Capitalize()
{
	if (_privateData == NULL)
		return *this;
		
	_privateData[0] = toupper(_privateData[0]);
	int32 length = Length();
		
	for (int32 count = 1; count < length; count++)
			_privateData[count] = tolower(_privateData[count]);

	return *this;
}


BmString&
BmString::CapitalizeEachWord()
{
	if (_privateData == NULL)
		return *this;
		
	int32 count = 0;
	int32 length = Length();
		
	do {
		// Find the first alphabetical character	
		for(; count < length; count++) {
			if (isalpha(_privateData[count])) {
				_privateData[count] = toupper(_privateData[count]);
				count++;
				break;
			}
		}
		// Find the first non-alphabetical character,
		// and meanwhile, turn to lowercase all the alphabetical ones
		for(; count < length; count++) {
			if (isalpha(_privateData[count]))
				_privateData[count] = tolower(_privateData[count]);
			else
				break;
		}
	} while (count < length);
				
	return *this;
}


/*----- Escaping and Deescaping --------------------------------------------*/
BmString&
BmString::CharacterEscape(const char *original, const char *setOfCharsToEscape, char escapeWith)
{
	SetTo(original);
	CharacterEscape(setOfCharsToEscape, escapeWith);
	
	return *this;
}


BmString&
BmString::CharacterEscape(const char *setOfCharsToEscape, char escapeWith)
{
	if (setOfCharsToEscape == NULL || _privateData == NULL)
		return *this;
	
	int32 offset = 0;
	
	for(int32 pos;;) {
		pos = strcspn(_privateData + offset, setOfCharsToEscape);
		offset += pos;
		if (offset >= Length())
			break;
		_OpenAtBy(offset, 1);
		memset(_privateData + offset, escapeWith, 1);
		offset += 2;
	}
		
	return *this;
}


BmString&
BmString::CharacterDeescape(const char *original, char escapeChar)
{
	SetTo(original);	
	CharacterDeescape(escapeChar);
		
	return *this;
}


BmString&
BmString::CharacterDeescape(char escapeChar)
{
	if (_privateData == NULL)
		return *this;
		
	char tmp[2] = { escapeChar, '\0' };
	RemoveAll(tmp);
	
	return *this;
}


/*---- Simple sprintf replacement calls ------------------------------------*/
/*---- Slower than sprintf but type and overflow safe ----------------------*/
BmString&
BmString::operator<<(const char *str)
{
	if (str)
		_DoAppend(str, strlen(str));
	return *this;	
}


BmString&
BmString::operator<<(const BmString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}


BmString&
BmString::operator<<(char c)
{
	_DoAppend(&c, 1);	
	return *this;
}


BmString&
BmString::operator<<(int i)
{
	char num[64];
	sprintf(num, "%d", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(unsigned int i)
{
	char num[64];
	sprintf(num, "%u", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(uint32 i)
{
	char num[64];
	sprintf(num, "%lu", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(int32 i)
{
	char num[64];
	sprintf(num, "%ld", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(uint64 i)
{
	char num[64];
#ifdef __POWERPC__
	sprintf(num, "%Lu", i);
#else
	sprintf(num, "%llu", i);
#endif
	return *this << num;
}


BmString&
BmString::operator<<(int64 i)
{
	char num[64];
#ifdef __POWERPC__
	sprintf(num, "%Ld", i);
#else
	sprintf(num, "%lld", i);
#endif
	
	return *this << num;
}


BmString&
BmString::operator<<(float f)
{
	char num[64];
	sprintf(num, "%.2f", f);
	
	return *this << num;
}


/*---- Private or Reserved ------------------------------------------------*/
void
BmString::_Init(const char* str, int32 len)
{
	ASSERT(str != NULL);
	ASSERT(_privateData == NULL);

	_privateData = (char*)malloc(len + sizeof(int32) + 1);
	assert( _privateData);
	_privateData += sizeof(int32);
	
	memcpy(_privateData, str, len);
	
	_SetLength(len);
	_privateData[len] = '\0';	
}


void
BmString::_DoAssign(const char *str, int32 len)
{
	ASSERT(str != NULL);	
	int32 curLen = Length();
	
	if (len != curLen)
		_privateData = _GrowBy(len - curLen);
	
	memcpy(_privateData, str, len);
}


void
BmString::_DoAppend(const char *str, int32 len)
{
	ASSERT(str != NULL);
	
	int32 length = Length();
	_privateData = _GrowBy(len);
	memcpy(_privateData + length, str, len);
}


char*
BmString::_GrowBy(int32 size)
{		
	int32 curLen = Length();
	ASSERT(curLen + size >= 0);
	
	if (_privateData != NULL)
	{
		_privateData -= sizeof(int32);
	}
		
	_privateData = (char*)realloc(_privateData, 
		curLen + size + sizeof(int32) + 1);
		
	assert( _privateData);

	_privateData += sizeof(int32);
	
	_SetLength(curLen + size);	
	_privateData[Length()] = '\0';
	
	return _privateData;
}


char *
BmString::_OpenAtBy(int32 offset, int32 length)
{
	ASSERT(offset >= 0);
			
	int32 oldLength = Length();
	
	if (_privateData != NULL)
		_privateData -= sizeof(int32);
	
	_privateData = (char*)realloc(_privateData , oldLength + length + sizeof(int32) + 1);
	assert( _privateData);
	_privateData += sizeof(int32);
	
	memmove(_privateData + offset + length, _privateData + offset,
			oldLength - offset);
	
	_SetLength(oldLength + length);
	_privateData[Length()] = '\0';
	
	return _privateData;	
}


char*
BmString::_ShrinkAtBy(int32 offset, int32 length)
{	
	int32 oldLength = Length();

	if (offset > oldLength || offset + length > oldLength)
		return _privateData;

	memmove(_privateData + offset, _privateData + offset + length,
		Length() - offset - length);
	
	_privateData -= sizeof(int32);	
	_privateData = (char*)realloc(_privateData, oldLength - length + sizeof(int32) + 1);
	assert( _privateData);
	_privateData += sizeof(int32);
	
	_SetLength(oldLength - length);	
	_privateData[Length()] = '\0';
	
	return _privateData;
}


void
BmString::_DoPrepend(const char *str, int32 count)
{
	ASSERT(str != NULL);
	_privateData = _OpenAtBy(0, count);
	memcpy(_privateData, str, count);
}


int32
BmString::_FindAfter(const char *str, int32 offset, int32) const
{	
	ASSERT(str != NULL);
	
	if (offset > Length())
		return B_ERROR;

	char *ptr = strstr(String() + offset, str);

	if (ptr != NULL)
		return ptr - String();
	
	return B_ERROR;
}


int32
BmString::_IFindAfter(const char *str, int32 offset, int32) const
{
	ASSERT(str != NULL);

	if (offset > Length())
		return B_ERROR;

	char *ptr = strcasestr(String() + offset, str);

	if (ptr != NULL)
		return ptr - String();

	return B_ERROR;
}


int32
BmString::_ShortFindAfter(const char *str, int32) const
{
	ASSERT(str != NULL);
	
	char *ptr = strstr(String(), str);

	if (ptr != NULL)
		return ptr - String();
	
	return B_ERROR;
}


int32
BmString::_FindBefore(const char *str, int32 offset, int32 strlen) const
{
	ASSERT(str != NULL);
	
	if (offset <= 0)
		return B_ERROR;
			
	char *ptr1 = _privateData + offset - strlen;
	
	while (ptr1 >= _privateData) {
		if (!memcmp(ptr1, str, strlen))
			return ptr1 - _privateData; 
		ptr1--;
	}
	
	return B_ERROR;
}


int32
BmString::_IFindBefore(const char *str, int32 offset, int32 strlen) const
{
	ASSERT(str != NULL);
	
	if (offset <= 0)
		return B_ERROR;
			
	char *ptr1 = _privateData + offset - strlen;
	
	while (ptr1 >= _privateData) {
		if (!strncasecmp(ptr1, str, strlen))
			return ptr1 - _privateData; 
		ptr1--;
	}
	
	return B_ERROR;
}


BmString&
BmString::_DoReplace(const char *findThis, const char *replaceWith, int32 maxReplaceCount, 
							int32 fromOffset,	bool ignoreCase)
{
	if (findThis == NULL || maxReplaceCount <= 0 || fromOffset < 0 || fromOffset >= Length())
		return *this;
	
	typedef int32 (BmString::*TFindMethod)(const char *, int32, int32) const;
	TFindMethod findMethod = ignoreCase
		? &BmString::_IFindAfter
		: &BmString::_FindAfter;
	int32 findLen = strlen( findThis);
	int32 replaceLen = replaceWith ? strlen( replaceWith) : 0;
	BmStringOBuf tempIO( (int32)MAX( MAX( findLen, 128), Length()*1.2), 1.2);
	int32 lastSrcPos = fromOffset;
	int32 len;
	for(  int32 srcPos=0; 
			maxReplaceCount>0 && (srcPos = (this->*findMethod)( findThis, lastSrcPos, findLen))!=B_ERROR; 
			maxReplaceCount-- ) {
		len = srcPos-lastSrcPos;
		if (fromOffset && !tempIO.HasData())
			tempIO.Write( String(), fromOffset);
		tempIO.Write( String()+lastSrcPos, len);
		tempIO.Write( replaceWith, replaceLen);
		lastSrcPos = srcPos+findLen;
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = Length()-lastSrcPos)!=0) {
			if (fromOffset && !tempIO.HasData())
				tempIO.Write( String(), fromOffset);
			tempIO.Write( String()+lastSrcPos, len);
		}
		Adopt( tempIO.TheString());
	}
	return *this;
}

void
BmString::_SetLength(int32 length)
{
	*((int32*)_privateData - 1) = length & 0x7fffffff;
}


#if DEBUG
// AFAIK, these are not implemented in BeOS R5
void
BmString::_SetUsingAsCString(bool state)
{		
}


void
BmString::_AssertNotUsingAsCString() const
{
}
#endif


/*----- Non-member compare for sorting, etc. ------------------------------*/
int
Compare(const BmString &string1, const BmString &string2)
{
	return strcmp(string1.String(), string2.String());
}


int
ICompare(const BmString &string1, const BmString &string2)
{
	return strcasecmp(string1.String(), string2.String());
}


int
Compare(const BmString *string1, const BmString *string2)
{
	return strcmp(string1->String(), string2->String());
}


int
ICompare(const BmString *string1, const BmString *string2)
{
	return strcasecmp(string1->String(), string2->String());
}

/*----- Non-member concatenations (slow) ----------------------------------*/
BmString operator+(const BmString& s1, const BmString& s2) 
{
	BmString result(s1);
	result += s2;
	return result;
}
BmString operator+(const char* s1, const BmString& s2) 
{
	BmString result(s1);
	result += s2;
	return result;
}
BmString operator+(const BmString& s1, const char* s2) 
{
	BmString result(s1);
	result += s2;
	return result;
}

/*------------------------------------------------------------------------------*\
	ConvertLinebreaksToLF()
		-	converts linebreaks of this string from CRLF to LF
		-	single CRs are not affected
\*------------------------------------------------------------------------------*/
BmString& BmString::ConvertLinebreaksToLF( const BmString* srcData) {
	const BmString* src = srcData ? srcData : this;
	if (!src->Length()) {
		Truncate( 0);
		return *this;
	}
	
	BmStringOBuf tempIO( (int32)MAX( 128, src->Length()), 1.2);
	int32 lastSrcPos = 0;
	int32 len;
	for( int32 srcPos=0; (srcPos=src->_FindAfter( "\r\n", lastSrcPos, 2)) != B_ERROR; ) {
		len = srcPos-lastSrcPos;
		tempIO.Write( src->String()+lastSrcPos, len);
		tempIO.Write( "\n", 1);
		lastSrcPos = srcPos+2;
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = src->Length()-lastSrcPos)!=0)
			tempIO.Write( src->String()+lastSrcPos, len);
		Adopt( tempIO.TheString());
	} else if (srcData)
		this->SetTo( *srcData);
	return *this;
}

/*------------------------------------------------------------------------------*\
	ConvertLinebreaksToCRLF()
		-	converts linebreaks of this string from LF to CRLF
		-	single CRs are not affected
\*------------------------------------------------------------------------------*/
BmString& BmString::ConvertLinebreaksToCRLF( const BmString* srcData) {
	const BmString* src = srcData ? srcData : this;
	if (!src->Length()) {
		Truncate( 0);
		return *this;
	}
	
	BmStringOBuf tempIO( (int32)MAX( 128, src->Length()*1.2), 1.2);
	int32 lastSrcPos = 0;
	int32 len;
	for( int32 srcPos=0; (srcPos=src->_FindAfter( "\n", srcPos, 1)) != B_ERROR; srcPos++) {
		if (!srcPos || src->ByteAt(srcPos-1)!='\r') {
			len = srcPos-lastSrcPos;
			tempIO.Write( src->String()+lastSrcPos, len);
			tempIO.Write( "\r\n", 2);
			lastSrcPos = srcPos+1;
		}
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = src->Length()-lastSrcPos)!=0)
			tempIO.Write( src->String()+lastSrcPos, len);
		Adopt( tempIO.TheString());
	} else if (srcData)
		this->SetTo( *srcData);
	return *this;
}

/*------------------------------------------------------------------------------*\
	ConvertTabsToSpaces( in, out, numSpaces)
		-	converts all tabs of given in-string into numSpaces spaces
		-	result is stored in out
\*------------------------------------------------------------------------------*/
BmString&
BmString::ConvertTabsToSpaces( int32 numSpaces, const BmString* srcData) {
	const BmString* src = srcData ? srcData : this;
	if (!src->Length()) {
		Truncate( 0);
		return *this;
	}
	
	BmString spaces;
	spaces.SetTo( ' ', numSpaces);
	BmStringOBuf tempIO( (int32)MAX( 128, src->Length()*1.2), 1.2);
	int32 lastSrcPos = 0;
	int32 len;
	for( int32 srcPos=0; (srcPos=src->_FindAfter( "\t", lastSrcPos, 1)) != B_ERROR; ) {
		len = srcPos-lastSrcPos;
		tempIO.Write( src->String()+lastSrcPos, len);
		tempIO.Write( spaces.String(), numSpaces);
		lastSrcPos = srcPos+1;
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = src->Length()-lastSrcPos)!=0)
			tempIO.Write( src->String()+lastSrcPos, len);
		Adopt( tempIO.TheString());
	} else if (srcData)
		this->SetTo( *srcData);
	return *this;
}

/*------------------------------------------------------------------------------*\
	DeUrlify()
		-	decodes Url to native string
\*------------------------------------------------------------------------------*/
#define HEXDIGIT2CHAR(d) (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0)
BmString&
BmString::DeUrlify() {
	BmStringOBuf tempIO( (int32)MAX( 128, Length()), 1.2);
	int32 lastSrcPos = 0;
	int32 len;
	char c1, c2;
	for( int32 srcPos=0; (srcPos=_FindAfter( "%", srcPos, 1)) != B_ERROR; srcPos++) {
		if ((c1=toupper(ByteAt(srcPos+1)))!=0
		&& (c1>='A' && c1<='F' || c1>='0' && c1<='9')
		&& (c2=toupper(ByteAt(srcPos+2)))!=0
		&& (c2>='A' && c2<='F' || c2>='0' && c2<='9')) {
			len = srcPos-lastSrcPos;
			tempIO.Write( String()+lastSrcPos, len);
			char native[2] = {HEXDIGIT2CHAR(c1)*16+HEXDIGIT2CHAR(c2), '\0'};
			tempIO.Write( native, 1);
			lastSrcPos = srcPos+3;
		} else if (c1 == '%')
			srcPos++;							// avoid to handle second % of '%%'-sequence
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = Length()-lastSrcPos)!=0)
			tempIO.Write( String()+lastSrcPos, len);
		Adopt( tempIO.TheString());
	}
	return *this;
}
