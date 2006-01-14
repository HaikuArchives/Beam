/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Looper.h>
#include <Message.h>

#include "BmBasics.h"

#include "BmAutoCompleter.h"
#include "BmAutoCompleterDefaultImpl.h"

// #pragma mark - DefaultPatternSelector
class DefaultPatternSelector : public BmAutoCompleter::PatternSelector
{
public:
	virtual void SelectPatternBounds( const BmString& text, int32 caretPos,
												 int32* start, int32* length);
};
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void DefaultPatternSelector
::SelectPatternBounds( const BmString& text, int32 caretPos,
							  int32* start, int32* length)
{
	if (!start || !length)
		return;
	*start = 0;
	*length = text.Length();
}

// #pragma mark - CompletionStyle
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmAutoCompleter::CompletionStyle
::CompletionStyle(EditView* editView, ChoiceModel* choiceModel,
						ChoiceView* choiceView, 
						PatternSelector* patternSelector)
	:	mEditView(editView)
	,	mChoiceModel(choiceModel)
	,	mChoiceView(choiceView)
	,	mPatternSelector(patternSelector 
									? patternSelector 
									: new DefaultPatternSelector())
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmAutoCompleter::CompletionStyle::~CompletionStyle()
{
	delete mEditView;
	delete mChoiceModel;
	delete mChoiceView;
	delete mPatternSelector;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::CompletionStyle::SetEditView(EditView* view)
{
	delete mEditView;
	mEditView = view;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::CompletionStyle
::SetPatternSelector(PatternSelector* selector)
{
	delete mPatternSelector;
	mPatternSelector = selector;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::CompletionStyle::SetChoiceModel(ChoiceModel* model)
{
	delete mChoiceModel;
	mChoiceModel = model;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::CompletionStyle::SetChoiceView(ChoiceView* view)
{
	delete mChoiceView;
	mChoiceView = view;
}

// #pragma mark - BmAutoCompleter
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmAutoCompleter::BmAutoCompleter(CompletionStyle* completionStyle)
	:	mCompletionStyle(completionStyle)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmAutoCompleter::BmAutoCompleter(EditView* editView, 
										   ChoiceModel* choiceModel,
										   ChoiceView* choiceView, 
										   PatternSelector* patternSelector)
	:	mCompletionStyle(
			new BmDefaultCompletionStyle(editView, choiceModel,
											     choiceView, patternSelector)
		)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmAutoCompleter::~BmAutoCompleter()
{
	delete mCompletionStyle;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
bool BmAutoCompleter::Select(int32 index)
{
	if (mCompletionStyle)
		return mCompletionStyle->Select(index);
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
bool BmAutoCompleter::SelectNext(bool wrap)
{
	if (mCompletionStyle)
		return mCompletionStyle->SelectNext(wrap);
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
bool BmAutoCompleter::SelectPrevious(bool wrap)
{
	if (mCompletionStyle)
		return mCompletionStyle->SelectPrevious(wrap);
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::ApplyChoice(bool hideChoices)
{
	if (mCompletionStyle)
		mCompletionStyle->ApplyChoice(hideChoices);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::CancelChoice()
{
	if (mCompletionStyle)
		mCompletionStyle->CancelChoice();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::EditViewStateChanged()
{
	if (mCompletionStyle)
		mCompletionStyle->EditViewStateChanged();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::SetEditView(EditView* view)
{
	if (mCompletionStyle)
		mCompletionStyle->SetEditView(view);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::SetPatternSelector(PatternSelector* selector)
{
	if (mCompletionStyle)
		mCompletionStyle->SetPatternSelector(selector);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::SetChoiceModel(ChoiceModel* model)
{
	if (mCompletionStyle)
		mCompletionStyle->SetChoiceModel(model);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::SetChoiceView(ChoiceView* view)
{
	if (mCompletionStyle)
		mCompletionStyle->SetChoiceView(view);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmAutoCompleter::SetCompletionStyle(CompletionStyle* style)
{
	delete mCompletionStyle;
	mCompletionStyle = style;
}
