/*
	BmString.cpp
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

#include "BmString.h"
#include "BmMemIO.h"

char* strcasestr(const char *s, const char *find);

BmString BM_DEFAULT_STRING;

// -----------------------------------------------------------------------
// start of OpenBeOS implemenation of BmString
// -----------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// System Includes -------------------------------------------------------------
#include <Debug.h>

// Temporary Includes
//#include "string_helper.h"

#define ENABLE_INLINES 0 // Set this to 1 to make some private methods inline

// define proper names for case-option of _DoReplace()
#define KEEP_CASE false
#define IGNORE_CASE true

// define proper names for count-option of _DoReplace()
#define REPLACE_ALL 0x7FFFFFFF


// helper function, returns minimum of two given values (but clamps to 0):
static inline int32
min_clamp0(int32 num1, int32 num2) 
{ 
	if (num1<num2)
		return num1 > 0 ? num1 : 0;
	else
		return num2 > 0 ? num2 : 0;
}


// helper function, returns length of given string (but clamps to given maximum):
static inline int32
strlen_clamp(const char* str, int32 max) 
{	// this should yield 0 for max<0:
	int32 len=0;
	while( len<max && *str++)
		len++;
	return len;
}


// helper function, massages given pointer into a legal c-string:
static inline const char * 
safestr(const char* str) 
{
	return str ? str : "";
}


// helper class for BmString::_ReplaceAtPositions():
struct
BmString::PosVect {
	PosVect() 
		:
		size(0),
		bufSize(20),
		buf(NULL)
	{
	}
	
	~PosVect()
	{
		free(buf);
	}
	
	bool Add(int32 pos)
	{
		if (!buf || size == bufSize) {
			if (buf)
				bufSize *= 2;
			int32 *newBuf = (int32 *)realloc(buf, bufSize * sizeof(int32));
			if (!newBuf)
				return false;
			buf = newBuf;
		}
		buf[size++] = pos;
		return true;
	}
	
	inline int32 ItemAt(int32 idx) const
	{
		return buf[idx];
	}
	
	inline int32 CountItems() const
	{
		return size;
	}
	
private:
	int32 size;
	int32 bufSize;
	int32 *buf;
};


// helper macro that is used to fall into debugger if a given param check fails:
#ifdef DEBUG
	#define CHECK_PARAM( expr, msg) \
	if (!(expr)) \
		debugger( msg)

	#define CHECK_PARAM_RET( expr, msg, retval) \
	if (!(expr)) \
		debugger( msg)

	#define CHECK_PARAM_VOID( expr, msg) \
	if (!(expr)) \
		debugger( msg)
#else
	#define CHECK_PARAM( expr, msg) \
	if (!(expr)) \
		return *this

	#define CHECK_PARAM_RET( expr, msg, retval) \
	if (!(expr)) \
		return (retval);

	#define CHECK_PARAM_VOID( expr, msg) 
#endif

// -----------------------------------------------------------------------------

/*!
	\class BmString
	\brief String class supporting common string operations
	
	BmString is a string allocation and manipulation class. The object
	takes care to allocate and free memory for you, so it will always be
	"big enough" to store your strings.
	
	\author <a href='mailto:mflerackers@androme.be>Marc Flerackers</a>
	\author <a href='mailto:burton666@freemail.it>Stefano Ceccherini</a>
	\author <a href='mailto:openbeos@hirschaefer.de>Oliver Tappe</a>
*/	

/*!	\var char* BmString::_privateData
	\brief BmString's storage for data
*/

// constructor
/*!	\brief Creates an uninitialized BmString.
*/
BmString::BmString()
	:_privateData(NULL)	
{
}


// constructor
/*! \brief Creates a BmString and initializes it to the given string.
	\param str Pointer to a NULL terminated string.
*/
BmString::BmString(const char* str)
	:_privateData(NULL)
{
	if (str != NULL)
		_Init(str, strlen(str));
}


// copy constructor
/*! \brief Creates a BmString and makes it a copy of the supplied one.
	\param string the BmString object to be copied.
*/
BmString::BmString(const BmString &string)
	:_privateData(NULL)			
{
	_Init(string.String(), string.Length());
}


// constructor
/*! \brief Creates a BmString and initializes it to the given string.
	\param str Pointer to a NULL terminated string.
	\param maxLength The amount of characters you want to copy from the original
		string.
*/
BmString::BmString(const char *str, int32 maxLength)
	:_privateData(NULL)		
{
	if (str != NULL) {
		int32 len = strlen_clamp(str, maxLength);
		_Init(str, len);
	}
}


// destructor
/*! \brief Frees all resources associated with the object.
	
	Frees the memory allocated by the BmString object.
*/
BmString::~BmString()
{
	if (_privateData)
		free(_privateData - sizeof(int32));
}


/*---- Access --------------------------------------------------------------*/
// String, implemented inline in the header
/*! \fn const char* BmString::String() const
	\brief Returns a pointer to the object string, NULL terminated.
	
	Returns a pointer to the object string, guaranteed to be NULL
	terminated. You can't modify or free the pointer. Once the BmString
	object is deleted, the pointer becomes invalid.
	
	\return A pointer to the object string. 
*/


// Length, implemented inline in the header
/*!	\fn int32 BmString::Length() const
	\brief Returns the length of the string, measured in bytes.
	\return The length of the string, measured in bytes.
*/

		
// CountChars
/*! \brief Returns the length of the object measured in characters.
	\return An integer which is the number of characters in the string.
	
	Counts the number of UTF8 characters contained in the string.
*/
int32
BmString::CountChars() const
{
	int32 count = 0;
	
	const char *start = _privateData;
	
	/* String's end. This way we don't have to check for '\0' */
	/* but just compare two pointers (which should be faster) */  
	const char *end = _privateData + Length();
	
#if 0
	// ejaesler: Left in memoriam of one man's foolish disregard for the
	// maxim "Premature optimization is the root of all evil"
	while (*ptr) {
		// Jump to next UTF8 character
		// ejaesler: BGA's nifty function
		ptr += utf8_char_len(*ptr);
		count++;
	}
#endif

	while (start++ != end) {
		count++;

		// Jump to next UTF8 character
		while((*start & 0xc0) == 0x80)
			start++;
	}

	return count;
}


