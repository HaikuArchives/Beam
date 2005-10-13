/*
	BmTextViewCompleter.cpp
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

#include <AppDefs.h>
#include <ListView.h>
#include <Looper.h>
#include <Message.h>
#include <Screen.h>
#include <TextControl.h>
#include <Window.h>

#include "BmBasics.h"

#include "BmAutoCompleterVariations.h"
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
			// we just make sure that the choices-view is closed when tabbing out:
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
