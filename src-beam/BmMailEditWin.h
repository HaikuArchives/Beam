/*
	BmMailEditWin.h
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


#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include <map>

#include <MessageFilter.h>

#include "BmWindow.h"

class MMenuBar;

class BmCheckControl;
class BmMail;
class BmMailView;
class BmMailRef;
class BmMailViewContainer;
class BmMenuControl;
class BmToolbarButton;
class BmTextControl;
class CLVContainerView;
class HGroup;
class MPictureButton;
class Space;
class VGroup;
class BFilePanel;

class BmMailEditWin : public BmWindow
{
	typedef BmWindow inherited;
	typedef map< const entry_ref, BmMailEditWin*> BmEditWinMap;

	class BmMsgFilter : public BMessageFilter {
	public:
		BmMsgFilter( BControl* stControl, uint32 cmd)
			: 	BMessageFilter( B_ANY_DELIVERY, B_ANY_SOURCE, cmd) 
			,	mShiftTabToControl( stControl)
		{
		}
		filter_result Filter( BMessage* msg, BHandler** handler);
	private:
		BControl* mShiftTabToControl;
	};

public:
	// creator-funcs, c'tors and d'tor:
	static BmMailEditWin* CreateInstance( BmMailRef* mailRef=NULL);
	static BmMailEditWin* CreateInstance( BmMail* mail=NULL);
	~BmMailEditWin();

	// overrides of BmWindow base:
	void BeginLife();
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	void Show();
	status_t UnarchiveState( BMessage* archive);
	
	// getters:
	BmRef<BmMail> CurrMail() const;

private:
	// hide constructors:
	BmMailEditWin();
	BmMailEditWin( BmMailRef* mailRef=NULL, BmMail* mail=NULL);

	void EditMail( BmMailRef* ref);
	void EditMail( BmMail* mail);
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	void CreateGUI();
	MMenuBar* CreateMenu();

	bool CreateMailFromFields();
	bool SaveMail();
	void SetFieldsFromMail( BmMail* mail);
	
	static BmEditWinMap nEditWinMap;

	BmMailView* mMailView;
	
	MPictureButton* mShowDetails1Button;
	MPictureButton* mShowDetails2Button;
	MPictureButton* mShowDetails3Button;

	BmToolbarButton* mSendButton;
	BmToolbarButton* mSaveButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mAttachButton;
	BmToolbarButton* mPeopleButton;
	BmToolbarButton* mPrintButton;
	
	BmTextControl* mBccControl;
	BmTextControl* mCcControl;
	BmTextControl* mFromControl;
	BmTextControl* mReplyToControl;
	BmTextControl* mSenderControl;
	BmTextControl* mSubjectControl;
	BmTextControl* mToControl;
	
	BmMenuControl* mCharsetControl;
	BmMenuControl* mSmtpControl;
	BmMenuControl* mSignatureControl;
	
	BmCheckControl* mEditHeaderControl;

	bool mShowDetails1;
	bool mShowDetails2;
	bool mShowDetails3;
	VGroup* mDetails1Group;
	HGroup* mDetails2Group;
	HGroup* mDetails3Group;
	HGroup* mSubjectGroup;
	VGroup* mOuterGroup;
	Space* mSeparator;
	bool mModified;
	bool mHasNeverBeenSaved;

	BFilePanel* mAttachPanel;

	static float nNextXPos;
	static float nNextYPos;

	// Hide copy-constructor and assignment:
	BmMailEditWin( const BmMailEditWin&);
	BmMailEditWin operator=( const BmMailEditWin&);
};



#endif