/*---- Assignment ----------------------------------------------------------*/
// equal operator
/*! \brief Makes a copy of the given BmString object.
	\param string The string object to copy.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::operator=(const BmString &string)
{
	if (&string != this) // Avoid auto-assignment
		_DoAssign(string.String(), string.Length());
	return *this;
}


// equal operator
/*! \brief Re-initializes the object to the given string.
	\param str Pointer to a string.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::operator=(const char *str)
{
	if (str != NULL)
		_DoAssign(str, strlen(str));	
	else
		_GrowBy(-Length()); // Empties the string
	
	return *this;
}


// equal operator
/*! \brief Re-initializes the object to the given character.
	\param c The character which you want to initialize the string to.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::operator=(char c)
{
	_DoAssign(&c, 1);
	return *this;
}


// SetTo
/*! \brief Re-initializes the object to the given string.
	\param str Pointer to a string.
	\param length Amount of characters to copy from the original string.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::SetTo(const char *str, int32 length)
{
	if (str != NULL) {
		int32 len = strlen_clamp(str, length);
		_DoAssign(str, len);
	}
	else 
		_GrowBy(-Length()); // Empties the string
	
	return *this;
}


// SetTo
/*! \brief Makes a copy of the given BmString object.
	\param from The string object to copy.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::SetTo(const BmString &from)
{
	if (&from != this) // Avoid auto-assignment
		_DoAssign(from.String(), from.Length());
	return *this;
}


// Adopt
/*! \brief Adopt's data of the given BmString object, freeing the original object.
	\param from The string object to adopt.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::Adopt(BmString &from)
{
	if (&from == this) // Avoid auto-adoption
		return *this;
		
	if (_privateData)
		free(_privateData - sizeof(int32));

	/* "steal" the data from the given BmString */
	_privateData = from._privateData;
	from._privateData = NULL;

	return *this;
}


// SetTo
/*! \brief Makes a copy of the given BmString object.
	\param from The string object to copy.
	\param length Amount of characters to copy from the original BmString.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::SetTo(const BmString &string, int32 length)
{
	if (&string != this) // Avoid auto-assignment
		_DoAssign(string.String(), min_clamp0(length, string.Length()));
	return *this;
}


// Adopt
/*! \brief Adopt's data of the given BmString object, freeing the original object.
	\param from The string object to adopt.
	\param length Amount of characters to get from the original BmString.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::Adopt(BmString &from, int32 length)
{
	if (&from == this) // Avoid auto-adoption
		return *this;

	int32 len = min_clamp0(length, from.Length());

	if (_privateData)
		free(_privateData - sizeof(int32));

	/* "steal" the data from the given BmString */
	_privateData = from._privateData;
	from._privateData = NULL;
	
	if (len < Length())
		_GrowBy(len - Length()); // Negative, we truncate

	return *this;
}


// SetTo
/*! \brief Initializes the object to a string composed by a character you specify.
	\param c The character you want to initialize the BmString.
	\param count The number of characters you want the BmString to be composed by.
	\return
		The function always returns \c *this .
*/
BmString&
BmString::SetTo(char c, int32 count)
{
	if (count < 0)
		count = 0;
	int32 curLen = Length();
	
	if (curLen == count || _GrowBy(count - curLen)) 
		memset(_privateData, c, count);
	return *this;	
}


/*---- Substring copying ---------------------------------------------------*/

// CopyInto
/*! \brief Copy the BmString data (or part of it) into another BmString.
	\param into The BmString where to copy the object.
	\param fromOffset The offset (zero based) where to begin the copy
	\param length The amount of bytes to copy.
	\return This function always returns *this .
*/
BmString &
BmString::CopyInto(BmString &into, int32 fromOffset, int32 length) const
{
	if (&into != this) {
		CHECK_PARAM_RET(fromOffset >= 0, "'fromOffset' must not be negative!", 
						into);
		CHECK_PARAM_RET(fromOffset <= Length(), "'fromOffset' exceeds length!", 
						into);
		into.SetTo(String() + fromOffset, length);
	}
	return into;
}


// CopyInto
/*! \brief Copy the BmString data (or part of it) into the supplied buffer.
	\param into The buffer where to copy the object.
	\param fromOffset The offset (zero based) where to begin the copy
	\param length The amount of bytes to copy.
*/
void
BmString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	if (into != NULL) {
		CHECK_PARAM_VOID(fromOffset >= 0, "'fromOffset' must not be negative!");
		CHECK_PARAM_VOID(fromOffset <= Length(), "'fromOffset' exceeds length!");
		int32 len = min_clamp0(length, Length() - fromOffset);
		memcpy(into, _privateData + fromOffset, len);
	}
}


/*---- Appending -----------------------------------------------------------*/
// plus operator
/*!	\brief Appends the given string to the object.
	\param str A pointer to the string to append.
	\return This function always returns *this .
*/
BmString&
BmString::operator+=(const char *str)
{
	if (str != NULL)
		_DoAppend(str, strlen(str));
	return *this;
}


// plus operator
/*!	\brief Appends the given character to the object.
	\param c The character to append.
	\return This function always returns *this .
*/
BmString&
BmString::operator+=(char c)
{
	_DoAppend(&c, 1);
	return *this;
}


// Append
/*!	\brief Appends the given BmString to the object.
	\param string The BmString to append.
	\param length The maximum bytes to get from the original object.
	\return This function always returns *this .
*/
BmString&
BmString::Append(const BmString &string, int32 length)
{
	_DoAppend(string.String(), min_clamp0(length, string.Length()));
	return *this;
}


