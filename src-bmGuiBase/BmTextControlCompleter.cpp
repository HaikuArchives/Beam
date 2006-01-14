/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Looper.h>
#include <TextControl.h>

#include "BmBasics.h"

#include "BmAutoCompleterDefaultImpl.h"
#include "BmTextControlCompleter.h"

// #pragma mark - TextControlWrapper
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControlCompleter::TextControlWrapper
::TextControlWrapper(BTextControl* textControl)
	:	mTextControl(textControl)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControlCompleter::TextControlWrapper
::GetEditViewState( BmString& text, int32* caretPos)
{
	if (mTextControl && mTextControl->LockLooper()) {
		text = mTextControl->Text();
		if (caretPos) {
			int32 end;
			mTextControl->TextView()->GetSelection(caretPos, &end);
		}
		mTextControl->UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControlCompleter::TextControlWrapper
::SetEditViewState( const BmString& text, int32 caretPos, int32 selectionLength)
{
	if (mTextControl && mTextControl->LockLooper()) {
		mTextControl->TextView()->SetText(text.String(), text.Length());
		mTextControl->TextView()->Select(caretPos, caretPos+selectionLength);
		mTextControl->TextView()->ScrollToSelection();
		mTextControl->UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmTextControlCompleter::TextControlWrapper::GetAdjustmentFrame()
{
	BRect frame = mTextControl->TextView()->Bounds();
	frame = mTextControl->TextView()->ConvertToScreen(frame);
	frame.InsetBy(-1, -3);
	return frame;
}

// #pragma mark - BmTextControlCompleter
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControlCompleter::BmTextControlCompleter(BTextControl* textControl, 
															  ChoiceModel* model,
															  PatternSelector* patternSelector)
	:	BmAutoCompleter(new TextControlWrapper(textControl), model, 
							 new BmDefaultChoiceView(), patternSelector)
	,	BMessageFilter(B_KEY_DOWN)
{
	textControl->TextView()->AddFilter(this);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControlCompleter::~BmTextControlCompleter()
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
filter_result BmTextControlCompleter::Filter(BMessage *message, 
															BHandler **target)
{
	int32 rawChar, modifiers;
	if	(!target || message->FindInt32("raw_char", &rawChar) != B_OK
	|| message->FindInt32("modifiers", &modifiers) != B_OK)
		return B_DISPATCH_MESSAGE;
	
	switch (rawChar) {
		case B_UP_ARROW:
			SelectPrevious();
			return B_SKIP_MESSAGE;
		case B_DOWN_ARROW:
			SelectNext();
			return B_SKIP_MESSAGE;
		case B_ESCAPE:
			CancelChoice();
			return B_SKIP_MESSAGE;
		case B_RETURN:
			ApplyChoice();
			EditViewStateChanged();
			return B_SKIP_MESSAGE;
		case B_TAB: {
			// make sure that the choices-view is closed when tabbing out:
			CancelChoice();
			return B_DISPATCH_MESSAGE;
		}
		default:
			// dispatch message to textview manually...
			Looper()->DispatchMessage(message, *target);
			// ...and propagate the new state to the auto-completer:
			EditViewStateChanged();
			return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}
