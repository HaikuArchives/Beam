/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmImapNestedStringList_h
#define _BmImapNestedStringList_h

#include <vector>

using std::vector;

/*------------------------------------------------------------------------------*\
	BmImapNestedStringList
		-	implements the parenthesized list structure as defined by the IMAP
			protocol
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmImapNestedStringList
{
	typedef vector<BmImapNestedStringList> BmItemVect;
public:
	BmImapNestedStringList();
	~BmImapNestedStringList();

	const char* Parse(const char* data);

	const BmString& Text() const			{ return mText; }

	inline bool IsLeaf() const				{ return mChildren.empty(); }
	inline uint32 Size() const				{ return mChildren.size(); }

	const BmImapNestedStringList& operator[] (uint32 index) const;
	
private:
	const char* _Parse(const char* data);
	const char* _ParseQuotedString(const char* data);
	const char* _ParseString(const char* data);

	BmItemVect mChildren;
	BmString mText;
};

#endif