// Append
/*!	\brief Appends the given string to the object.
	\param str A pointer to the string to append.
	\param length The maximum bytes to get from the original string.
	\return This function always returns *this .
*/
BmString&
BmString::Append(const char *str, int32 length)
{
	if (str != NULL) {
		int32 len = strlen_clamp(str, length);
		_DoAppend(str, len);
	}	
	return *this;
}


// Append
/*!	\brief Appends the given character to the object.
	\param c The character to append.
	\param count The number of characters to append.
	\return This function always returns *this .
*/
BmString&
BmString::Append(char c, int32 count)
{
	int32 len = Length();
	if (count > 0 && _GrowBy(count))
		memset(_privateData + len, c, count);

	return *this;
}


/*---- Prepending ----------------------------------------------------------*/
// Prepend
/*!	\brief Prepends the given string to the object.
	\param str A pointer to the string to prepend.
	\return This function always returns *this .
*/
BmString&
BmString::Prepend(const char *str)
{
	if (str != NULL)
		_DoPrepend(str, strlen(str));
	return *this;
}


// Prepend
/*!	\brief Prepends the given BmString to the object.
	\param string The BmString object to prepend.
	\return This function always returns *this .
*/
BmString&
BmString::Prepend(const BmString &string)
{
	if (&string != this)
		_DoPrepend(string.String(), string.Length());
	return *this;
}


// Prepend
/*!	\brief Prepends the given string to the object.
	\param str A pointer to the string to prepend.
	\param length The maximum amount of bytes to get from the string.
	\return This function always returns *this .
*/
BmString&
BmString::Prepend(const char *str, int32 length)
{
	if (str != NULL) {
		int32 len = strlen_clamp(str, length);
		_DoPrepend(str, len);
	}
	return *this;
}


// Prepend
/*!	\brief Prepends the given BmString to the object.
	\param string The BmString object to prepend.
	\param len The maximum amount of bytes to get from the BmString.
	\return This function always returns *this .
*/
BmString&
BmString::Prepend(const BmString &string, int32 len)
{
	if (&string != this)
		_DoPrepend(string.String(), min_clamp0(len, string.Length()));
	return *this;
}


// Prepend
/*!	\brief Prepends the given character to the object.
	\param c The character to prepend.
	\param count The amount of characters to prepend.
	\return This function always returns *this .
*/
BmString&
BmString::Prepend(char c, int32 count)
{
	if (count > 0 && _OpenAtBy(0, count))
		memset(_privateData, c, count);
	
	return *this;
}


/*---- Inserting ----------------------------------------------------------*/
// Insert
/*! \brief Inserts the given string at the given position into the object's data.
	\param str A pointer to the string to insert.
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const char *str, int32 pos)
{
	if (str != NULL) {
		CHECK_PARAM(pos <= Length(), "'pos' exceeds length!");
		int32 len = (int32)strlen(str);
		if (pos < 0) {
			int32 skipLen = min_clamp0(-1 * pos, len);
			str += skipLen;
			len -= skipLen;
			pos = 0;
		} else
			pos = min_clamp0(pos, Length());
		if (_OpenAtBy(pos, len))
			memcpy(_privateData + pos, str, len);
	}
	return *this;
}


// Insert
/*! \brief Inserts the given string at the given position into the object's data.
	\param str A pointer to the string to insert.
	\param length The amount of bytes to insert.	
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const char *str, int32 length, int32 pos)
{
	if (str != NULL) {
		CHECK_PARAM(pos <= Length(), "'pos' exceeds length!");
		int32 len = strlen_clamp(str, length);
		if (pos < 0) {
			int32 skipLen = min_clamp0(-1 * pos, len);
			str += skipLen;
			len -= skipLen;
			pos = 0;
		} else
			pos = min_clamp0(pos, Length());
		if (_OpenAtBy(pos, len))
			memcpy(_privateData + pos, str, len);
	}
	return *this;
}


// Insert
/*! \brief Inserts the given string at the given position into the object's data.
	\param str A pointer to the string to insert.
	\param fromOffset
	\param length The amount of bytes to insert.	
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const char *str, int32 fromOffset, int32 length, int32 pos)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return Insert(str + fromOffset, length, pos);
}


// Insert
/*! \brief Inserts the given BmString at the given position into the object's data.
	\param string The BmString object to insert.
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const BmString &string, int32 pos)
{
	if (&string != this)
		Insert(string.String(), pos); //TODO: Optimize
	return *this;				  
}


// Insert
/*! \brief Inserts the given BmString at the given position into the object's data.
	\param string The BmString object to insert.
	\param length The amount of bytes to insert.
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const BmString &string, int32 length, int32 pos)
{
	if (&string != this)
		Insert(string.String(), length, pos); //TODO: Optimize
	return *this;
}


// Insert
/*! \brief Inserts the given string at the given position into the object's data.
	\param string The BmString object to insert.
	\param fromOffset
	\param length The amount of bytes to insert.
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(const BmString &string, int32 fromOffset, int32 length, int32 pos)
{
	if (&string != this) {
		CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
		Insert(string.String() + fromOffset, length, pos);
	}
	return *this;
}


// Insert
/*! \brief Inserts the given character at the given position into the object's data.
	\param c The character to insert.
	\param count The amount of bytes to insert.
	\param pos The offset into the BmString's data where to insert the string.
	\return This function always returns *this .
*/
BmString&
BmString::Insert(char c, int32 count, int32 pos)
{
	CHECK_PARAM(pos <= Length(), "'pos' exceeds length!");
	if (pos < 0) {
		count = max_c(count + pos, 0);
		pos = 0;
	} else
		pos = min_clamp0(pos, Length());
	
	if (count > 0 && _OpenAtBy(pos, count))
		memset(_privateData + pos, c, count);
	
	return *this;
}


