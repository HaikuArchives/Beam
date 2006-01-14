/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailAddrCompleter_h
#define _BmMailAddrCompleter_h

#include "BmTextControlCompleter.h"

class BmMailAddressCompleter
	: public BmTextControlCompleter
{
	class MailAddrChoiceModel : public ChoiceModel
	{
	public:
		virtual void FetchChoicesFor(const BmString& pattern);
		//
		virtual int32 CountChoices() const;
		virtual const Choice* ChoiceAt(int32 index) const;
	private:
		BList mChoicesList;
	};
	

	class MailAddrPatternSelector : public BmAutoCompleter::PatternSelector
	{
	public:
		virtual void SelectPatternBounds( const BmString& text, int32 caretPos,
													 int32* start, int32* length);
	};

public:
	BmMailAddressCompleter(BTextControl* textControl);
};
	

#endif
