/*
	BmTextViewCompleter.h
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