/*---- Removing -----------------------------------------------------------*/
// Truncate
/*! \brief Truncate the string to the new length.
	\param newLength The new lenght of the string.
	\param lazy If true, the memory-optimisation is postponed to later
	\return This function always returns *this .
*/
BmString&
BmString::Truncate(int32 newLength, bool lazy)
{
	if (newLength < 0)
		newLength = 0;
	
	int32 curLen = Length();
		
	if (newLength < curLen) {
		if (lazy) {
			// don't free memory yet, just set new length:
			// XXX: Uhm, where do we keep track of the amount
			// of memory we allocated ?
			_SetLength(newLength);
			_privateData[newLength] = '\0';
		} else
			_GrowBy(newLength - curLen); //Negative	
	}
	return *this;
}


// Remove
/*! \brief Removes some bytes, starting at the given offset
	\param from The offset from which you want to start removing
	\param length The number of bytes to remove
	\return This function always returns *this .
*/
BmString&
BmString::Remove(int32 from, int32 length)
{
	int32 len = Length();
	if (from < 0) {
		int32 skipLen = min_clamp0(from, len);
		len -= skipLen;
		from = 0;
	} else
		from = min_clamp0(from, len);
	_ShrinkAtBy(from, min_clamp0(length, len - from));
	return *this;
}


// Remove
/*! \brief Removes the first occurrence of the given BmString.
	\param string The BmString to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveFirst(const BmString &string)
{
	int32 pos = _ShortFindAfter(string.String(), string.Length());
	
	if (pos >= 0)
		_ShrinkAtBy(pos, string.Length());
	
	return *this;
}


// Remove
/*! \brief Removes the last occurrence of the given BmString.
	\param string The BmString to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveLast(const BmString &string)
{
	int32 pos = _FindBefore(string.String(), Length(), string.Length());
	
	if (pos >= 0)
		_ShrinkAtBy(pos, string.Length());
		
	return *this;
}


// Remove
/*! \brief Removes all occurrences of the given BmString.
	\param string The BmString to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveAll(const BmString &string)
{
	return _DoReplace(string.String(), "", REPLACE_ALL, 0, KEEP_CASE);
}


// Remove
/*! \brief Removes the first occurrence of the given string.
	\param str A pointer to the string to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveFirst(const char *str)
{
	if (str != NULL) {
		int32 pos = _ShortFindAfter(str, strlen(str));
		if (pos >= 0)
			_ShrinkAtBy(pos, strlen(str));
	}
	return *this;
}


// Remove
/*! \brief Removes the last occurrence of the given string.
	\param str A pointer to the string to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveLast(const char *str)
{
	if (str != NULL) {
		int32 len = strlen(str);
		int32 pos = _FindBefore(str, Length(), len);
		if (pos >= 0)
			_ShrinkAtBy(pos, len);
	}
	return *this;
}


// Remove
/*! \brief Removes all occurrences of the given string.
	\param str A pointer to the string to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveAll(const char *str)
{
	return _DoReplace(str, "", REPLACE_ALL, 0, KEEP_CASE);
}


// Remove
/*! \brief Removes all the characters specified.
	\param setOfCharsToRemove The set of characters to remove.
	\return This function always returns *this .
*/
BmString&
BmString::RemoveSet(const char *setOfCharsToRemove)
{
	return ReplaceSet(setOfCharsToRemove, "");
}


// MoveInto
/*! \brief Move the BmString data (or part of it) into another BmString.
	\param into The BmString where to move the object.
	\param from The offset (zero based) where to begin the move
	\param length The amount of bytes to move.
	\return This function always returns into.
*/
BmString&
BmString::MoveInto(BmString &into, int32 from, int32 length)
{
	CHECK_PARAM_RET(from >= 0, "'from' must not be negative!", into);
	CHECK_PARAM_RET(from <= Length(), "'from' exceeds length!", into);
	int32 len = min_clamp0(length, Length() - from);
	if (&into == this) {
		/* TODO: [zooey]: to be activated later (>R1):
		// strings are identical, just move the data:
		if (from>0 && _privateData)
			memmove( _privateData, _privateData+from, len);
		Truncate( len);
		*/
		return *this;
	}
	into.SetTo(String() + from, len);
	_ShrinkAtBy(from, len);

	return into;
}


// MoveInto
/*! \brief Move the BmString data (or part of it) into the given buffer.
	\param into The buffer where to move the object.
	\param from The offset (zero based) where to begin the move
	\param length The amount of bytes to move.
*/
void
BmString::MoveInto(char *into, int32 from, int32 length)
{
	if (into != NULL) {
		CHECK_PARAM_VOID(from >= 0, "'from' must not be negative!");
		CHECK_PARAM_VOID(from <= Length(), "'from' exceeds length!");
		int32 len = min_clamp0(length, Length() - from);
		memcpy(into, String() + from, len);
		into[len] = '\0';
		_ShrinkAtBy(from, len);
	}
}


/*---- Compare functions ---------------------------------------------------*/
bool
BmString::operator<(const char *string) const
{
	return strcmp(String(), safestr(string)) < 0;
}


bool
BmString::operator<=(const char *string) const
{
	return strcmp(String(), safestr(string)) <= 0;
}


bool
BmString::operator==(const char *string) const
{
	return strcmp(String(), safestr(string)) == 0;
}


bool
BmString::operator>=(const char *string) const
{
	return strcmp(String(), safestr(string)) >= 0;
}


bool
BmString::operator>(const char *string) const
{
	return strcmp(String(), safestr(string)) > 0;
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
	return strcmp(String(), safestr(string));
}


int
BmString::Compare(const BmString &string, int32 n) const
{
	return strncmp(String(), string.String(), n);
}


int
BmString::Compare(const char *string, int32 n) const
{
	return strncmp(String(), safestr(string), n);
}


int
BmString::ICompare(const BmString &string) const
{
	return strcasecmp(String(), string.String());
}


int
BmString::ICompare(const char *str) const
{
	return strcasecmp(String(), safestr(str));
}


