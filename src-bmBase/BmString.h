/*
	BmString.h
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


#ifndef __BmString__
#define __BmString__

#include <SupportDefs.h>

#include <cstring>

#include "BmBase.h"

class IMPEXPBMBASE BmString {
public:
						BmString();
						BmString(const char *);
						BmString(const BmString &);
						BmString(const char *, int32 maxLength);
					
						~BmString();
			
/*---- Access --------------------------------------------------------------*/
	const char 			*String() const;
						/* returns null-terminated string */

	int32 				Length() const;
						/* length of corresponding string */

	int32				CountChars() const;
						/* returns number of UTF8 characters in string */
/*---- Assignment ----------------------------------------------------------*/
	BmString 			&operator=(const BmString &);
	BmString 			&operator=(const char *);
	BmString 			&operator=(char);
	
	BmString				&SetTo(const char *);
	BmString 			&SetTo(const char *, int32 length);

	BmString				&SetTo(const BmString &from);
	BmString				&Adopt(BmString &from);
						/* leaves <from> empty, avoiding a copy */
						
	BmString 			&SetTo(const BmString &, int32 length);
	BmString 			&Adopt(BmString &from, int32 length);
						/* leaves <from> empty, avoiding a copy */
		
	BmString 			&SetTo(char, int32 count);

/*---- Substring copying ---------------------------------------------------*/
	BmString 			&CopyInto(BmString &into, int32 fromOffset,
							int32 length) const;
						/* returns <into> ref as it's result; doesn't do
						 * anything if <into> is <this>
						 */

	void				CopyInto(char *into, int32 fromOffset,
							int32 length) const;
						/* caller guarantees that <into> is large enough */

/*---- Appending -----------------------------------------------------------*/
	BmString 			&operator+=(const BmString &);
	BmString 			&operator+=(const char *);
	BmString 			&operator+=(char);
	
	BmString 			&Append(const BmString &);
	BmString 			&Append(const char *);

	BmString 			&Append(const BmString &, int32 length);
	BmString 			&Append(const char *, int32 length);
	BmString 			&Append(char, int32 count);

/*---- Prepending ----------------------------------------------------------*/
	BmString 			&Prepend(const char *);
	BmString 			&Prepend(const BmString &);
	BmString 			&Prepend(const char *, int32);
	BmString 			&Prepend(const BmString &, int32);
	BmString 			&Prepend(char, int32 count);

/*---- Inserting ----------------------------------------------------------*/
	BmString 			&Insert(const char *, int32 pos);
	BmString 			&Insert(const char *, int32 length, int32 pos);
	BmString 			&Insert(const char *, int32 fromOffset,
							int32 length, int32 pos);

	BmString 			&Insert(const BmString &, int32 pos);
	BmString 			&Insert(const BmString &, int32 length, int32 pos);
	BmString 			&Insert(const BmString &, int32 fromOffset,
							int32 length, int32 pos);
	BmString 			&Insert(char, int32 count, int32 pos);

/*---- Removing -----------------------------------------------------------*/
	BmString 			&Truncate(int32 newLength, bool lazy = true);
						/* pass false in <lazy> to ensure freeing up the
						 * truncated memory
						 */
						
	BmString 			&Remove(int32 from, int32 length);

	BmString 			&RemoveFirst(const BmString &);
	BmString 			&RemoveLast(const BmString &);
	BmString 			&RemoveAll(const BmString &);

	BmString 			&RemoveFirst(const char *);
	BmString 			&RemoveLast(const char *);
	BmString 			&RemoveAll(const char *);
	
	BmString 			&RemoveSet(const char *setOfCharsToRemove);
	
	BmString 			&MoveInto(BmString &into, int32 from, int32 length);
	void				MoveInto(char *into, int32 from, int32 length);
						/* caller guarantees that <into> is large enough */


