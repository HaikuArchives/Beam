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

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMail.h"
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
						mQuoteFormattingControl = new BmMenuControl( "Quote-formatting:", new BPopUpMenu("")),
						new Space( minimax(0,4,0,4)),
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
	divider = MAX( divider, mQuoteFormattingControl->Divider());
	divider = MAX( divider, mDefaultEncodingControl->Divider());
	mMaxLineLenControl->SetDivider( divider);
	mQuotingStringControl->SetDivider( divider);
	mQuoteFormattingControl->SetDivider( divider);
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

	TheBubbleHelper.SetHelp( mMaxLineLenControl, "Here you can enter the maximum number of characters\nper line Beam should allow in the mailtext.\nThis corresponds to the right margin in the mail-editor.");
	TheBubbleHelper.SetHelp( mQuotingStringControl, "Here you can enter the string used for quoting.\nThis string will be prepended to every quoted line.");
	TheBubbleHelper.SetHelp( mQuoteFormattingControl, "This menu controls the way Beam formats quoted lines of a reply/forward.\n\n\
Simple:\n\
	Beam will simply prepend the quote-string to every line. Lines that exceed\n\
	the maximum line length will be wrapped around, resulting in a very short line.\n\
	This sooner or later causes ugly mails with a comb-like layout (alternating long \n\
	and short lines).\n\
Push Margin:\n\
	In this mode, Beam will push the right margin of a reply/forward just as needed.\n\
	The original formatting of the mail stays intact, but the mail may exceed 80 chars\n\
	per line (which will annoy recipients using terminal-based mailers).\n\
Auto Wrap:\n\
	This will cause Beam to automatically rewrap blocks of quoted lines in a way that\n\
	tries to keep the paragraph structure of the original mail intact.\n\
\n\
Since 'Auto Wrap' usually gives the best results, Beam uses it by default, but if you \n\
encounter mails where the automatic wrapping doesn't work, you should try one \n\
of the other modes.");
	TheBubbleHelper.SetHelp( mForwardIntroStrControl, "Here you can enter a string that will \nappear at the top of every forwarded mail.\n\
The following macros are supported:\n\
	%D  -  expands to the original mail's date.\n\
	%T  -  expands to the original mail's time.\n\
	%F  -  expands to the sender of the original mail.");
	TheBubbleHelper.SetHelp( mForwardSubjectStrControl, "Here you can influence the subject-string \nBeam generates for a forwarded mail.\n\
The following macros are supported:\n\
	%S  -  expands to the original mail's subject.");
	TheBubbleHelper.SetHelp( mForwardSubjectRxControl, "This string is the regular-expression (perl-style) Beam uses\nto determine whether a given subject indicates\nthat the mail already is a forward.\nThis way subjects like \n\t'Fwd: Fwd: Fwd: fun-stuff'\ncan be avoided.");
	TheBubbleHelper.SetHelp( mReplyIntroStrControl, "Here you can enter a string that will \nappear at the top of every reply.\n\
The following macros are supported:\n\
	%D  -  expands to the original mail's date.\n\
	%T  -  expands to the original mail's time.\n\
	%F  -  expands to the sender of the original mail.");
	TheBubbleHelper.SetHelp( mReplySubjectStrControl, "Here you can influence the subject-string \nBeam generates for a reply.\n\
The following macros are supported:\n\
	%S  -  expands to the original mail's subject.");
	TheBubbleHelper.SetHelp( mReplySubjectRxControl, "This string is the regular-expression (perl-style) Beam uses\nto determine whether a given subject indicates\nthat the mail already is a reply.\nThis way subjects like \n\t'Re: Re: Re: your offer'\ncan be avoided.");
	TheBubbleHelper.SetHelp( mDefaultEncodingControl, "Here you can select the charset-encoding Beam should usually use.");
	TheBubbleHelper.SetHelp( mSpecialForEachBccControl, "Here you can select the way Beam sends mails with Bcc recipients\n\
	\n\
Checked:\n\
	Beam will send separate mails to each Bcc-recipient, each of which will \n\
	contain exactly one Bcc-header (the current recipient). \n\
	This results in more traffic, but has the advantage that each mail actually \n\
	contains the recipients address in the header, making it look less like spam.\n\
Unchecked:\n\
	Beam will send a single mail to all recipients, which does not contain any \n\
	Bcc-headers. \n\
	This results in less network traffic but makes it more likely that the mail\n\
	is filtered right into the spam-folder on the recipient's side.");
	TheBubbleHelper.SetHelp( mPreferUserAgentControl, "Email-clients used to identify themselves in a header-field called 'X-Mailer'.\nLately, the use of a header-field named 'UserAgent' became popular.\nBy checking/unchecking this control you can decide which field Beam should use.");
	TheBubbleHelper.SetHelp( mGenerateIDsControl, "This control determines if Beam should generate the message-IDs\n\
used to uniquely identify every mail on the net.\n\
If unchecked, the SMTP-server will generate these IDs, which is usually ok,\n\
but makes sorting mails by thread less reliable (since mail-threading is \n\
currently not implemented this doesn't apply for now).");
	TheBubbleHelper.SetHelp( mMakeQpSafeControl, "This makes Beam generate quoted-printables that are safe for EBCDIC-gateways\n (if you don't know what EBCDIC is, don't worry and leave this as is).");
	TheBubbleHelper.SetHelp( mDefaultForwardTypeControl, "Here you can select the forwarding-type Beam should use when you press the 'Forward'-button.");
	TheBubbleHelper.SetHelp( mDontAttachVCardsControl, "Checking this causes Beam to NOT include \nvcard-attachments (appended address-info) in a forwarded mail.");

	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		AddItemToMenu( mDefaultEncodingControl->Menu(), 
							new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_ENCODING_SELECTED)), this);
	}
	mDefaultEncodingControl->MarkItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());

	// add quote-formattings:
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_AUTO_WRAP, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_PUSH_MARGIN, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_SIMPLE, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	mQuoteFormattingControl->MarkItem( ThePrefs->GetString("QuoteFormatting", BmMail::BM_QUOTE_AUTO_WRAP).String());

	// add forward-types:
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
	// prefs are already stored by General View
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::UndoChanges() {
	// prefs are already undone by General View
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
					ThePrefs->SetString("ForwardIntroStr", mForwardIntroStrControl->Text());
				else if ( source == mForwardSubjectStrControl)
					ThePrefs->SetString("ForwardSubjectStr", mForwardSubjectStrControl->Text());
				else if ( source == mForwardSubjectRxControl)
					ThePrefs->SetString("ForwardSubjectRX", mForwardSubjectRxControl->Text());
				else if ( source == mReplyIntroStrControl)
					ThePrefs->SetString("ReplyIntroStr", mReplyIntroStrControl->Text());
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
			case BM_QUOTE_FORMATTING_SELECTED: {
				BMenuItem* item = mQuoteFormattingControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "QuoteFormatting", item->Label());
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