int
BmString::ICompare(const BmString &string, int32 n) const
{
	return strncasecmp(String(), string.String(), n);
}


int
BmString::ICompare(const char *str, int32 n) const
{
	return strncasecmp(String(), safestr(str), n);
}


/*---- Searching -----------------------------------------------------------*/
// FindFirst
/*! \brief Find the first occurrence of the given BmString.
	\param string The BmString to search for.
	\return The offset(zero based) into the data
		where the given BmString has been found.
*/
int32
BmString::FindFirst(const BmString &string) const
{
	return _ShortFindAfter(string.String(), string.Length());
}


// FindFirst
/*! \brief Find the first occurrence of the given string.
	\param string The string to search for.
	\return The offset(zero based) into the data
		where the given string has been found.
*/
int32
BmString::FindFirst(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _ShortFindAfter(string, strlen(string));
}


// FindFirst
/*! \brief Find the first occurrence of the given BmString,
		starting from the given offset.
	\param string The BmString to search for.
	\param fromOffset The offset where to start the search.
	\return An integer which is the offset(zero based) into the data
		where the given BmString has been found.
*/
int32
BmString::FindFirst(const BmString &string, int32 fromOffset) const
{
	if (fromOffset < 0)
		return B_ERROR;
	return _FindAfter(string.String(), min_clamp0(fromOffset, Length()),
						    string.Length());
}


// FindFirst
/*! \brief Find the first occurrence of the given string,
		starting from the given offset.
	\param string The string to search for.
	\param fromOffset The offset where to start the search.
	\return The offset(zero based) into the data
		where the given string has been found.
*/
int32
BmString::FindFirst(const char *string, int32 fromOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	if (fromOffset < 0)
		return B_ERROR;
	return _FindAfter(string, min_clamp0(fromOffset, Length()), strlen(string));
}


// FindFirst
/*! \brief Find the first occurrence of the given character.
	\param c The character to search for.
	\return The offset(zero based) into the data
		where the given character has been found.
*/
int32
BmString::FindFirst(char c) const
{	
	const char *start = String();
	const char *end = String() + Length(); /* String's end */
	
	/* Scans the string until we find the character, */
	/* or we hit the string's end */
	while(start != end && *start != c)
		start++;
	
	if (start == end)
		return B_ERROR;
			
	return start - String();
}


// FindFirst
/*! \brief Find the first occurrence of the given character,
		starting from the given offset.
	\param c The character to search for.
	\param fromOffset The offset where to start the search.
	\return The offset(zero based) into the data
		where the given character has been found.
*/
int32
BmString::FindFirst(char c, int32 fromOffset) const
{
	if (fromOffset < 0)
		return B_ERROR;
		
	const char *start = String() + min_clamp0(fromOffset, Length());
	const char *end = String() + Length(); /* String's end */
	
	/* Scans the string until we found the character, */
	/* or we hit the string's end */
	while(start < end && *start != c)
		start++;
	
	if (start >= end)
		return B_ERROR;
			
	return start - String();
}


// FindLast
/*! \brief Find the last occurrence of the given BmString.
	\param string The BmString to search for.
	\return The offset(zero based) into the data
		where the given BmString has been found.
*/
int32
BmString::FindLast(const BmString &string) const
{
	return _FindBefore(string.String(), Length(), string.Length());
}


// FindLast
/*! \brief Find the last occurrence of the given string.
	\param string The string to search for.
	\return The offset(zero based) into the data
		where the given string has been found.
*/
int32
BmString::FindLast(const char *string) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	return _FindBefore(string, Length(), strlen(string));
}


// FindLast
/*! \brief Find the last occurrence of the given BmString,
		starting from the given offset, and going backwards.
	\param string The BmString to search for.
	\param beforeOffset The offset where to start the search.
	\return An integer which is the offset(zero based) into the data
		where the given BmString has been found.
*/
int32
BmString::FindLast(const BmString &string, int32 beforeOffset) const
{
	if (beforeOffset < 0)
		return B_ERROR;
	return _FindBefore(string.String(), min_clamp0(beforeOffset, Length()), 
							 string.Length()); 
}


// FindLast
/*! \brief Find the last occurrence of the given string,
		starting from the given offset, and going backwards.
	\param string The string to search for.
	\return The offset(zero based) into the data
		where the given string has been found.
*/
int32
BmString::FindLast(const char *string, int32 beforeOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	if (beforeOffset < 0)
		return B_ERROR;
	return _FindBefore(string, min_clamp0(beforeOffset, Length()), strlen(string));
}


// FindLast
/*! \brief Find the last occurrence of the given character.
	\param c The character to search for.
	\return The offset(zero based) into the data
		where the given character has been found.
*/
int32
BmString::FindLast(char c) const
{
	const char *start = String();
	const char *end = String() + Length(); /* String's end */
	
	/* Scans the string backwards until we found the character, */
	/* or we reach the string's start */
	while(end != start && *end != c)
		end--;
	
	if (end == start)
		return B_ERROR;
			
	return end - String();
}


// FindLast
/*! \brief Find the last occurrence of the given character,
		starting from the given offset and going backwards.
	\param c The character to search for.
	\param beforeOffset The offset where to start the search.
	\return The offset(zero based) into the data
		where the given character has been found.
*/
int32
BmString::FindLast(char c, int32 beforeOffset) const
{
	if (beforeOffset < 0)
		return B_ERROR;
		
	const char *start = String();
	const char *end = String() + Length() - beforeOffset;
	
	/* Scans the string backwards until we found the character, */
	/* or we reach the string's start */
	while(end > start && *end != c)
		end--;
	
	if (end <= start)
		return B_ERROR;
			
	return end - String();
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
	if (fromOffset < 0)
		return B_ERROR;
	return _IFindAfter(string.String(), min_clamp0(fromOffset, Length()), 
						    string.Length());
}


