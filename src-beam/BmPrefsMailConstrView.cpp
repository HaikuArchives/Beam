/*
	BmPrefsMailConstrView.cpp
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

#include <MenuItem.h>
#include <PopUpMenu.h>

#include <layout-all.h>

#include "Colors.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmPrefsMailConstrView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsMailConstrView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailConstrView::BmPrefsMailConstrView() 
	:	inherited( "Sending Mail")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Construction",
					new VGroup(
						mMaxLineLenControl = new BmTextControl( "Wrap lines at (chars):"),
						mQuotingStringControl = new BmTextControl( "Quoting string:"),
						mDefaultEncodingControl = new BmMenuControl( "Default-encoding:", new BPopUpMenu("")),
						new Space( minimax(0,4,0,4)),
						mSpecialForEachBccControl = new BmCheckControl( "Generate header for each Bcc-recipient", 
																					   new BMessage(BM_EACH_BCC_CHANGED), 
																					   this, ThePrefs->GetBool("SpecialHeaderForEachBcc")),
						mPreferUserAgentControl = new BmCheckControl( "Prefer 'UserAgent'-header over 'X-Mailer'", 
																					 new BMessage(BM_PREFER_USER_AGENT_CHANGED), 
																					 this, ThePrefs->GetBool("PreferUserAgentOverX-Mailer")),
						mGenerateIDsControl = new BmCheckControl( "Generate own message-IDs", 
																				new BMessage(BM_GENERATE_MSGIDS_CHANGED), 
																				this, ThePrefs->GetBool("GenerateOwnMessageIDs")),
						mMakeQpSafeControl = new BmCheckControl( "Make quoted-printable safe for non-ASCII gateways (EBCDIC)", 
																			  new BMessage(BM_QP_SAFE_CHANGED), 
																			  this, ThePrefs->GetBool("MakeQPSafeForEBCDIC")),
						0
					)
				),
				new Space(),
				0
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Forwarding",
				new VGroup(
					mForwardIntroStrControl = new BmTextControl( "Intro:"),
					mForwardSubjectStrControl = new BmTextControl( "Subject:"),
					mForwardSubjectRxControl = new BmTextControl( "Regex that checks if forward:"),
					new HGroup( 
						mDefaultForwardTypeControl = new BmMenuControl( "Default forward-type:", new BPopUpMenu("")),
						new Space(),
						0
					),
					new Space( minimax(0,4,0,4)),
					mDontAttachVCardsControl = new BmCheckControl( "Do not attach v-cards in forward", 
																				  new BMessage(BM_ATTACH_VCARDS_CHANGED), 
																				  this, ThePrefs->GetBool("DoNotAttachVCardsToForward")),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Replying",
				new VGroup(
					mReplyIntroStrControl = new BmTextControl( "Intro:"),
					mReplySubjectStrControl = new BmTextControl( "Subject:"),
					mReplySubjectRxControl = new BmTextControl( "Regex that checks if reply:"),
					new Space(),
					0
				)
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mMaxLineLenControl->Divider();
	divider = MAX( divider, mQuotingStringControl->Divider());
	divider = MAX( divider, mDefaultEncodingControl->Divider());
	mMaxLineLenControl->SetDivider( divider);
	mQuotingStringControl->SetDivider( divider);
	mDefaultEncodingControl->SetDivider( divider);

	divider = mForwardIntroStrControl->Divider();
	divider = MAX( divider, mForwardSubjectStrControl->Divider());
	divider = MAX( divider, mForwardSubjectRxControl->Divider());
	divider = MAX( divider, mDefaultForwardTypeControl->Divider());
	mForwardIntroStrControl->SetDivider( divider);
	mForwardSubjectStrControl->SetDivider( divider);
	mForwardSubjectRxControl->SetDivider( divider);
	mDefaultForwardTypeControl->SetDivider( divider);
	
	divider = mReplyIntroStrControl->Divider();
	divider = MAX( divider, mReplySubjectStrControl->Divider());
	divider = MAX( divider, mReplySubjectRxControl->Divider());
	mReplyIntroStrControl->SetDivider( divider);
	mReplySubjectStrControl->SetDivider( divider);
	mReplySubjectRxControl->SetDivider( divider);

	BString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetText( val.String());
	mQuotingStringControl->SetText( ThePrefs->GetString("QuotingString").String());
	mForwardIntroStrControl->SetText( ThePrefs->GetString("ForwardIntroStr").String());
	mForwardSubjectStrControl->SetText( ThePrefs->GetString("ForwardSubjectStr").String());
	mForwardSubjectRxControl->SetText( ThePrefs->GetString("ForwardSubjectRX").String());
	mReplyIntroStrControl->SetText( ThePrefs->GetString("ReplyIntroStr").String());
	mReplySubjectStrControl->SetText( ThePrefs->GetString("ReplySubjectStr").String());
	mReplySubjectRxControl->SetText( ThePrefs->GetString("ReplySubjectRX").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailConstrView::~BmPrefsMailConstrView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::Initialize() {
	inherited::Initialize();

	mMaxLineLenControl->SetTarget( this);
	mQuotingStringControl->SetTarget( this);
	mForwardIntroStrControl->SetTarget( this);
	mForwardSubjectStrControl->SetTarget( this);
	mForwardSubjectRxControl->SetTarget( this);
	mReplyIntroStrControl->SetTarget( this);
	mReplySubjectStrControl->SetTarget( this);
	mReplySubjectRxControl->SetTarget( this);

	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		AddItemToMenu( mDefaultEncodingControl->Menu(), 
							new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_ENCODING_SELECTED)), this);
	}
	mDefaultEncodingControl->MarkItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());

	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Attached", new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);
	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Inline", new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);
	BString markItem = ThePrefs->GetInt( "DefaultForwardType") == BMM_FORWARD_INLINE 
								? "Inline"
								: "Attached";
	mDefaultForwardTypeControl->MarkItem( markItem.String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::SaveData() {
//	ThePrefs->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::UndoChanges() {
//	ThePrefs = NULL;
//	BmPrefs::CreateInstance();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( source == mMaxLineLenControl)
					ThePrefs->SetInt("MaxLineLen", atoi(mMaxLineLenControl->Text()));
				else if ( source == mQuotingStringControl)
					ThePrefs->SetString("QuotingString", mQuotingStringControl->Text());
				else if ( source == mForwardIntroStrControl)
					ThePrefs->SetString("ForwardIntoStr", mForwardIntroStrControl->Text());
				else if ( source == mForwardSubjectStrControl)
					ThePrefs->SetString("ForwardSubjectStr", mForwardSubjectStrControl->Text());
				else if ( source == mForwardSubjectRxControl)
					ThePrefs->SetString("ForwardSubjectRX", mForwardSubjectRxControl->Text());
				else if ( source == mReplyIntroStrControl)
					ThePrefs->SetString("ReplyIntoStr", mReplyIntroStrControl->Text());
				else if ( source == mReplySubjectStrControl)
					ThePrefs->SetString("ReplySubjectStr", mReplySubjectStrControl->Text());
				else if ( source == mReplySubjectRxControl)
					ThePrefs->SetString("ReplySubjectRX", mReplySubjectRxControl->Text());
				break;
			}
			case BM_EACH_BCC_CHANGED: {
				ThePrefs->SetBool("SpecialHeaderForEachBcc", mSpecialForEachBccControl->Value());
				break;
			}
			case BM_PREFER_USER_AGENT_CHANGED: {
				ThePrefs->SetBool("PreferUserAgentOverX-Mailer", mPreferUserAgentControl->Value());
				break;
			}
			case BM_GENERATE_MSGIDS_CHANGED: {
				ThePrefs->SetBool("GenerateOwnMessageIDs", mGenerateIDsControl->Value());
				break;
			}
			case BM_QP_SAFE_CHANGED: {
				ThePrefs->SetBool("MakeQPSafeForEBCDIC", mMakeQpSafeControl->Value());
				break;
			}
			case BM_ATTACH_VCARDS_CHANGED: {
				ThePrefs->SetBool("DoNotAttachVCardsToForward", mDontAttachVCardsControl->Value());
				break;
			}
			case BM_ENCODING_SELECTED: {
				BMenuItem* item = mDefaultEncodingControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetInt("DefaultEncoding", CharsetToEncoding( item->Label()));
				break;
			}
			case BM_FORWARD_TYPE_SELECTED: {
				BMenuItem* item = mDefaultForwardTypeControl->Menu()->FindMarked();
				if (item) {
					if (BString("Attached").Compare( item->Label()) == 0)
						ThePrefs->SetInt("DefaultForwardType", BMM_FORWARD_ATTACHED);
					else
						ThePrefs->SetInt("DefaultForwardType", BMM_FORWARD_INLINE);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

