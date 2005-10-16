/*
	BmMailEditWin.cpp
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

#include <cctype>

#include <FilePanel.h>
#include <MenuItem.h>
#include <MessageFilter.h>

#include "BmString.h"

#include <HGroup.h>
#include <VGroup.h>
#include <MBorder.h>
#include <MMenuBar.h>
#include <Space.h>

#include "TextEntryAlert.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmCheckControl.h"
#include "BmDataModel.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailAddrCompleter.h"
#include "BmMailFolder.h"
#include "BmMailEditWin.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"

// #pragma mark - MailAddrChoiceModel
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static int MailAddrSorter(const void* l, const void* r) 
{
	const BmAutoCompleter::Choice* left 
		= *static_cast<const BmAutoCompleter::Choice**>(l);
	const BmAutoCompleter::Choice* right 
		= *static_cast<const BmAutoCompleter::Choice**>(r);
	BmString leftStr(left->Text());
	leftStr.RemoveSet("\"'");
	BmString rightStr(right->Text());
	rightStr.RemoveSet("\"'");
	return strcasecmp(leftStr.String(), rightStr.String());
}

void BmMailAddressCompleter::MailAddrChoiceModel
::FetchChoicesFor(const BmString& pattern)
{
	int32 count = mChoicesList.CountItems();
	for( int32 i=0; i<count; ++i)
		delete static_cast<Choice*>(mChoicesList.ItemAt(i));
	mChoicesList.MakeEmpty();
	if (pattern.Length() == 0)
		return;
	BmAutolockCheckGlobal lock( ThePeopleList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "MailAddrChoiceModel: Unable to get lock on people-list");
	BmModelItemMap::const_iterator iter;
	for( iter = ThePeopleList->begin(); iter != ThePeopleList->end(); 
		  ++iter) {
		BmPerson* person = dynamic_cast< BmPerson*>( iter->second.Get());
		if (person) {
			// TODO: maybe it'd make more sense to only show the default (first)
			// email for each person, as otherwise the list may get rather long
			// (without improving the usefulness a lot).
			const BmStringVect& emails = person->Emails();
			for( uint32 e=0; e<emails.size(); ++e) {
				BmString mailAddr 
					= BmPerson::GenerateMailAddr(person->Name(), emails[e], 
														  person->Nick());
				if (person->Name().ICompare(pattern, pattern.Length()) == 0
				|| emails[e].ICompare(pattern, pattern.Length()) == 0
				|| person->Nick().ICompare(pattern, pattern.Length()) == 0
				|| mailAddr.ICompare(pattern, pattern.Length()) == 0) {
					mChoicesList.AddItem(new Choice(mailAddr));
				}
			}
		}
	}
	mChoicesList.SortItems(MailAddrSorter);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMailAddressCompleter::MailAddrChoiceModel
::CountChoices() const
{
	return mChoicesList.CountItems();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const BmAutoCompleter::Choice* BmMailAddressCompleter::MailAddrChoiceModel
::ChoiceAt(int32 index) const
{
	return static_cast<Choice*>(mChoicesList.ItemAt(index));
}
	

// #pragma mark - MailAddrPatternSelector
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailAddressCompleter::MailAddrPatternSelector
::SelectPatternBounds( const BmString& text, int32 caretPos,
							  int32* start, int32* length)
{
	if (!text.Length()) {
		*start = *length = 0;
		return;
	}
	const char* pos = text.String();
	const char* endPos;
	int32 pattStart = 0;
	int32 pattLen = 0;
	while( *pos) {
		if (*pos == '"') {
			// quoted-string started, we skip over it en-block (avoiding
			// to interprete any contained commas as mail-addr separators).
			for( 	endPos=pos+1; 
					*endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string.
				int32 numChars = 1+endPos-pos;
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the 
				// remainder to be part of the quoted string:
				pos = endPos;
			}
		} else {
			// we copy characters until we find the start of a quoted string or 
			// the separator char:
			for(  endPos=pos; *endPos && *endPos!='"' && *endPos!=','; ++endPos)
				;
			int32 numChars = endPos-pos;
			pos += numChars;
			if (*endPos == ',') {
				pattLen = endPos-text.String()-pattStart;
				if (caretPos <= pattStart+pattLen)
					break;
				pos++;
				pattStart = pos-text.String();
			}
			if (!*endPos)
				pattLen = endPos-text.String()-pattStart;
		}
	}
	while(isspace(text[pattStart])) {
		pattStart++;
		pattLen--;
	}
	*start = pattStart;
	*length = pattLen;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailAddressCompleter::BmMailAddressCompleter(BTextControl* textControl)
	: BmTextControlCompleter(textControl, new MailAddrChoiceModel(), 
									 new MailAddrPatternSelector())
{
}
