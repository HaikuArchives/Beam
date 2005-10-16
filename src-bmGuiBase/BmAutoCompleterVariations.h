/*
	BmAutoCompleterVariations.h
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


#ifndef _BmAutoCompleterVariations_h
#define _BmAutoCompleterVariations_h

#include <MessageFilter.h>

#include "BmGuiBase.h"

#include "BmString.h"

#include "BmAutoCompleter.h"

class IMPEXPBMGUIBASE BmDefaultPatternSelector 
	: public BmAutoCompleter::PatternSelector
{
public:
	virtual void SelectPatternBounds( const BmString& text, int32 caretPos,
												 int32* start, int32* length);
};


class IMPEXPBMGUIBASE BmDefaultCompletionStyle 
	: public BmAutoCompleter::CompletionStyle
{
public:
	BmDefaultCompletionStyle(BmAutoCompleter::EditView* editView, 
									 BmAutoCompleter::ChoiceModel* choiceModel,
									 BmAutoCompleter::ChoiceView* choiceView, 
									 BmAutoCompleter::PatternSelector* patternSelector);
	virtual ~BmDefaultCompletionStyle();

	virtual bool Select(int32 index);
	virtual bool SelectNext(bool wrap = false);
	virtual bool SelectPrevious(bool wrap = false);

	virtual void ApplyChoice(bool hideChoices = true);
	virtual void CancelChoice();

	virtual void EditViewStateChanged();

private:
	BmString mFullEnteredText;
	int32 mSelectedIndex;
	int32 mPatternStartPos;
	int32 mPatternLength;
};


class IMPEXPBMGUIBASE BmDefaultChoiceView 
	: public BmAutoCompleter::ChoiceView
{
protected:
	class ListView : public BListView
	{
	public:
		ListView(BmAutoCompleter::CompletionStyle* completer);
		virtual void SelectionChanged();
		virtual void MessageReceived(BMessage* msg);
		virtual void MouseDown(BPoint point);
		virtual void AttachedToWindow();
	private:
		BmAutoCompleter::CompletionStyle* mCompleter;
	};

	class ListItem : public BListItem
	{
	public:
		ListItem(const BmAutoCompleter::Choice* choice);
		virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
	private:
		BmString mPreText;
		BmString mMatchText;
		BmString mPostText;
	};
	
	
public:
	BmDefaultChoiceView();
	virtual ~BmDefaultChoiceView();
	
	virtual void SelectChoiceAt(int32 index);
	virtual void ShowChoices(BmAutoCompleter::CompletionStyle* completer);
	virtual void HideChoices();
	virtual bool ChoicesAreShown();

private:
	BWindow* mWindow;
	ListView* mListView;
};

#endif
