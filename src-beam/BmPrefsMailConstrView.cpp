/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <FilePanel.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <LayeredGroup.h>
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "split.hh"
using namespace regexx;

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFactory.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmPrefsMailConstrView.h"
#include "BmRosterBase.h"
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
						new Space(),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Character-Sets",
						new VGroup(
							mDefaultCharsetControl = new BmMenuControl( 
								"Default-charset:", 
								new BmMenuController( "", this, 
															 new BMessage( BM_CHARSET_SELECTED), 
															 &BmGuiRosterBase::RebuildCharsetMenu,
															 BM_MC_LABEL_FROM_MARKED
								)
							),
							mUsedCharsetsControl = new BmMenuControl( 
								"Frequently used charsets:", 
								new BPopUpMenu("")),
							new Space(),
							0
						)
					),
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
							new HGroup(
								mPeopleFolderButton = new MButton( 
									PeopleFolderButtonLabel().String(), 
									new BMessage( BM_SELECT_PEOPLE_FOLDER), 
									this,minimax(-1,-1,-1,-1)
								),
								new Space(),
								0
							),
							0
						)
					),
					0
				),
				0
			),
			new Space( minimax(0,5,0,5)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Replying",
					new VGroup(
						mReplyIntroStrControl = new BmTextControl( "Intro:"),
						mReplySubjectStrControl = new BmTextControl( "Subject:"),
						mReplyIntroStrPrivateControl 
							= new BmTextControl( "Personal Phrase:"),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Forwarding",
					new VGroup(
						mForwardIntroStrControl 
							= new BmTextControl( "Intro:", false, 0, 23),
						mForwardSubjectStrControl = new BmTextControl( "Subject:"),
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
	
	BmDividable::DivideSame(
		mMaxLineLenControl,
		mQuotingStringControl, 
		mQuoteFormattingControl,
		mDefaultCharsetControl,
		mUsedCharsetsControl,
		mForwardIntroStrControl,
		mForwardSubjectStrControl,
		mReplyIntroStrControl,
		mReplyIntroStrPrivateControl,
		mReplySubjectStrControl,
		mUndoModeControl,
		NULL
	);

	BmString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetText( val.String());
	mQuotingStringControl->SetText( 
		ThePrefs->GetString("QuotingString").String());
	mForwardIntroStrControl->SetText( 
		ThePrefs->GetString("ForwardIntroStr").String());
	mForwardSubjectStrControl->SetText( 
		ThePrefs->GetString("ForwardSubjectStr").String());
	mReplyIntroStrControl->SetText( 
		ThePrefs->GetString("ReplyIntroStr").String());
	mReplyIntroStrPrivateControl->SetText( 
		ThePrefs->GetString("ReplyIntroDefaultNick", "you").String());
	mReplySubjectStrControl->SetText( 
		ThePrefs->GetString("ReplySubjectStr").String());
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
	TheBubbleHelper->SetHelp( mReplyIntroStrControl, NULL);
	TheBubbleHelper->SetHelp( mReplyIntroStrPrivateControl, NULL);
	TheBubbleHelper->SetHelp( mReplySubjectStrControl, NULL);
	TheBubbleHelper->SetHelp( mDefaultCharsetControl, NULL);
	TheBubbleHelper->SetHelp( mUsedCharsetsControl, NULL);
	TheBubbleHelper->SetHelp( mHardWrapControl, NULL);
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
	mReplyIntroStrControl->SetTarget( this);
	mReplyIntroStrPrivateControl->SetTarget( this);
	mReplySubjectStrControl->SetTarget( this);

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
		"The following macros (case insensitive) are supported:\n"
		"	%d  -  expands to the date.\n"
		"	%f  -  expands to the sender.\n"
		"	%fa -  expands to the sender's address.\n"
		"	%fn -  expands to the sender's name.\n"
		"	%s  -  expands to the subject.\n"
		"	%t  -  expands to the time."
	);
	TheBubbleHelper->SetHelp( 
		mForwardSubjectStrControl, 
		"Here you can influence the subject-string \n"
		"Beam generates for a forwarded mail.\n"
		"The following macros are supported:\n"
		"	%s  -  expands to the original mail's subject."
	);
	TheBubbleHelper->SetHelp( 
		mReplyIntroStrControl, 
		"Here you can enter a string that will \n"
		"appear at the top of every reply.\n"
		"The following macros (case insensitive) are supported:\n"
		"	%d  -  expands to the date.\n"
		"	%f  -  expands to the sender.\n"
		"	%fa -  expands to the sender's address.\n"
		"	%fn -  expands to the sender's name.\n"
		"	%s  -  expands to the subject.\n"
		"	%t  -  expands to the time."
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
		mDefaultCharsetControl, 
		"Here you can select the charset-encoding Beam should use by default."
	);
	TheBubbleHelper->SetHelp( 
		mUsedCharsetsControl, 
		"Here you can select the charsets you are frequently using.\n"
		"These will then be shown in the first level of every charset-menu."
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
						new BMenuItem( BmMailFactory::BM_QUOTE_AUTO_WRAP, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMailFactory::BM_QUOTE_PUSH_MARGIN, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMailFactory::BM_QUOTE_SIMPLE, 
											new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
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
	BmString stdSetStr = ThePrefs->GetString( "StandardCharsets");
	vector<BmString> stdSets;
	split( BmPrefs::nListSeparator, stdSetStr, stdSets);
	BmCharsetMap::const_iterator iter;
	for( iter = TheCharsetMap.begin(); iter != TheCharsetMap.end(); ++iter) {
		if (iter->second) {
			charset = iter->first;
			charset.ToLower();
			BMessage* msg = new BMessage( BM_USED_CHARSET_SELECTED);
			item = CreateMenuItem( charset.String(), msg);
			AddItemToMenu( menu, item, this);
			for( uint32 i=0; i<stdSets.size(); ++i) {
				if (stdSets[i].ICompare(charset) == 0) {
					item->SetMarked( true);
					break;
				}
			}
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
		if (item->IsMarked()) {
			if (stdSets.Length())
				stdSets << BmPrefs::nListSeparator;
			stdSets << item->Label();
		}
	}
	ThePrefs->SetString( "StandardCharsets", stdSets);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::Update() {
	mHardWrapControl->SetValueSilently( ThePrefs->GetBool("HardWrapMailText"));
	mQuoteFormattingControl->MarkItem( 
		ThePrefs->GetString( "QuoteFormatting").String()
	);
	mDefaultCharsetControl->ClearMark();
	mUndoModeControl->MarkItem( 
		ThePrefs->GetString("UndoMode", "Words").String()
	);
	BmString charset( ThePrefs->GetString( "DefaultCharset"));
	mDefaultCharsetControl->MarkItem( charset.String());
	mDefaultCharsetControl->MenuItem()->SetLabel( charset.String());
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
	mReplyIntroStrControl->SetTextSilently( 
		ThePrefs->GetString("ReplyIntroStr").String()
	);
	mReplyIntroStrPrivateControl->SetTextSilently( 
		ThePrefs->GetString("ReplyIntroDefaultNick", "you").String()
	);
	mReplySubjectStrControl->SetTextSilently( 
		ThePrefs->GetString("ReplySubjectStr").String()
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
				else if ( source == mReplyIntroStrControl)
					ThePrefs->SetString( "ReplyIntroStr", 
												mReplyIntroStrControl->Text());
				else if ( source == mReplyIntroStrPrivateControl)
					ThePrefs->SetString( "ReplyIntroDefaultNick", 
												mReplyIntroStrPrivateControl->Text());
				else if ( source == mReplySubjectStrControl)
					ThePrefs->SetString( "ReplySubjectStr", 
												mReplySubjectStrControl->Text());
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