/*---- Compare functions ---------------------------------------------------*/
	bool 				operator<(const BmString &) const;
	bool 				operator<=(const BmString &) const;
	bool 				operator==(const BmString &) const;
	bool 				operator>=(const BmString &) const;
	bool 				operator>(const BmString &) const;
	bool 				operator!=(const BmString &) const;
	
	bool 				operator<(const char *) const;
	bool 				operator<=(const char *) const;
	bool 				operator==(const char *) const;
	bool 				operator>=(const char *) const;
	bool 				operator>(const char *) const;
	bool 				operator!=(const char *) const;
	
/*---- strcmp-style compare functions --------------------------------------*/
	int 				Compare(const BmString &) const;
	int 				Compare(const char *) const;
	int 				Compare(const BmString &, int32 n) const;
	int 				Compare(const char *, int32 n) const;
	int 				ICompare(const BmString &) const;
	int 				ICompare(const char *) const;
	int 				ICompare(const BmString &, int32 n) const;
	int 				ICompare(const char *, int32 n) const;
	
/*---- Searching -----------------------------------------------------------*/
	int32 				FindFirst(const BmString &) const;
	int32 				FindFirst(const char *) const;
	int32 				FindFirst(const BmString &, int32 fromOffset) const;
	int32 				FindFirst(const char *, int32 fromOffset) const;
	int32				FindFirst(char) const;
	int32				FindFirst(char, int32 fromOffset) const;

	int32 				FindLast(const BmString &) const;
	int32 				FindLast(const char *) const;
	int32 				FindLast(const BmString &, int32 beforeOffset) const;
	int32 				FindLast(const char *, int32 beforeOffset) const;
	int32				FindLast(char) const;
	int32				FindLast(char, int32 beforeOffset) const;

	int32 				IFindFirst(const BmString &) const;
	int32 				IFindFirst(const char *) const;
	int32 				IFindFirst(const BmString &, int32 fromOffset) const;
	int32 				IFindFirst(const char *, int32 fromOffset) const;

	int32 				IFindLast(const BmString &) const;
	int32 				IFindLast(const char *) const;
	int32 				IFindLast(const BmString &, int32 beforeOffset) const;
	int32 				IFindLast(const char *, int32 beforeOffset) const;

/*---- Replacing -----------------------------------------------------------*/

	BmString 			&ReplaceFirst(char replaceThis, char withThis);
	BmString 			&ReplaceLast(char replaceThis, char withThis);
	BmString 			&ReplaceAll(char replaceThis, char withThis,
							int32 fromOffset = 0);
	BmString 			&Replace(char replaceThis, char withThis,
							int32 maxReplaceCount, int32 fromOffset = 0);
	BmString 			&ReplaceFirst(const char *replaceThis,
							const char *withThis);
	BmString 			&ReplaceLast(const char *replaceThis,
							const char *withThis);
	BmString 			&ReplaceAll(const char *replaceThis,
							const char *withThis, int32 fromOffset = 0);
	BmString 			&Replace(const char *replaceThis, const char *withThis,
							int32 maxReplaceCount, int32 fromOffset = 0);

	BmString 			&IReplaceFirst(char replaceThis, char withThis);
	BmString 			&IReplaceLast(char replaceThis, char withThis);
	BmString 			&IReplaceAll(char replaceThis, char withThis,
							int32 fromOffset = 0);
	BmString 			&IReplace(char replaceThis, char withThis,
							int32 maxReplaceCount, int32 fromOffset = 0);
	BmString 			&IReplaceFirst(const char *replaceThis,
							const char *withThis);
	BmString 			&IReplaceLast(const char *replaceThis,
							const char *withThis);
	BmString 			&IReplaceAll(const char *replaceThis,
							const char *withThis, int32 fromOffset = 0);
	BmString 			&IReplace(const char *replaceThis, const char *withThis,
							int32 maxReplaceCount, int32 fromOffset = 0);
	
	BmString				&ReplaceSet(const char *setOfChars, char with);
	BmString				&ReplaceSet(const char *setOfChars, const char *with);