int32
BmString::IFindFirst(const char *string, int32 fromOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	if (fromOffset < 0)
		return B_ERROR;
	return _IFindAfter(string, min_clamp0(fromOffset,Length()), strlen(string));
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
	if (beforeOffset < 0)
		return B_ERROR;
	return _IFindBefore(string.String(), min_clamp0(beforeOffset, Length()), 
							  string.Length());
}


int32
BmString::IFindLast(const char *string, int32 beforeOffset) const
{
	if (string == NULL)
		return B_BAD_VALUE;
	if (beforeOffset < 0)
		return B_ERROR;
	return _IFindBefore(string, min_clamp0(beforeOffset,Length()), 
							  strlen(string));
}


/*---- Replacing -----------------------------------------------------------*/
BmString&
BmString::ReplaceFirst(char replaceThis, char withThis)
{
	int32 pos = FindFirst(replaceThis);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BmString&
BmString::ReplaceLast(char replaceThis, char withThis)
{
	int32 pos = FindLast(replaceThis);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BmString&
BmString::ReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return Replace(replaceThis, withThis, REPLACE_ALL, fromOffset);
}


BmString&
BmString::Replace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	if (maxReplaceCount > 0) {
		for (int32 pos = min_clamp0(fromOffset, Length()); 
			  		maxReplaceCount > 0; --maxReplaceCount, ++pos) {
			pos = FindFirst(replaceThis, pos);
			if (pos < 0)
				break;
			_privateData[pos] = withThis;
		}
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
		
		if (difference > 0) {
			if (!_OpenAtBy(pos, difference))
				return *this;
		} else if (difference < 0) {
			if (!_ShrinkAtBy(pos, -difference))
				return *this;
		}
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BmString&
BmString::ReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return _DoReplace(replaceThis, withThis, REPLACE_ALL, 
							 min_clamp0(fromOffset,Length()), KEEP_CASE);
}


BmString&
BmString::Replace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return _DoReplace(replaceThis, withThis, maxReplaceCount, 
							 min_clamp0(fromOffset,Length()), KEEP_CASE);
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
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return IReplace(replaceThis, withThis, REPLACE_ALL, fromOffset);
}


BmString&
BmString::IReplace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");

	char tmp[2] = { replaceThis, '\0' };
	
	if (_privateData == NULL)
		return *this;
		
	for (int32 pos = min_clamp0(fromOffset,Length()); 
		  	maxReplaceCount > 0;   --maxReplaceCount, ++pos) {
		pos = _IFindAfter(tmp, pos, 1);
		if (pos < 0)
			break;
		_privateData[pos] = withThis;
	}
	return *this;
}


BmString&
BmString::IReplaceFirst(const char *replaceThis, const char *withThis)
{
	return _DoReplace(replaceThis, withThis, 1, 0, IGNORE_CASE);
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
		
		if (difference > 0) {
			if (!_OpenAtBy(pos, difference))
				return *this;
		} else if (difference < 0) {
			if (!_ShrinkAtBy(pos, -difference))
				return *this;
		}
		memcpy(_privateData + pos, withThis, len);
		
	}
		
	return *this;
}


BmString&
BmString::IReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return _DoReplace(replaceThis, withThis, REPLACE_ALL, 
							 min_clamp0(fromOffset, Length()), IGNORE_CASE);
}


BmString&
BmString::IReplace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	CHECK_PARAM(fromOffset >= 0, "'fromOffset' must not be negative!");
	return _DoReplace(replaceThis, withThis, maxReplaceCount, 
							 min_clamp0(fromOffset, Length()), IGNORE_CASE);
}


BmString&
BmString::ReplaceSet(const char *setOfChars, char with)
{
	if (setOfChars == NULL)
		return *this;

	int32 offset = 0;
	int32 length = Length();
	
	for (int32 pos;;) {
		pos = strcspn(String() + offset, setOfChars);

		offset += pos;
		if (offset >= length)
			break;

		_privateData[offset] = with;
		offset++;
	}

	return *this;
}

BmString&
BmString::ReplaceSet(const char *setOfChars, const char *with)
{
	int32 withLen = with ? strlen(with) : 0;
	if (withLen == 1)
		// delegate simple case:
		return ReplaceSet( setOfChars, *with);

	if (setOfChars == NULL || _privateData == NULL)
		return *this;
	
	PosVect positions;

	int32 searchLen = 1;
	int32 len = Length();
	int32 pos = 0;
	for (int32 offset = 0; offset < len; offset += (pos+searchLen)) 
	{
		pos = strcspn(_privateData + offset, setOfChars);
		if (pos + offset >= len)
			break;
		if (!positions.Add(offset + pos))
			return *this;
	}

	_ReplaceAtPositions(&positions, searchLen, with, withLen);	
	return *this;
}


/*---- Unchecked char access -----------------------------------------------*/

// operator[]
/*! \brief Returns a reference to the data at the given offset.
	
	This function can be used to read a byte or to change its value.
	\param index The index (zero based) of the byte to get.
	\return Returns a reference to the specified byte.
*/
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
	
	if (maxLength > len) {
		if (!_GrowBy(maxLength - len))
			return NULL;
		if (!len && _privateData)
			// if string was empty before call to LockBuffer(), we make sure the
			// buffer represents an empty c-string:
			*_privateData = '\0';
	} else if (!maxLength && !len) {
		// special case for unallocated string, we return an empty c-string:
		return const_cast<char*>(String());
	}

	return _privateData;
}


BmString&
BmString::UnlockBuffer(int32 length)
{
	_SetUsingAsCString(false); //debug
	
	int32 len = length;

	if (len < 0)
		len = (_privateData == NULL) ? 0 : strlen(_privateData);

	if (len != Length())
		_GrowBy(len - Length());
		
	return *this;
}


/*---- Uppercase<->Lowercase ------------------------------------------------*/
// ToLower
/*! \brief Converts the BmString to lowercase
	\return This function always returns *this .
*/
BmString&
BmString::ToLower()
{
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = tolower(_privateData[count]);
	
	return *this;
}


