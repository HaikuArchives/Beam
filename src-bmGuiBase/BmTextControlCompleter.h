/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmTextViewCompleter_h
#define _BmTextViewCompleter_h

#include <MessageFilter.h>

#include "BmGuiBase.h"

#include "BmAutoCompleter.h"

class BTextControl;
class IMPEXPBMGUIBASE BmTextControlCompleter
	: protected BmAutoCompleter
	, public BMessageFilter
{
public:
	BmTextControlCompleter(BTextControl* textControl, 
								  ChoiceModel* choiceModel = NULL,
								  PatternSelector* patternSelector = NULL);
	virtual ~BmTextControlCompleter();
	
private:
	virtual filter_result Filter(BMessage *message, BHandler **target);
	
	class TextControlWrapper : public EditView
	{
	public:
		TextControlWrapper(BTextControl* textControl);
		virtual BRect GetAdjustmentFrame();
		virtual void GetEditViewState( BmString& text, int32* caretPos);
		virtual void SetEditViewState( const BmString& text, int32 caretPos,
												 int32 selectionLength = 0);
	private:
		BTextControl* mTextControl;
	};
};


#endif
