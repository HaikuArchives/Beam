/*
	BmAutoCompleter.h
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


#ifndef _BmAutoCompleter_h
#define _BmAutoCompleter_h

#include <MessageFilter.h>

#include "BmGuiBase.h"

#include "BmString.h"

class IMPEXPBMGUIBASE BmAutoCompleter
{
public:
	class Choice
	{
	public:
		Choice( const BmString& choiceText, const BmString& displayText, 
				  int32 matchPos, int32 matchLen)
			:	mText(choiceText)
			,	mDisplayText(displayText)
			,	mMatchPos(matchPos)
			,	mMatchLen(matchLen)			{}
		virtual ~Choice()						{}
		const BmString& Text() const		{ return mText; }
		const BmString& DisplayText() const		
													{ return mDisplayText; }
		int32 MatchPos() const				{ return mMatchPos; }
		int32 MatchLen() const				{ return mMatchLen; }
	private:
		BmString mText;
		BmString mDisplayText;
		int32 mMatchPos;
		int32 mMatchLen;
	};

	class EditView
	{
	public:
		virtual ~EditView()					{}

		virtual BRect GetAdjustmentFrame() = 0;
		virtual void GetEditViewState( BmString& text, int32* caretPos) = 0;
		virtual void SetEditViewState( const BmString& text, int32 caretPos,
												 int32 selectionLength = 0) = 0;
	};

	class PatternSelector
	{
	public:
		virtual ~PatternSelector()	{}
		
		virtual void SelectPatternBounds( const BmString& text, int32 caretPos,
													 int32* start, int32* length) = 0;
	};

	class ChoiceModel
	{
	public:
	
		virtual ~ChoiceModel()				{}
		
		virtual void FetchChoicesFor(const BmString& pattern) = 0;

		virtual int32 CountChoices() const = 0;
		virtual const Choice* ChoiceAt(int32 index) const = 0;
	};
	
	class CompletionStyle;
	class ChoiceView
	{
	public:
		virtual ~ChoiceView()				{}

		virtual void SelectChoiceAt(int32 index) = 0;
		virtual void ShowChoices(BmAutoCompleter::CompletionStyle* completer) = 0;
		virtual void HideChoices() = 0;
		virtual bool ChoicesAreShown() = 0;
	};

	class CompletionStyle
	{
	public:
		CompletionStyle(EditView* editView, ChoiceModel* choiceModel,
							 ChoiceView* choiceView, 
							 PatternSelector* patternSelector);
		virtual ~CompletionStyle();

		virtual bool Select(int32 index) = 0;
		virtual bool SelectNext(bool wrap = false) = 0;
		virtual bool SelectPrevious(bool wrap = false) = 0;

		virtual void ApplyChoice(bool hideChoices = true) = 0;
		virtual void CancelChoice() = 0;

		virtual void EditViewStateChanged() = 0;

		void SetEditView(EditView* view);
		void SetPatternSelector(PatternSelector* selector);
		void SetChoiceModel(ChoiceModel* model);
		void SetChoiceView(ChoiceView* view);

		EditView* GetEditView()				{ return mEditView; }
		PatternSelector* GetPatternSelector()
													{ return mPatternSelector; }
		ChoiceModel* GetChoiceModel()		{ return mChoiceModel; }
		ChoiceView* GetChoiceView()		{ return mChoiceView; }

	protected:
		EditView* mEditView;
		PatternSelector* mPatternSelector;
		ChoiceModel* mChoiceModel;
		ChoiceView* mChoiceView;
	};

protected:
	BmAutoCompleter(CompletionStyle* completionStyle = NULL);
	BmAutoCompleter(EditView* editView, ChoiceModel* choiceModel,
						 ChoiceView* choiceView, PatternSelector* patternSelector);
	virtual ~BmAutoCompleter();
	
	void EditViewStateChanged();

	bool Select(int32 index);
	bool SelectNext(bool wrap = false);
	bool SelectPrevious(bool wrap = false);

	void ApplyChoice(bool hideChoices = true);
	void CancelChoice();
	
	void SetEditView(EditView* view);
	void SetPatternSelector(PatternSelector* selector);
	void SetChoiceModel(ChoiceModel* model);
	void SetChoiceView(ChoiceView* view);

	void SetCompletionStyle(CompletionStyle* style);
	
private:
	CompletionStyle* mCompletionStyle;
};


#endif