// ToUpper
/*! \brief Converts the BmString to uppercase
	\return This function always returns *this .
*/
BmString&
BmString::ToUpper()
{			
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = toupper(_privateData[count]);
	
	return *this;
}


// Capitalize
/*! \brief Converts the first character to uppercase, rest to lowercase
	\return This function always returns *this .
*/
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


// CapitalizeEachWord
/*! \brief Converts the first character of every word to uppercase, rest to lowercase.
	
	Converts the first character of every "word" (series of alpabetical characters
	separated by non alphabetical characters) to uppercase, and the rest to lowercase.
	\return This function always returns *this .
*/
BmString&
BmString::CapitalizeEachWord()
{
	if (_privateData == NULL)
		return *this;
		
	int32 count = 0;
	int32 length = Length();
		
	do {
		// Find the first alphabetical character...
		for(; count < length; count++) {
			if (isalpha(_privateData[count])) {
				// ...found! Convert it to uppercase.
				_privateData[count] = toupper(_privateData[count]);
				count++;
				break;
			}
		}
		// Now find the first non-alphabetical character,
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
	
	PosVect positions;
	int32 len = Length();
	int32 pos = 0;
	for (int32 offset = 0; offset < len; offset += pos + 1) {
		if ((pos = strcspn(_privateData + offset, setOfCharsToEscape)) < len - offset)
			if (!positions.Add(offset + pos))
				return *this;
	}

	uint32 count = positions.CountItems();
	int32 newLength = len + count;
	if (!newLength) {
		_GrowBy( -len);
		return *this;
	}
	int32 lastPos = 0;
	char* oldAdr = _privateData;
	char* newData = (char*)malloc(newLength + sizeof(int32) + 1);
	if (newData) {
		newData += sizeof(int32);
		char* newAdr = newData;
		for (uint32 i = 0; i < count; ++i) {
			pos = positions.ItemAt( i);
			len = pos-lastPos;
			if (len > 0) {
				memcpy(newAdr, oldAdr, len);
				oldAdr += len;
				newAdr += len;
			}
			*newAdr++ = escapeWith;
			*newAdr++ = *oldAdr++;
			lastPos = pos + 1;
		}
		len = Length() + 1 - lastPos;
		if (len > 0)
			memcpy(newAdr, oldAdr, len);

		free(_privateData - sizeof(int32));
		_privateData = newData;
		_privateData[newLength] = 0;
		_SetLength( newLength);
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
	const char temp[2] = {escapeChar, 0};
	return _DoReplace(temp, "", REPLACE_ALL, 0, KEEP_CASE);
}


/*---- Simple sprintf replacement calls ------------------------------------*/
/*---- Slower than sprintf but type and overflow safe ----------------------*/
BmString&
BmString::operator<<(const char *str)
{
	if (str != NULL)
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
	sprintf(num, "%Lu", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(int64 i)
{
	char num[64];
	sprintf(num, "%Ld", i);
	
	return *this << num;
}


BmString&
BmString::operator<<(float f)
{
	char num[64];
	sprintf(num, "%.2f", f);
	
	return *this << num;
}


BmString&
BmString::operator<<(double d)
{
	char num[64];
	sprintf(num, "%.2g", d);
	
	return *this << num;
}


/*---- Private or Reserved ------------------------------------------------*/
char*
BmString::_Alloc(int32 dataLen)
{
	char *dataPtr = _privateData ? _privateData - sizeof(int32) : NULL;
	if (dataLen <= 0) {	// release buffer if requested size is 0:
		free(dataPtr);
		_privateData = NULL;
		return NULL;
	}
	int32 allocLen = dataLen + sizeof(int32) + 1;
	dataPtr = (char *)realloc(dataPtr, allocLen);
	if (dataPtr) {
		dataPtr += sizeof(int32);
		_privateData = dataPtr;
		int32 newLen = allocLen - sizeof(int32) - 1;
		_SetLength(newLen);
		_privateData[newLen] = '\0';
	}
	return dataPtr;
}	

void
BmString::_Init(const char *str, int32 len)
{
	if (_Alloc(len))
		memcpy(_privateData, str, len);
}


#if ENABLE_INLINES
inline
#endif
void
BmString::_DoAssign(const char *str, int32 len)
{
	int32 curLen = Length();
	
	if (len == curLen || _GrowBy(len - curLen))
		memcpy(_privateData, str, len);
}


#if ENABLE_INLINES
inline
#endif
void
BmString::_DoAppend(const char *str, int32 len)
{
	int32 length = Length();
	if (_GrowBy(len))
		memcpy(_privateData + length, str, len);
}


char*
BmString::_GrowBy(int32 size)
{		
	int32 newLen = Length() + size; 	
	return _Alloc(newLen);
}


char *
BmString::_OpenAtBy(int32 offset, int32 length)
{
	int32 oldLength = Length();
	
	char* newData = _Alloc(oldLength + length);
	if (newData != NULL)
		memmove(_privateData + offset + length, _privateData + offset,
				  oldLength - offset);
	
	return newData;
}


char*
BmString::_ShrinkAtBy(int32 offset, int32 length)
{	
	if (!_privateData)
		return NULL;
	int32 oldLength = Length();

	memmove(_privateData + offset, _privateData + offset + length,
		     oldLength - offset - length);

	// the following actually should never fail, since we are reducing the size...
	return _Alloc(oldLength - length);
}


#if ENABLE_INLINES
inline
#endif
void
BmString::_DoPrepend(const char *str, int32 count)
{
	if (_OpenAtBy(0, count))
		memcpy(_privateData, str, count);
}


/* XXX: These could be inlined too, if they are too slow */
int32
BmString::_FindAfter(const char *str, int32 offset, int32 /*strlen*/) const
{	
	char *ptr = strstr(String() + offset, str);

	if (ptr != NULL)
		return ptr - String();
	
	return B_ERROR;
}


int32
BmString::_IFindAfter(const char *str, int32 offset, int32 /*strlen*/) const
{
	char *ptr = strcasestr(String() + offset, str);

	if (ptr != NULL)
		return ptr - String();

	return B_ERROR;
}


int32
BmString::_ShortFindAfter(const char *str, int32 /*len*/) const
{
	char *ptr = strstr(String(), str);
	
	if (ptr != NULL)
		return ptr - String();
		
	return B_ERROR;
}


int32
BmString::_FindBefore(const char *str, int32 offset, int32 strlen) const
{
	if (_privateData) {
		const char *ptr = _privateData + offset - strlen;
		
		while (ptr >= _privateData) {	
			if (!memcmp(ptr, str, strlen))
				return ptr - _privateData; 
			ptr--;
		}
	}
	return B_ERROR;
}


int32
BmString::_IFindBefore(const char *str, int32 offset, int32 strlen) const
{
	if (_privateData) {
		char *ptr1 = _privateData + offset - strlen;
		
		while (ptr1 >= _privateData) {
			if (!strncasecmp(ptr1, str, strlen))
				return ptr1 - _privateData; 
			ptr1--;
		}
	}
	return B_ERROR;
}


BmString&
BmString::_DoReplace(const char *findThis, const char *replaceWith, int32 maxReplaceCount, 
						  int32 fromOffset,	bool ignoreCase)
{
	if (findThis == NULL || maxReplaceCount <= 0 
		|| fromOffset < 0 || fromOffset >= Length())
		return *this;
	
	typedef int32 (BmString::*TFindMethod)(const char *, int32, int32) const;
	TFindMethod findMethod = ignoreCase ? &BmString::_IFindAfter : &BmString::_FindAfter;
	int32 findLen = strlen(findThis);
	
	if (!replaceWith)
		replaceWith = "";
		
	int32 replaceLen = strlen(replaceWith);
	int32 lastSrcPos = fromOffset;
	PosVect positions;
	for(int32 srcPos = 0; 
			maxReplaceCount > 0 
			&& (srcPos = (this->*findMethod)(findThis, lastSrcPos, findLen)) >= 0; 
			maxReplaceCount-- ) {
		positions.Add(srcPos);
		lastSrcPos = srcPos + findLen;
	}
	_ReplaceAtPositions(&positions, findLen, replaceWith, replaceLen);
	return *this;
}


void
BmString::_ReplaceAtPositions(const PosVect* positions,
											  int32 searchLen, const char* with,
											  int32 withLen)
{
	int32 len = Length();
	uint32 count = positions->CountItems();
	int32 newLength = len + count * (withLen - searchLen);
	if (!newLength) {
		_GrowBy(-len);
		return;
	}
	int32 pos;
	int32 lastPos = 0;
	char *oldAdr = _privateData;
	char *newData = (char *)malloc(newLength + sizeof(int32) + 1);
	if (newData) {
		newData += sizeof(int32);
		char *newAdr = newData;
		for(uint32 i = 0; i < count; ++i) {
			pos = positions->ItemAt(i);
			len = pos - lastPos;
			if (len > 0) {
				memcpy(newAdr, oldAdr, len);
				oldAdr += len;
				newAdr += len;
			}
			memcpy(newAdr, with, withLen);
			oldAdr += searchLen;
			newAdr += withLen;
			lastPos = pos+searchLen;
		}
		len = Length() + 1 - lastPos;
		if (len > 0)
			memcpy(newAdr, oldAdr, len);

		free(_privateData - sizeof(int32));
		_privateData = newData;
		_privateData[newLength] = 0;
		_SetLength( newLength);
	}
}


#if ENABLE_INLINES
inline
#endif
void
BmString::_SetLength(int32 length)
{
	*((int32*)_privateData - 1) = length & 0x7fffffff;
}


#if DEBUG
// AFAIK, these are not implemented in BeOS R5
// XXX : Test these puppies
void
BmString::_SetUsingAsCString(bool state)
{	
	//TODO: Implement ?		
}


void
BmString::_AssertNotUsingAsCString() const
{
	//TODO: Implement ?
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



// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
// end of OpenBeOS implemenation of BmString
// -----------------------------------------------------------------------



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
	
	BmStringOBuf tempIO( (int32)max_c( 128, src->Length()*1.2), 1.2);
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
	
	BmStringOBuf tempIO( (int32)max_c( 128, src->Length()*1.2), 1.2);
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
	BmStringOBuf tempIO( (int32)max_c( 128, src->Length()*1.2), 1.2);
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
	BmStringOBuf tempIO( (int32)max_c( 128, Length()), 1.2);
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
		} else if (c1 == '%') {
			len = srcPos-lastSrcPos;
			tempIO.Write( String()+lastSrcPos, len);
			tempIO.Write( &c1, 1);
			lastSrcPos = srcPos+2;
			srcPos++;							// avoid to handle second % of '%%'-sequence
		}
	}
	if (tempIO.HasData()) {
		// only copy remainder if we have actually changed anything
		if ((len = Length()-lastSrcPos)!=0)
			tempIO.Write( String()+lastSrcPos, len);
		Adopt( tempIO.TheString());
	}
	return *this;
}

/*------------------------------------------------------------------------------*\
	Trim()
		-	trims left/right/both sides of string from whitespace
\*------------------------------------------------------------------------------*/
BmString&
BmString::Trim( bool left, bool right) {
	int32 len = Length();
	if (len) {
		char* buf = LockBuffer( len);
		char* start = buf;
		char* end = start+len;
		if (left)
			while( isspace(*start))
				start++;
		if (right)
			while( end>start && isspace(*(end-1)))
				end--;
		if (*start==0 || end==buf) {
			// string contains whitespace only, we throw it away:
			*buf = 0;
			len = 0;
		} else {
			len = end-start;
			if (len>0)
				memmove( buf, start, len);
		}
		UnlockBuffer( len);
	}
	return *this;
}