/*---- Unchecked char access -----------------------------------------------*/
	char 				operator[](int32 index) const;
	char 				&operator[](int32 index);

/*---- Checked char access -------------------------------------------------*/
	char 				ByteAt(int32 index) const;

/*---- Fast low-level manipulation -----------------------------------------*/
	char 				*LockBuffer(int32 maxLength);
	
		/* Make room for characters to be added by C-string like manipulation.
		 * Returns the equivalent of String(), <maxLength> includes space for
		 * trailing zero while used as C-string, it is illegal to call other
		 * BmString routines that rely on data/length consistency until
		 * UnlockBuffer sets things up again.
		 */

	BmString 			&UnlockBuffer(int32 length = -1);
	
		/* Finish using BmString as C-string, adjusting length. If no length
		 * passed in, strlen of internal data is used to determine it.
		 * BmString is in consistent state after this.
		 */
	
/*---- Upercase<->Lowercase ------------------------------------------------*/
	BmString				&ToLower();
	BmString 			&ToUpper();

	BmString 			&Capitalize();
						/* Converts first character to upper-case, rest to
						 * lower-case
						 */

	BmString 			&CapitalizeEachWord();
						/* Converts first character in each 
						 * non-alphabethycal-character-separated
						 * word to upper-case, rest to lower-case
						 */
/*----- Escaping and Deescaping --------------------------------------------*/
	BmString				&CharacterEscape(const char *original,
							const char *setOfCharsToEscape, char escapeWith);
						/* copies original into <this>, escaping characters
						 * specified in <setOfCharsToEscape> by prepending
						 * them with <escapeWith>
						 */
	BmString				&CharacterEscape(const char *setOfCharsToEscape,
							char escapeWith);
						/* escapes characters specified in <setOfCharsToEscape>
						 * by prepending them with <escapeWith>
						 */

	BmString				&CharacterDeescape(const char *original, char escapeChar);
						/* copy <original> into the string removing the escaping
						 * characters <escapeChar> 
						 */
	BmString				&CharacterDeescape(char escapeChar);
						/* remove the escaping characters <escapeChar> from 
						 * the string
						 */

/*---- Simple sprintf replacement calls ------------------------------------*/
/*---- Slower than sprintf but type and overflow safe ----------------------*/
	BmString 		&operator<<(const char *);
	BmString 		&operator<<(const BmString &);
	BmString 		&operator<<(char);
	BmString 		&operator<<(int);
	BmString 		&operator<<(unsigned int);
	BmString 		&operator<<(uint32);
	BmString 		&operator<<(int32);
	BmString 		&operator<<(uint64);
	BmString 		&operator<<(int64);
	BmString 		&operator<<(float);
		/* float output hardcodes %.2f style formatting */
	

/*----- Private or reserved ------------------------------------------------*/
private:
	void 			_Init(const char *, int32);
	void 			_DoAssign(const char *, int32);
	void 			_DoAppend(const char *, int32);
	char 			*_GrowBy(int32);
	char 			*_OpenAtBy(int32, int32);
	char 			*_ShrinkAtBy(int32, int32);
	void 			_DoPrepend(const char *, int32);
	
	int32 			_FindAfter(const char *, int32, int32) const;
	int32 			_IFindAfter(const char *, int32, int32) const;
	int32 			_ShortFindAfter(const char *, int32) const;
	int32 			_FindBefore(const char *, int32, int32) const;
	int32 			_IFindBefore(const char *, int32, int32) const;
	BmString			&_DoReplace(const char *, const char *, int32, int32,
								bool);
	void 			_SetLength(int32);

#if DEBUG
	void			_SetUsingAsCString(bool);
	void 			_AssertNotUsingAsCString() const;
#else
	void 			_SetUsingAsCString(bool) {}
	void			_AssertNotUsingAsCString() const {}
