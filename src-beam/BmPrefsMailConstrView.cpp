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

#include <FilePanel.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
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
	,	mPeoplePanel( NULL)
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Editing",
					new VGroup(
						mMaxLineLenControl 
							= new BmTextControl( "Set right margin to (chars):"),
						mHardWrapControl 
							= new BmCheckControl( 
								"Hard-wrap mailtext at right margin", 
								new BMessage(BM_HARD_WRAP_CHANGED), 
								this, 
								ThePrefs->GetBool("HardWrapMailText")
						),
						mHardWrapAt78Control 
							= new BmCheckControl( 
								"Always respect maximum line-length of 78 characters", 
							   new BMessage(BM_HARD_WRAP_AT_78_CHANGED), 
							   this, 
							   ThePrefs->GetBool( "NeverExceed78Chars", false)
							),
						new Space( minimax(0,10,0,10)),
						mQuotingStringControl 
							= new BmTextControl( "Quoting string:"),
						mQuoteFormattingControl 
							= new BmMenuControl( "Quote-formatting:", 
														new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mUndoModeControl 
							= new BmMenuControl( "Undo-Granularity:", 
														new BPopUpMenu("")),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, 
								 (char*)"Mail-Construction Options",
					new VGroup(
						mAllow8BitControl = new BmCheckControl( 
							"Use 8-bit-MIME ", 
							new BMessage(BM_ALLOW_8_BIT_CHANGED), 
							this, ThePrefs->GetBool("Allow8BitMime")
						),
						mImportExportUtf8Control = new BmCheckControl( 
							"Treat all text-attachments as UTF-8.", 
							new BMessage(BM_IMPORT_EXPORT_UTF8_CHANGED), 
							this, ThePrefs->GetBool("ImportExportTextAsUtf8", true)
						),
						mSpecialForEachBccControl = new BmCheckControl( 
							"Generate header for each Bcc-recipient", 
						   new BMessage(BM_EACH_BCC_CHANGED), 
						   this, ThePrefs->GetBool("SpecialHeaderForEachBcc")
						),
						mPreferUserAgentControl = new BmCheckControl( 
							"Prefer 'UserAgent'-header over 'X-Mailer'", 
							new BMessage(BM_PREFER_USER_AGENT_CHANGED), 
							this, ThePrefs->GetBool("PreferUserAgentOverX-Mailer")
						),
						mGenerateIDsControl = new BmCheckControl( 
							"Generate own message-IDs", 
							new BMessage(BM_GENERATE_MSGIDS_CHANGED), 
							this, ThePrefs->GetBool("GenerateOwnMessageIDs")
						),
						mMakeQpSafeControl = new BmCheckControl( 
							"Make quoted-printable safe for non-ASCII gateways "
							"(EBCDIC)", 
							new BMessage(BM_QP_SAFE_CHANGED), 
							this, ThePrefs->GetBool("MakeQPSafeForEBCDIC")
						),
						0
					)
				),
				0
			),
			new Space( minimax(0,5,0,5)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"People Options",
					new VGroup(
						mLookInPeopleFolderControl = new BmCheckControl( 
							"Look only in people-folder", 
				         new BMessage(BM_LOOK_IN_PEOPLE_CHANGED), 
				         this, 
				         ThePrefs->GetBool( "LookForPeopleOnlyInPeopleFolder", 
				         						 true)
				      ),
						new Space( minimax(0,5,0,5)),
						mAddNameToPeopleControl = new BmCheckControl( 
							"Add People's Names to Mail-Address", 
				         new BMessage(BM_ADD_PEOPLE_NAME_CHANGED), 
					      this, ThePrefs->GetBool("AddPeopleNameToMailAddr",true)
					   ),
						new Space( minimax(0,5,0,5)),
						mPeopleFolderButton = new MButton( 
							PeopleFolderButtonLabel().String(), 
							new BMessage( BM_SELECT_PEOPLE_FOLDER), 
							this,minimax(-1,-1,-1,-1)
						),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Character-Sets",
					new VGroup(
						mDefaultCharsetControl = new BmMenuControl( 
							"Default-charset:", 
							new BmMenuController( "", this, 
														 new BMessage( BM_CHARSET_SELECTED), 
														 BmRebuildCharsetMenu,
														 BM_MC_LABEL_FROM_MARKED
							)
						),
						mUsedCharsetsControl = new BmMenuControl( 
							"Frequently used charsets:", 
							new BPopUpMenu("")),
						0
					)
				),
				0
			),
			new Space( minimax(0,5,0,5)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Forwarding",
					new VGroup(
						mForwardIntroStrControl 
							= new BmTextControl( "Intro:", false, 0, 23),
						mForwardSubjectStrControl = new BmTextControl( "Subject:"),
						mForwardSubjectRxControl 
							= new BmTextControl( "Is-Forward-Regex:"),
						new HGroup( 
							mDefaultForwardTypeControl 
								= new BmMenuControl( "Default forward-type:", 
															new BPopUpMenu("")),
							new Space(),
							0
						),
						new Space( minimax(0,4,0,4)),
						mDontAttachVCardsControl = new BmCheckControl( 
								"Do not attach v-cards in forward", 
								new BMessage(BM_ATTACH_VCARDS_CHANGED), 
								this, ThePrefs->GetBool("DoNotAttachVCardsToForward")
						),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Replying",
					new VGroup(
						mReplyIntroStrControl = new BmTextControl( "Intro:"),
						mReplySubjectStrControl = new BmTextControl( "Subject:"),
						mReplySubjectRxControl 
							= new BmTextControl( "Is-Reply-Regex:"),
						mReplyIntroStrPrivateControl 
							= new BmTextControl( "Personal Phrase:"),
						new Space(),
						0
					)
				),
				0
			),
			new Space( minimax(0,5,0,5)),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mMaxLineLenControl->Divider();
	divider = MAX( divider, mQuotingStringControl->Divider());
	divider = MAX( divider, mQuoteFormattingControl->Divider());
	divider = MAX( divider, mDefaultCharsetControl->Divider());
	divider = MAX( divider, mUsedCharsetsControl->Divider());
	divider = MAX( divider, mForwardIntroStrControl->Divider());
	divider = MAX( divider, mForwardSubjectStrControl->Divider());
	divider = MAX( divider, mForwardSubjectRxControl->Divider());
	divider = MAX( divider, mDefaultForwardTypeControl->Divider());
	divider = MAX( divider, mReplyIntroStrControl->Divider());
	divider = MAX( divider, mReplyIntroStrPrivateControl->Divider());
	divider = MAX( divider, mReplySubjectStrControl->Divider());
	divider = MAX( divider, mReplySubjectRxControl->Divider());
	divider = MAX( divider, mUndoModeControl->Divider());
	mMaxLineLenControl->SetDivider( divider);
	mQuotingStringControl->SetDivider( divider);
	mQuoteFormattingControl->SetDivider( divider);
	mDefaultCharsetControl->SetDivider( divider);
	mUsedCharsetsControl->SetDivider( divider);
	mForwardIntroStrControl->SetDivider( divider);
	mForwardSubjectStrControl->SetDivider( divider);
	mForwardSubjectRxControl->SetDivider( divider);
	mDefaultForwardTypeControl->SetDivider( divider);
	mReplyIntroStrControl->SetDivider( divider);
	mReplyIntroStrPrivateControl->SetDivider( divider);
	mReplySubjectStrControl->SetDivider( divider);
	mReplySubjectRxControl->SetDivider( divider);
	mUndoModeControl->SetDivider( divider);

	BmString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetText( val.String());
	mQuotingStringControl->SetText( 
		ThePrefs->GetString("QuotingString").String());
	mForwardIntroStrControl->SetText( 
		ThePrefs->GetString("ForwardIntroStr").String());
	mForwardSubjectStrControl->SetText( 
		ThePrefs->GetString("ForwardSubjectStr").String());
	mForwardSubjectRxControl->SetText( 
		ThePrefs->GetString("ForwardSubjectRX").String());
	mReplyIntroStrControl->SetText( 
		ThePrefs->GetString("ReplyIntroStr").String());
	mReplyIntroStrPrivateControl->SetText( 
		ThePrefs->GetString("ReplyIntroDefaultNick", "you").String());
	mReplySubjectStrControl->SetText( 
		ThePrefs->GetString("ReplySubjectStr").String());
	mReplySubjectRxControl->SetText( 
		ThePrefs->GetString("ReplySubjectRX").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailConstrView::~BmPrefsMailConstrView() {
	delete mPeoplePanel;
	TheBubbleHelper->SetHelp( mMaxLineLenControl, NULL);
	TheBubbleHelper->SetHelp( mQuotingStringControl, NULL);
	TheBubbleHelper->SetHelp( mQuoteFormattingControl, NULL);
	TheBubbleHelper->SetHelp( mForwardIntroStrControl, NULL);
	TheBubbleHelper->SetHelp( mForwardSubjectStrControl, NULL);
	TheBubbleHelper->SetHelp( mForwardSubjectRxControl, NULL);
	TheBubbleHelper->SetHelp( mReplyIntroStrControl, NULL);
	TheBubbleHelper->SetHelp( mReplyIntroStrPrivateControl, NULL);
	TheBubbleHelper->SetHelp( mReplySubjectStrControl, NULL);
	TheBubbleHelper->SetHelp( mReplySubjectRxControl, NULL);
	TheBubbleHelper->SetHelp( mDefaultCharsetControl, NULL);
	TheBubbleHelper->SetHelp( mUsedCharsetsControl, NULL);
	TheBubbleHelper->SetHelp( mSpecialForEachBccControl, NULL);
	TheBubbleHelper->SetHelp( mPreferUserAgentControl, NULL);
	TheBubbleHelper->SetHelp( mGenerateIDsControl, NULL);
	TheBubbleHelper->SetHelp( mMakeQpSafeControl, NULL);
	TheBubbleHelper->SetHelp( mDefaultForwardTypeControl, NULL);
	TheBubbleHelper->SetHelp( mDontAttachVCardsControl, NULL);
	TheBubbleHelper->SetHelp( mHardWrapControl, NULL);
	TheBubbleHelper->SetHelp( mHardWrapAt78Control, NULL);
	TheBubbleHelper->SetHelp( mAllow8BitControl, NULL);
	TheBubbleHelper->SetHelp( mImportExportUtf8Control, NULL);
	TheBubbleHelper->SetHelp( mLookInPeopleFolderControl, NULL);
	TheBubbleHelper->SetHelp( mAddNameToPeopleControl, NULL);
	TheBubbleHelper->SetHelp( mPeopleFolderButton, NULL);
	TheBubbleHelper->SetHelp( mUndoModeControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsMailConstrView::PeopleFolderButtonLabel() {
	BmString label( "   Set People Folder (currently '");
	label << ThePrefs->GetString( "PeopleFolder", 
											"/boot/home/people") 
			<< "')...   ";
	return label;
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

	TheBubbleHelper->SetHelp( 
		mMaxLineLenControl, 
		"Here you can enter the maximum number of characters\n"
		"per line Beam should allow in the mailtext.\n"
		"This corresponds to the right margin in the mail-editor."
	);
	TheBubbleHelper->SetHelp( 
		mQuotingStringControl, 
		"Here you can enter the string used for quoting.\n"
		"This string will be prepended to every quoted line."
	);
	TheBubbleHelper->SetHelp( 
		mQuoteFormattingControl, 
		"This menu controls the way Beam formats quoted lines\n"
		"of a reply/forward.\n\n"
		"Simple:\n"
		"	Beam will simply prepend the quote-string to every line.\n"
		"	Lines that exceed the maximum line length will be wrapped,\n"
		"	resulting in a very short line. This sooner or later causes\n"
		"	ugly mails with a comb-like layout (alternating long and\n"
		"	short lines).\n"
		"Push Margin:\n"
		"	In this mode, Beam will push the right margin of a reply/forward\n"
		"	just as needed. The original formatting of the mail stays intact,\n"
		"	but the mail may exceed 80 chars per line (which will annoy\n"
		"	recipients using terminal-based mailers).\n"
		"Auto Wrap:\n"
		"	This will cause Beam to automatically rewrap blocks of quoted \n"
		"	lines in a way that tries to keep the paragraph structure of the\n"
		"	original mail intact.\n\n"
		"'Auto Wrap' usually gives the best results for normal text,\n"
		"but since it can lead to unwanted wrapping of structured text\n"
		"(e.g. code), Beam uses 'Push Margin' by default."
	);
	TheBubbleHelper->SetHelp( 
		mForwardIntroStrControl, 
		"Here you can enter a string that will\n"
		"appear at the top of every forwarded mail.\n"
		"The following macros are supported:\n"
		"	%d  -  expands to the original mail's date.\n"
		"	%f  -  expands to the sender of the original mail.\n"
		"	%s  -  expands to the original mail's subject.\n"
		"	%t  -  expands to the original mail's time."
	);
	TheBubbleHelper->SetHelp( 
		mForwardSubjectStrControl, 
		"Here you can influence the subject-string \n"
		"Beam generates for a forwarded mail.\n"
		"The following macros are supported:\n"
		"	%s  -  expands to the original mail's subject."
	);
	TheBubbleHelper->SetHelp( 
		mForwardSubjectRxControl, 
		"This string is the regular-expression (perl-style) Beam uses\n"
		"to determine whether a given subject indicates\n"
		"that the mail already is a forward.\n"
		"This way subjects like \n\t'Fwd: Fwd: Fwd: fun-stuff'\n"
		"can be avoided."
	);
	TheBubbleHelper->SetHelp( 
		mReplyIntroStrControl, 
		"Here you can enter a string that will \n"
		"appear at the top of every reply.\n"
		"The following macros are supported:\n"
		"	%d  -  expands to the original mail's date.\n"
		"	%f  -  expands to the sender of the original mail.\n"
		"	%s  -  expands to the original mail's subject.\n"
		"	%t  -  expands to the original mail's time."
	);
	TheBubbleHelper->SetHelp( 
		mReplyIntroStrPrivateControl, 
		"When replying to a person only, it seems inappropriate\n"
		"to have: 'On ... Steve Miller wrote:' in the intro text.\n"
		"In this field you can enter a text that will be used\n"
		"instead of the name when expanding %f in order to\n"
		"achieve something like: 'On ... you wrote'."
	);
	TheBubbleHelper->SetHelp( 
		mReplySubjectStrControl, 
		"Here you can influence the subject-string \n"
		"Beam generates for a reply.\n"
		"The following macros are supported:\n"
		"	%s  -  expands to the original mail's subject."
	);
	TheBubbleHelper->SetHelp( 
		mReplySubjectRxControl, 
		"This string is the regular-expression (perl-style) Beam uses\n"
		"to determine whether a given subject indicates\n"
		"that the mail already is a reply.\n"
		"This way subjects like \n\t'Re: Re: Re: your offer'\n"
		"can be avoided."
	);
	TheBubbleHelper->SetHelp( 
		mDefaultCharsetControl, 
		"Here you can select the charset-encoding Beam should use by default."
	);
	TheBubbleHelper->SetHelp( 
		mUsedCharsetsControl, 
		"Here you can select the charsets you are frequently using.\n"
		"These will then be shown in the first level of every charset-menu."
	);
	TheBubbleHelper->SetHelp( 
		mSpecialForEachBccControl, 
		"Here you can select the way Beam sends mails with Bcc recipients\n\n"
		"Checked:\n"
		"	Beam will send separate mails to each Bcc-recipient, each of which\n"
		"	will contain exactly one Bcc-header (the current recipient). \n"
		"	This results in more traffic, but has the advantage that each mail\n"
		"	actually contains the recipients address in the header, making it \n"
		"	look less like spam.\n"
		"Unchecked:\n"
		"	Beam will send a single mail to all recipients, which does not\n"
		"	contain any Bcc-headers. \n"
		"	This results in less network traffic but makes it more likely that\n"
		"	the mail is filtered right into the spam-folder on the\n"
		"	recipient's side."
	);
	TheBubbleHelper->SetHelp( 
		mPreferUserAgentControl, 
		"Email-clients used to identify themselves in a header-field called\n"
		"'X-Mailer'.\n"
		"Lately, the use of a header-field named 'UserAgent' became popular.\n"
		"By checking/unchecking this control you can decide which field\n"
		"Beam should use."
	);
	TheBubbleHelper->SetHelp( 
		mGenerateIDsControl, 
		"This control determines if Beam should generate the message-IDs\n"
		"used to uniquely identify every mail on the net.\n"
		"If unchecked, the SMTP-server will generate these IDs, which is \n"
		"usually ok, but makes sorting mails by thread less reliable\n"
		"(since mail-threading is currently not implemented this doesn't\n"
		"apply for now)."
	);
	TheBubbleHelper->SetHelp( 
		mMakeQpSafeControl, 
		"This makes Beam generate quoted-printables that are safe for\n"
		"EBCDIC-gateways\n (if you don't know what EBCDIC is, don't worry\n"
		"and leave this as is)."
	);
	TheBubbleHelper->SetHelp( 
		mDefaultForwardTypeControl, 
		"Here you can select the forwarding-type Beam should use when you\n"
		"press the 'Forward'-button."
	);
	TheBubbleHelper->SetHelp( 
		mDontAttachVCardsControl, 
		"Checking this causes Beam to NOT include vcard-attachments\n"
		"(appended address-info) in a forwarded mail."
	);
	TheBubbleHelper->SetHelp( 
		mHardWrapControl, 
		"Checking this causes Beam to hard-wrap the mailtext at the given\n"
		"right margin. This means that the mail will be sent exactly as you \n"
		"see it on screen, so that every other mail-program will be able to\n"
		"correctly display this mail.\n"
		"Uncheck this if you want soft-wrapped paragraphs, that will be \n"
		"layouted by the receiving mail-client.\n"
		"Please note that this only concerns the mailtext entered by yourself,\n"
		"quoted text will *always* be hard-wrapped.\n\n"
		"Hint: Use soft-wrap only if you know that the receiving mail-program\n"
		"is able to handle long lines nicely (most modern mailers do, but some\n"
		"mailing-list software does not)."
	);
	TheBubbleHelper->SetHelp( 
		mHardWrapAt78Control, 
		"Checking this causes Beam to ensure that no single line of a mail\n"
		"exceeds the length of 78 characters (as suggested by RFC2822).\n"
		"This will fix the right margin to at most 78 chars."
	);
	TheBubbleHelper->SetHelp( 
		mAllow8BitControl, 
		"Checking this causes Beam to allow 8-bit characters\n"
		"inside a mail-body without encoding them.\n"
		"This avoids the use of quoted-printables and is usually ok with \n"
		"modern mail-servers, but it *may* cause problems during transport,\n"
		"so if you get complaints about strange/missing characters,\n"
		"try unchecking this."
	);
	TheBubbleHelper->SetHelp( 
		mImportExportUtf8Control, 
		"If this is checked, every text-attachment that you add to a mail\n"
		"will be treated as being utf-8 text."
		"If unchecked, each text-attachment added to a mail will be added\n"
		"with the encoding currently selected in the mail-edit-window.\n"
		"This is useful if you need to add text-files to mails that\n"
		"have non-utf8 character-sets.\n"
		"If you are in doubt, leave checked.");
	TheBubbleHelper->SetHelp( 
		mLookInPeopleFolderControl, 
		"If checked, Beam will only deal with people-files that\n"
		"live in the people-folder.\n"
		"If unchecked, Beam will accept people-files outside of this\n"
		"folder, too."
	);
	TheBubbleHelper->SetHelp( 
		mAddNameToPeopleControl, 
		"If checked, Beam will not only use people's mail-addresses when\n"
		"sending a mail to them, but the people's names will be added, too.\n"
		"Assuming name 'Polly Jean Harvey' with address 'pjharvey@test.org',\n"
		"you get:\n"
		"	Polly Jean Harvey <pjharvey@test.org>, if checked\n"
		"	pjharvey@test.org, if unchecked."
	);
	TheBubbleHelper->SetHelp( 
		mPeopleFolderButton, 
		"Click this button in order to\nselect a different people-folder."
	);
	TheBubbleHelper->SetHelp( 
		mUndoModeControl, 
		"Here you can select the granularity of the\n"
		"undo- & redo-operations in the mail-editor:\n"
		"	 'Words' - means that single words will be undone/redone.\n"
		"	 'Paragraphs' - means that whole paragraphs will be undone/redone.\n"
		"	 'None' - means that different kinds of editing operations will be\n"
		"	          undone/redone (BeOS-standard)."
	);

	// setup menu for frequently used charsets:
	SetupUsedCharsetsMenu();

	// add quote-formattings:
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_AUTO_WRAP, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_PUSH_MARGIN, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_SIMPLE, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);

	// add forward-types:
	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Attached", 
											new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);
	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Inline", 
											new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);

	// add undo-modes:
	AddItemToMenu( mUndoModeControl->Menu(), 
						new BMenuItem( "Words", new BMessage(BM_UNDO_MODE_SELECTED)), 
						this);
	AddItemToMenu( mUndoModeControl->Menu(), 
						new BMenuItem( "Paragraphs", 
											new BMessage(BM_UNDO_MODE_SELECTED)), 
						this);
	AddItemToMenu( mUndoModeControl->Menu(), 
						new BMenuItem( "None", new BMessage(BM_UNDO_MODE_SELECTED)), 
						this);

	Update();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::SetupUsedCharsetsMenu() {
	BmString charset;
	BMenu* menu = mUsedCharsetsControl->Menu();
	if (!menu)
		return;
	BMenuItem* item;
	while( (item = menu->RemoveItem( (int32)0))!=NULL)
		delete item;
	menu->SetRadioMode( false);
	item = mUsedCharsetsControl->MenuItem();
	if (!item)
		return;
	item->SetLabel("<please select...>");
	menu->SetLabelFromMarked( false);
	// add all charsets and check the ones that are selected:
	BmString stdSets = ThePrefs->GetString( "StandardCharsets");
	BmCharsetMap::const_iterator iter;
	for( iter = TheCharsetMap.begin(); iter != TheCharsetMap.end(); ++iter) {
		if (iter->second) {
			charset = iter->first;
			charset.ToLower();
			BMessage* msg = new BMessage( BM_USED_CHARSET_SELECTED);
			msg->AddString( MSG_CHARSET, charset.String());
			item = CreateMenuItem( charset.String(), msg);
			AddItemToMenu( menu, item, this);
			if (stdSets.IFindFirst( BmString("<")<<charset<<">") != B_ERROR)
				item->SetMarked( true);
		}
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::SetupUsedCharsetsPrefs() {
	BmString stdSets;
	BMenuItem* item;
	BMenu* menu = mUsedCharsetsControl->Menu();
	if (!menu)
		return;
	int32 count = menu->CountItems();
	for( int32 i=0; i < count; ++i) {
		item = menu->ItemAt( i);
		if (item->IsMarked())
			stdSets << "<" << item->Label() << ">";
	}
	ThePrefs->SetString( "StandardCharsets", stdSets);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::Update() {
	mHardWrapControl->SetValueSilently( ThePrefs->GetBool("HardWrapMailText"));
	mHardWrapAt78Control->SetEnabled( mHardWrapControl->Value());
	mHardWrapAt78Control->SetValueSilently( 
		ThePrefs->GetBool( "NeverExceed78Chars", false)
	);
	mQuoteFormattingControl->MarkItem( 
		ThePrefs->GetString( "QuoteFormatting", 
									BmMail::BM_QUOTE_AUTO_WRAP).String()
	);
	mDefaultForwardTypeControl->MarkItem( 
		ThePrefs->GetString( "DefaultForwardType").String()
	);
	mDefaultCharsetControl->ClearMark();
	mUndoModeControl->MarkItem( 
		ThePrefs->GetString("UndoMode", "Words").String()
	);
	BmString charset( ThePrefs->GetString( "DefaultCharset"));
	mDefaultCharsetControl->MarkItem( charset.String());
	mDefaultCharsetControl->MenuItem()->SetLabel( charset.String());
	mAllow8BitControl->SetValueSilently( ThePrefs->GetBool("Allow8BitMime"));
	mImportExportUtf8Control->SetValueSilently( 
		ThePrefs->GetBool("ImportExportTextAsUtf8", true)
	);
	mSpecialForEachBccControl->SetValueSilently( 
		ThePrefs->GetBool("SpecialHeaderForEachBcc")
	);
	mPreferUserAgentControl->SetValueSilently( 
		ThePrefs->GetBool("PreferUserAgentOverX-Mailer")
	);
	mGenerateIDsControl->SetValueSilently( 
		ThePrefs->GetBool("GenerateOwnMessageIDs")
	);
	mMakeQpSafeControl->SetValueSilently( 
		ThePrefs->GetBool("MakeQPSafeForEBCDIC")
	);
	mDontAttachVCardsControl->SetValueSilently(
		ThePrefs->GetBool("DoNotAttachVCardsToForward")
	);
	mAddNameToPeopleControl->SetValueSilently( 
		ThePrefs->GetBool("AddPeopleNameToMailAddr", true)
	);
	mLookInPeopleFolderControl->SetValueSilently( 
		ThePrefs->GetBool("LookForPeopleOnlyInPeopleFolder", true)
	);
	BmString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetTextSilently( val.String());
	mQuotingStringControl->SetTextSilently( 
		ThePrefs->GetString("QuotingString").String()
	);
	mForwardIntroStrControl->SetTextSilently( 
		ThePrefs->GetString("ForwardIntroStr").String()
	);
	mForwardSubjectStrControl->SetTextSilently( 
		ThePrefs->GetString("ForwardSubjectStr").String()
	);
	mForwardSubjectRxControl->SetTextSilently( 
		ThePrefs->GetString("ForwardSubjectRX").String()
	);
	mReplyIntroStrControl->SetTextSilently( 
		ThePrefs->GetString("ReplyIntroStr").String()
	);
	mReplyIntroStrPrivateControl->SetTextSilently( 
		ThePrefs->GetString("ReplyIntroDefaultNick", "you").String()
	);
	mReplySubjectStrControl->SetTextSilently( 
		ThePrefs->GetString("ReplySubjectStr").String()
	);
	mReplySubjectRxControl->SetTextSilently( 
		ThePrefs->GetString("ReplySubjectRX").String()
	);
	SetupUsedCharsetsMenu();
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
					ThePrefs->SetInt( "MaxLineLen", 
											atoi(mMaxLineLenControl->Text()));
				else if ( source == mQuotingStringControl)
					ThePrefs->SetString( "QuotingString", 
												mQuotingStringControl->Text());
				else if ( source == mForwardIntroStrControl)
					ThePrefs->SetString( "ForwardIntroStr", 
												mForwardIntroStrControl->Text());
				else if ( source == mForwardSubjectStrControl)
					ThePrefs->SetString( "ForwardSubjectStr", 
												mForwardSubjectStrControl->Text());
				else if ( source == mForwardSubjectRxControl)
					ThePrefs->SetString( "ForwardSubjectRX", 
												mForwardSubjectRxControl->Text());
				else if ( source == mReplyIntroStrControl)
					ThePrefs->SetString( "ReplyIntroStr", 
												mReplyIntroStrControl->Text());
				else if ( source == mReplyIntroStrPrivateControl)
					ThePrefs->SetString( "ReplyIntroDefaultNick", 
												mReplyIntroStrPrivateControl->Text());
				else if ( source == mReplySubjectStrControl)
					ThePrefs->SetString( "ReplySubjectStr", 
												mReplySubjectStrControl->Text());
				else if ( source == mReplySubjectRxControl)
					ThePrefs->SetString( "ReplySubjectRX", 
												mReplySubjectRxControl->Text());
				NoticeChange();
				break;
			}
			case BM_EACH_BCC_CHANGED: {
				ThePrefs->SetBool( "SpecialHeaderForEachBcc",
										 mSpecialForEachBccControl->Value());
				NoticeChange();
				break;
			}
			case BM_PREFER_USER_AGENT_CHANGED: {
				ThePrefs->SetBool( "PreferUserAgentOverX-Mailer", 
										 mPreferUserAgentControl->Value());
				NoticeChange();
				break;
			}
			case BM_GENERATE_MSGIDS_CHANGED: {
				ThePrefs->SetBool( "GenerateOwnMessageIDs", 
										 mGenerateIDsControl->Value());
				NoticeChange();
				break;
			}
			case BM_QP_SAFE_CHANGED: {
				ThePrefs->SetBool( "MakeQPSafeForEBCDIC", 
										 mMakeQpSafeControl->Value());
				NoticeChange();
				break;
			}
			case BM_ATTACH_VCARDS_CHANGED: {
				ThePrefs->SetBool( "DoNotAttachVCardsToForward", 
										 mDontAttachVCardsControl->Value());
				NoticeChange();
				break;
			}
			case BM_ALLOW_8_BIT_CHANGED: {
				ThePrefs->SetBool( "Allow8BitMime", mAllow8BitControl->Value());
				NoticeChange();
				break;
			}
			case BM_IMPORT_EXPORT_UTF8_CHANGED: {
				ThePrefs->SetBool( "ImportExportTextAsUtf8", 
										 mImportExportUtf8Control->Value());
				NoticeChange();
				break;
			}
			case BM_HARD_WRAP_AT_78_CHANGED: {
				ThePrefs->SetBool( "NeverExceed78Chars", 
										 mHardWrapAt78Control->Value());
				NoticeChange();
				break;
			}
			case BM_LOOK_IN_PEOPLE_CHANGED: {
				ThePrefs->SetBool( "LookForPeopleOnlyInPeopleFolder", 
										 mLookInPeopleFolderControl->Value());
				NoticeChange();
				break;
			}
			case BM_ADD_PEOPLE_NAME_CHANGED: {
				ThePrefs->SetBool( "AddPeopleNameToMailAddr", 
										 mAddNameToPeopleControl->Value());
				NoticeChange();
				break;
			}
			case BM_HARD_WRAP_CHANGED: {
				bool val = mHardWrapControl->Value();
				ThePrefs->SetBool("HardWrapMailText", val);
				mHardWrapAt78Control->SetEnabled( val);
				if (!val)
					mHardWrapAt78Control->SetValue( false);
				NoticeChange();
				break;
			}
			case BM_CHARSET_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					mDefaultCharsetControl->ClearMark();
					mDefaultCharsetControl->MenuItem()->SetLabel( item->Label());
					ThePrefs->SetString( "DefaultCharset", item->Label());
					item->SetMarked( true);
					NoticeChange();
				}
				break;
			}
			case BM_USED_CHARSET_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					item->SetMarked( !item->IsMarked());
					SetupUsedCharsetsPrefs();
					NoticeChange();
				}
				break;
			}
			case BM_UNDO_MODE_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					ThePrefs->SetString( "UndoMode", item->Label());
					NoticeChange();
				}
				break;
			}
			case BM_QUOTE_FORMATTING_SELECTED: {
				BMenuItem* item = mQuoteFormattingControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "QuoteFormatting", item->Label());
				NoticeChange();
				break;
			}
			case BM_FORWARD_TYPE_SELECTED: {
				BMenuItem* item = mDefaultForwardTypeControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "DefaultForwardType", item->Label());
				NoticeChange();
				break;
			}
			case BM_SELECT_PEOPLE_FOLDER: {
				entry_ref folderRef;
				if (msg->FindRef( "refs", 0, &folderRef) != B_OK) {
					// first step, let user select new folder:
					if (!mPeoplePanel) {
						mPeoplePanel = new BFilePanel( B_OPEN_PANEL, 
																 new BMessenger(this), NULL,
																 B_DIRECTORY_NODE, false, msg);
					}
					mPeoplePanel->Show();
				} else {
					// second step, set people-folder accordingly:
					BEntry entry( &folderRef);
					BPath path;
					if (entry.GetPath( &path) == B_OK) {
						ThePrefs->SetString( "PeopleFolder", path.Path());
						mPeopleFolderButton->SetLabel( 
							PeopleFolderButtonLabel().String());
						NoticeChange();
						BAlert* alert = new BAlert( 
								"People Folder", 
								"Done, Beam will use the new people-folder after a "
									"restart",
								"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
								B_INFO_ALERT
						);
						alert->Go();
					}
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