#endif

	char			*_Alloc( int32);

	struct PosVect;
	void 			_ReplaceAtPositions( const PosVect* positions,
											   int32 searchLen, 
											   const char* with,
											   int32 withLen);

protected:
	char *_privateData;


	// ----------------------------------------------------------
	// Beam extensions start here!	
	// ----------------------------------------------------------
public:
	BmString& ConvertLinebreaksToLF( const BmString* srcData=NULL);
	BmString& ConvertLinebreaksToCRLF( const BmString* srcData=NULL);
	BmString& ConvertTabsToSpaces( int32 numSpaces,
											 const BmString* srcData=NULL);
	BmString& DeUrlify();
	BmString& Trim( bool left=true, bool right=true);

};

/*----- Comutative compare operators --------------------------------------*/
bool 				operator<(const char *, const BmString &);
bool 				operator<=(const char *, const BmString &);
bool 				operator==(const char *, const BmString &);
bool 				operator>(const char *, const BmString &);
bool 				operator>=(const char *, const BmString &);
bool 				operator!=(const char *, const BmString &);

/*----- Non-member compare for sorting, etc. ------------------------------*/
int 				Compare(const BmString &, const BmString &);
int 				ICompare(const BmString &, const BmString &);
int 				Compare(const BmString *, const BmString *);
int 				ICompare(const BmString *, const BmString *);




/*-------------------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------------------*/

inline int32 
BmString::Length() const
{
	return _privateData ? (*((int32 *)_privateData - 1) & 0x7fffffff) : 0;
		/* the most significant bit is reserved; accessing
		 * it in any way will cause the computer to explode
		 */
}

inline const char *
BmString::String() const
{
	if (!_privateData)
		return "";
	return _privateData;
}

inline BmString &
BmString::SetTo(const char *str)
{
	return operator=(str);
}

inline char 
BmString::operator[](int32 index) const
{
	return _privateData[index];
}

inline char 
BmString::ByteAt(int32 index) const
{
	if (!_privateData || index < 0 || index > Length())
		return 0;
	return _privateData[index];
}

inline BmString &
BmString::operator+=(const BmString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}

inline BmString &
BmString::Append(const BmString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}

inline BmString &
BmString::Append(const char *str)
{
	return operator+=(str);
}

inline bool 
BmString::operator==(const BmString &string) const
{
	return strcmp(String(), string.String()) == 0;
}

inline bool 
BmString::operator<(const BmString &string) const
{
	return strcmp(String(), string.String()) < 0;
}

inline bool 
BmString::operator<=(const BmString &string) const
{
	return strcmp(String(), string.String()) <= 0;
}

inline bool 
BmString::operator>=(const BmString &string) const
{
	return strcmp(String(), string.String()) >= 0;
}

inline bool 
BmString::operator>(const BmString &string) const
{
	return strcmp(String(), string.String()) > 0;
}

inline bool 
BmString::operator!=(const BmString &string) const
{
	return strcmp(String(), string.String()) != 0;
}

inline bool 
BmString::operator!=(const char *str) const
{
	return !operator==(str);
}

inline bool 
operator<(const char *str, const BmString &string)
{
	return string > str;
}

inline bool 
operator<=(const char *str, const BmString &string)
{
	return string >= str;
}

inline bool 
operator==(const char *str, const BmString &string)
{
	return string == str;
}

inline bool 
operator>(const char *str, const BmString &string)
{
	return string < str;
}

inline bool 
operator>=(const char *str, const BmString &string)
{
	return string <= str;
}

inline bool 
operator!=(const char *str, const BmString &string)
{
	return string != str;
}

/*------------------------------------------------------------------------------*\
	utility operator to easy concatenation of BmStrings
\*------------------------------------------------------------------------------*/
IMPEXPBMBASE BmString operator+(const BmString& s1, const BmString& s2);
IMPEXPBMBASE BmString operator+(const char* s1, const BmString& s2);
IMPEXPBMBASE BmString operator+(const BmString& s1, const char* s2);


#endif
