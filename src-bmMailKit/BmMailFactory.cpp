/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <algorithm>
#include <map>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMailFactory.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"


/******************************************************************************/
// #pragma mark --- BmMailFactory ---
/******************************************************************************/

const char* const BmMailFactory::BM_QUOTE_AUTO_WRAP = 		"Auto Wrap";
const char* const BmMailFactory::BM_QUOTE_SIMPLE = 			"Simple";
const char* const BmMailFactory::BM_QUOTE_PUSH_MARGIN = 	"Push Margin";

/*------------------------------------------------------------------------------*\
	AddBaseMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFactory::AddBaseMail( BmMail* mail)
{
	if (mail)
		AddBaseMailRef( mail->MailRef());
}

/*------------------------------------------------------------------------------*\
	AddBaseMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFactory::AddBaseMailRef( BmMailRef* ref) 
{
	if (ref)
		mBaseRefVect.push_back( ref);
}

/*------------------------------------------------------------------------------*\
	AddPartsFromMail()
	-	
\*------------------------------------------------------------------------------*/
void BmMailFactory::CopyMailParts( BmRef<BmMail>& newMail, 
											  BmRef<BmMail>& oldMail,
											  bool withAttachments, bool isForward, 
											  const BmString& intro,
											  const BmString& selectedText)
{
	if (!newMail || !oldMail || !newMail->Body() || !oldMail->Body())
		return;
	BmBodyPartList* oldBody = oldMail->Body();
	BmBodyPartList* newBody = newMail->Body();
	// copy and quote text-body:
	BmRef<BmBodyPart> newTextBody( newBody->EditableTextBody());
	BmRef<BmBodyPart> textBody( oldBody->EditableTextBody());
	// copy info about charset from old into new mail:
	BmString charset = oldMail->DefaultCharset();
	BmString quotedText;
	int32 newLineLen = QuoteText( (selectedText.Length() || !textBody)
													? selectedText 
													: textBody->DecodedData(),
											quotedText,
				 							ThePrefs->GetString( "QuotingString"),
											ThePrefs->GetInt( "MaxLineLen"));
	if (newTextBody) {
		newBody->SetEditableText( newTextBody->DecodedData() + "\n" 
														+ intro + "\n"
														+ quotedText, 
													 charset);
	} else
		newBody->SetEditableText( intro + "\n" + quotedText, charset);

	newMail->BumpRightMargin( newLineLen);
	if (withAttachments && oldBody->HasAttachments()) {
		BmAutolockCheckGlobal lock( oldBody->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				oldBody->ModelNameNC() << ": Unable to get lock"
			);
		BmModelItemMap::const_iterator iter, end;
		if (oldBody->IsMultiPart()) {
			iter = oldBody->begin()->second->begin();
			end = oldBody->begin()->second->end();
		} else {
			iter = oldBody->begin();
			end = oldBody->end();
		}
		// copy all attachments (maybe except v-cards):
		bool doNotAttachVCards 
			= ThePrefs->GetBool( "DoNotAttachVCardsToForward");
		for( ; iter != end; ++iter) {
			BmBodyPart* bodyPart 
				= dynamic_cast< BmBodyPart*>( iter->second.Get());
			if (textBody != bodyPart) {
				if (doNotAttachVCards 
				&& bodyPart->MimeType().ICompare( "text/x-vcard") == 0)
					continue;
				BmBodyPart* copiedBody = new BmBodyPart( *bodyPart);
				newBody->AddItemToList( copiedBody);
			}
		}
	}
	newMail->AddBaseMailRef( oldMail->MailRef());
}

/*------------------------------------------------------------------------------*\
	ExpandIntroMacros()
		-	expands all macros within the given intro-string
\*------------------------------------------------------------------------------*/
enum AddrKind {
	AK_FULL = 0,
	AK_NAME,
	AK_ADDR
};

struct AddrCollector {
	AddrCollector( AddrKind kind) : kind(kind), count(0) {}
	void operator()( const BmAddress& addr) {
		if (count++ > 0)
			result << ", ";
		switch( kind) {
			case AK_NAME: {
				// use name if that exists, use addr-spec otherwise:
				result << (addr.HasPhrase() ? addr.Phrase() : addr.AddrSpec());
				break;
			}
			case AK_ADDR: {
				// use addr-spec only:
				result << addr.AddrSpec();
				break;
			}
			default: {
				// use full address:
				result << addr.AddrString();
			}
		}
	}
	AddrKind kind;
	int count;
	BmString result;
};

void BmMailFactory::ExpandIntroMacros( BmRef<BmMail> mail, BmString& intro, 
													bool usePersonalPhrase)
{
	BmMailRef* mailRef = mail->MailRef();
	Regexx rx;
	intro = rx.replace( intro, "%D", 
							  TimeToString( mailRef->When(), "%Y-%m-%d"),
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%T", 
							  TimeToString( mailRef->When(), "%X [%z]"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "\\n", "\n", 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%S", mailRef->Subject(), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);

	BmAddressList addrs = mail->Header()->DetermineOriginator( true);
	if (intro.IFindFirst( "%FN") >= B_OK) {
		BmString fromNames;
		if (!usePersonalPhrase) {
			// the more formal approach, we replace %F by the originator(s) 
			// name:
			AddrCollector collector( AK_NAME);
			fromNames = for_each( addrs.begin(), addrs.end(), collector).result;
		} else
			// less formal way, replace %F by ReplyIntroDefaultNick in order
			// to say something like "On xxx, you wrote":
			fromNames = ThePrefs->GetString( "ReplyIntroDefaultNick", "you");
		intro.IReplaceAll( "%FN", fromNames.String()); 
	} 
	if (intro.IFindFirst( "%FA") >= B_OK) {
		// replace %FA by the originator(s) addr-spec:
		BmString fromAddrs;
		AddrCollector collector( AK_ADDR);
		fromAddrs = for_each( addrs.begin(), addrs.end(), collector).result;
		intro.IReplaceAll( "%FA", fromAddrs.String()); 
	}
	if (intro.IFindFirst( "%F") >= B_OK) {
		// replace %F by the originator(s) full address:
		BmString fromAddrs;
		AddrCollector collector( AK_FULL);
		fromAddrs = for_each( addrs.begin(), addrs.end(), collector).result;
		intro.IReplaceAll( "%F", fromAddrs.String()); 
	}
}

/*------------------------------------------------------------------------------*\
	QuoteText()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMailFactory::QuoteText( const BmString& in, BmString& out, 
								 const BmString inQuoteString, int maxLineLen) 
{
	out = "";
	if (!in.Length())
		return maxLineLen;
	BmString quoteString;
	quoteString.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), 
												&inQuoteString);
	BmString qStyle = ThePrefs->GetString( "QuoteFormatting");
	if (qStyle == BM_QUOTE_AUTO_WRAP)
		return QuoteTextWithReWrap( in, out, quoteString, maxLineLen);
	BmString quote;
	BmString text;
	Regexx rx;
	rx.str( in);
	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	int modifiedMaxLen = maxLineLen;
	int maxTextLen;
	// cache a few preference values
	int32 spacesPerTab = ThePrefs->GetInt( "SpacesPerTab", 4);
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		BmString q(rx.match[i].atom[0]);
		quote.ConvertTabsToSpaces( spacesPerTab, &q);
		// limit the quote chars to a sane length (as otherwise we might
		// loop endlessly when trying to wrap the content lines)
		if (quote.Length() > maxLineLen / 2)
			quote.Truncate(maxLineLen / 2);
		if (qStyle == BM_QUOTE_SIMPLE) {
			// always respect maxLineLen, wrap when lines exceed right margin.
			// This results in a combing-effect when long lines are wrapped
			// around, producing a very short next line.
			maxTextLen 
				= MAX( 0, 
						 maxLineLen - quote.CountChars() - quoteString.CountChars());
		} else {
			// qStyle == BM_QUOTE_PUSH_MARGIN
			// push right margin for new quote-string, if needed, in effect 
			// leaving the mail-formatting intact more often (but possibly
			// exceeding 80 chars per line):
			maxTextLen = MAX( 0, maxLineLen - quote.CountChars());
		}
		text = rx.match[i].atom[1];
		int32 len = text.Length();
		// trim trailing spaces:
		while( len>0 && text[len-1]==' ')
			len--;
		text.Truncate( len);
		int32 newLen = AddQuotedText( text, out, quote, quoteString, maxTextLen);
		modifiedMaxLen = MAX( newLen, modifiedMaxLen);
	}
	// now remove trailing empty lines:
	BmString emptyLinesAtEndRX 
		= BmString("(?:") << "\\Q" << quoteString << "\\E" 
								<< "(" << "\\Q" << quote << "\\E" 
								<< ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", 
							Regexx::newline|Regexx::global|Regexx::noatom);
	return modifiedMaxLen;
}

/*------------------------------------------------------------------------------*\
	QuoteText()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMailFactory::QuoteTextWithReWrap( const BmString& in, BmString& out, 
											  BmString quoteString, int maxLineLen)
{
	out = "";
	if (!in.Length())
		return maxLineLen;
	Regexx rx;
	rx.str( in);

	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	BmString currQuote;
	BmString text;
	BmString line;
	BmString quote;
	Regexx rxl;
	int maxTextLen;
	int minLenForWrappedLine = ThePrefs->GetInt( "MinLenForWrappedLine", 50);
	bool lastWasSpecialLine = true;
	int32 lastLineLen = 0;
	// cache a few preference values
	int32 spacesPerTab = ThePrefs->GetInt( "SpacesPerTab", 4);
	BmString quotingLevelEmptyLineRX
		= ThePrefs->GetString( "QuotingLevelEmptyLineRX", "^[ \\t]*$");
	BmString quotingLevelListLineRX
		= ThePrefs->GetString( "QuotingLevelListLineRX",  "^[*+\\-\\d]+.*?$");
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		BmString q(rx.match[i].atom[0]);
		quote.ConvertTabsToSpaces( spacesPerTab, &q);
		// limit the quote chars to a sane length (as otherwise we might
		// loop endlessly when trying to wrap the content lines)
		if (quote.Length() > maxLineLen / 2)
			quote.Truncate(maxLineLen / 2);
		line = rx.match[i].atom[1];
		if ((line.CountChars() < minLenForWrappedLine && lastWasSpecialLine)
		|| rxl.exec( line, quotingLevelEmptyLineRX)
		|| rxl.exec( line, quotingLevelListLineRX)) {
			if (i != 0) {
				maxTextLen = MAX( 0, 
							 			maxLineLen - currQuote.CountChars() 
							 				- quoteString.CountChars());
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = true;
		} else if (lastWasSpecialLine || currQuote != quote 
		|| lastLineLen < minLenForWrappedLine) {
			if (i != 0) {
				maxTextLen = MAX( 0, 
										maxLineLen - currQuote.CountChars() 
											- quoteString.CountChars());
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = false;
		}
		currQuote = quote;
		lastLineLen = line.CountChars();
		if (!text.Length())
			text = line;
		else {
			int32 len = text.Length();
			// trim trailing spaces:
			while( len>0 && text[len-1]==' ')
				len--;
			text.Truncate( len);
			text << " " << line;
		}
	}
	maxTextLen = MAX( 0, 
							maxLineLen - currQuote.CountChars() 
								- quoteString.CountChars());
	AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
	// now remove trailing empty lines:
	BmString emptyLinesAtEndRX 
		= BmString("(?:") << "\\Q" << quoteString << "\\E" 
								<< "(" << "\\Q" << currQuote << "\\E" 
								<< ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", 
							Regexx::newline|Regexx::global|Regexx::noatom);
	return maxLineLen;
}

/*------------------------------------------------------------------------------*\
	AddQuotedLine()
		-	
		-	N.B.: We use the character-count in order to determine line-lengths, 
			which for some charsets (e.g. iso-2022-jp) results in lines longer than
			78 *byte* hard-limit (it just respects a limit of 78 *characters*).
			This probably violates the RFC, but I believe it just makes more sense
			for the users (since characters is what they see on screen, not bytes).
\*------------------------------------------------------------------------------*/
int32 BmMailFactory::AddQuotedText( const BmString& inText, BmString& out, 
									  const BmString& quote,
									  const BmString& quoteString,
								     int maxTextLen) {
	int32 modifiedMaxLen = 0;
	BmString tmp;
	BmString text;
	bool isUrl = false;
	Regexx rxUrl;
	maxTextLen = MAX( 0, maxTextLen);
	text.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), &inText);
	int32 charsLeft = text.CountChars();
	while( charsLeft > maxTextLen) {
		int32 wrapPos = B_ERROR;
		int32 idx=0;
		isUrl = rxUrl.exec(
			text, "^\\s*(https?://|ftp://|nntp://|file://|mailto:)", 
			Regexx::nocase
		);
		for(  int32 charCount=0; 
				charCount<maxTextLen || (isUrl && wrapPos==B_ERROR && text[idx]); 
			   ++charCount) {
			if (IS_UTF8_STARTCHAR(text[idx])) {
				idx++;
				while( IS_WITHIN_UTF8_MULTICHAR(text[idx]))
					idx++;
			} else {
				if (text[idx]==B_SPACE)
					wrapPos = idx+1;
				if (text[idx]=='\n')
					wrapPos = idx+1;
				idx++;
			}
		}
		text.MoveInto( tmp, 0, wrapPos!=B_ERROR ? wrapPos : idx);
		charsLeft -= tmp.CountChars();
		tmp.Prepend( quoteString + quote);
		modifiedMaxLen = MAX( tmp.CountChars(), modifiedMaxLen);
		out << tmp << "\n";
	}
	if (!inText.Length() || text.Length()) {
		tmp = quoteString + quote + text;
		modifiedMaxLen = MAX( tmp.CountChars(), modifiedMaxLen);
		out << tmp << "\n";
	}
	return modifiedMaxLen;
}



/******************************************************************************/
// #pragma mark --- BmReplyFactory ---
/******************************************************************************/

/*------------------------------------------------------------------------------*\
	BmReplyFactory()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmReplyFactory::BmReplyFactory( BmReplyMode replyMode, 
										  bool join, bool joinIntoOne,
										  const BmString& selectedText)
	:	mReplyMode( replyMode)
	,	mJoin( join)
	,	mJoinIntoOne( joinIntoOne)
	,	mSelectedText( selectedText)
{
}

/*------------------------------------------------------------------------------*\
	~BmReplyFactory()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmReplyFactory::~BmReplyFactory()
{
}

struct LessTimewise {
	bool operator() ( const BmRef<BmMailRef>& left, 
							const BmRef<BmMailRef>& right) {
		return left->When() < right->When();
	}
};

/*------------------------------------------------------------------------------*\
	Produce()
		-	replies to all mailref's that have been added so far.
		-	depending on mJoin & mJoinIntoOne, multiple mails are joined into 
			one single reply (one per originator) or one reply is generated 
			for each mail.
\*------------------------------------------------------------------------------*/
void BmReplyFactory::Produce()
{
	BmMailRef* mailRef;
	if (mJoin) {
		// sort mail-refs chronologically...
		sort( mBaseRefVect.begin(), mBaseRefVect.end(), LessTimewise());
		// ...and file mails into the different originator-slots:
		typedef map< BmString, BmRef<BmMail> >BmNewMailMap;
		BmNewMailMap newMailMap;
		for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
				!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
				++iter) {
			mailRef = iter->Get();
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					// couldn't read this mail, ignore it:
					continue;
				BmString replyAddr = DetermineReplyAddress( mail);
				BmString replyAddrSpec = BmAddress( replyAddr).AddrSpec();
				BmString index = mJoinIntoOne 
											? BmString("single") 
											: replyAddrSpec;
				BmRef<BmMail>& newMail = newMailMap[index];
				bool demandNonPersonal = mJoinIntoOne;
				if (!newMail) {
					// first round, create new mail as reply of current:
					newMail = CreateReplyTo( mail, mSelectedText, demandNonPersonal);
				} else {
					// new mail exists, we copy items of current mail into it:
					bool usePersonalPhrase = demandNonPersonal
														? false
														: IsReplyToPersonOnly( mail);
					BmString intro = CreateReplyIntro( mail, usePersonalPhrase);
					CopyMailParts( newMail, mail, false, BM_IS_REPLY, intro);
					BmRef<BmMailHeader> hdr( newMail->Header());
					if (!hdr->AddressFieldContainsAddress( BM_FIELD_TO, replyAddr))
						hdr->AddFieldVal( BM_FIELD_TO, replyAddr);
				}
				if (iter == mBaseRefVect.begin()) {
					// set subject for multiple replies:
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString newSub = CreateReplySubjectFor( oldSub);
					newMail->SetFieldVal( BM_FIELD_SUBJECT, newSub);
				} else {
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString newSub = newMail->GetFieldVal( BM_FIELD_SUBJECT);
					if (newSub != oldSub) {
						BmString suffix(" [...]");
						if (newSub.FindFirst( suffix) < B_OK) {
							newSub << suffix;
							newMail->SetFieldVal( BM_FIELD_SUBJECT, newSub);
						}
					}
				}
			}
		}
		// ...and now add all the freshly generated mails:
		for(  BmNewMailMap::const_iterator iter = newMailMap.begin(); 
			   !BeamRoster->IsQuitting() && iter != newMailMap.end(); 
			   ++iter) {
			TheMails.push_back( iter->second);
		}
	} else {
		// create one reply per mail:
		BmRef<BmMail> newMail;
		for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
				!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
				++iter) {
			mailRef = iter->Get();
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					// couldn't read this mail, ignore it:
					continue;
				newMail = CreateReplyTo( 
					mail,  
					iter==mBaseRefVect.begin() ? mSelectedText : BM_DEFAULT_STRING
				);
				if (newMail)
					TheMails.push_back( newMail);
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	DetermineReplyAddress()
	-	
\*------------------------------------------------------------------------------*/
BmString BmReplyFactory::DetermineReplyAddress( BmRef<BmMail>& mail)
{
	// fill address information, depending on reply-mode:
	BmAddressList replyAddr;
	BmRef<BmMailHeader> header = mail->Header();
	if (!header)
		return "";
	if (mail->Outbound()) {
		// if replying to outbound messages, we re-use the original recipients,
		// not ourselves:
		replyAddr = header->GetAddressList( BM_FIELD_TO);
	} else if (mReplyMode == BM_REPLY_MODE_SMART) {
		// smart (*cough*) mode: If the mail has come from a list, we react
		// according to user prefs (reply-to-list or reply-to-originator).
		bool hasComeFromList = mail->HasComeFromList();
		if (hasComeFromList) {
			replyAddr = ThePrefs->GetBool( "PreferReplyToList", true)
							? header->DetermineListAddress()
							: header->DetermineOriginator();
		}
		if (!replyAddr.AddrCount()) {
			replyAddr = header->DetermineOriginator();
		}
	} else if (mReplyMode == BM_REPLY_MODE_LIST) {
		// blindly use list-address for reply (this might mean that we send
		// a reply to the list although the messages has not come from the list):
		replyAddr = header->DetermineListAddress( true);
	} else if (mReplyMode == BM_REPLY_MODE_PERSON) {
		// bypass the reply-to, this way one can send mail to the 
		// original author of a mail which has been 'reply-to'-munged 
		// by a mailing-list processor:
		replyAddr = header->DetermineOriginator( true);
	} else if (mReplyMode == BM_REPLY_MODE_ALL) {
		// since we are replying to all recipients of this message,
		// we now include the Originator (plain and standard way):
		replyAddr = header->DetermineOriginator();
	}
	return replyAddr.AddrString();
}

/*------------------------------------------------------------------------------*\
	IsReplyToPersonOnly()
	-	
\*------------------------------------------------------------------------------*/
bool BmReplyFactory::IsReplyToPersonOnly( BmRef<BmMail>& mail)
{
	if (mReplyMode == BM_REPLY_MODE_SMART)
		return !mail->HasComeFromList();
	else if (mReplyMode == BM_REPLY_MODE_PERSON)
		return true;
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	CreateReplySubjectFor()
	-	
\*------------------------------------------------------------------------------*/
BmString BmReplyFactory::CreateReplySubjectFor( const BmString& subject)
{
	BmString isReplyRX 
		= ThePrefs->GetString( "ReplySubjectRX");
	Regexx rx;
	if (!rx.exec( subject, isReplyRX, Regexx::nocase|Regexx::nomatch)) {
		BmString subjectStr = ThePrefs->GetString( "ReplySubjectStr");
		subjectStr = rx.replace( subjectStr, "%s", subject, 
								 		 Regexx::nocase|Regexx::global|Regexx::noatom);
		return subjectStr;
	}
	return subject;
}

/*------------------------------------------------------------------------------*\
	CreateReplyIntro()
		-	creates an appropriate intro-line for a reply-message
		-	the returned string is the intro in UTF8
\*------------------------------------------------------------------------------*/
BmString BmReplyFactory::CreateReplyIntro( BmRef<BmMail>& mail, 
														 bool usePersonalPhrase)
{
	BmString intro = ThePrefs->GetString( "ReplyIntroStr");
	ExpandIntroMacros( mail, intro, usePersonalPhrase);
	return intro;
}

/*------------------------------------------------------------------------------*\
	CreateReplyTo()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmReplyFactory::CreateReplyTo( BmRef<BmMail>& oldMail, 
															const BmString& selectedText,
															bool demandNonPersonal)
{
	BmRef<BmMail> newMail = new BmMail( true);
	// copy old message ID into in-reply-to and references fields:
	BmString messageID = oldMail->GetFieldVal( BM_FIELD_MESSAGE_ID);
	newMail->SetFieldVal( BM_FIELD_IN_REPLY_TO, messageID);
	BmString oldRefs = oldMail->GetFieldVal( BM_FIELD_REFERENCES);
	if (oldRefs.Length())
		newMail->SetFieldVal( BM_FIELD_REFERENCES, oldRefs + " " + messageID);
	else
		newMail->SetFieldVal( BM_FIELD_REFERENCES, messageID);
	BmString newTo = DetermineReplyAddress( oldMail);
	newMail->SetFieldVal( BM_FIELD_TO, newTo);

	BmString receivingAddr;
	BmRef<BmIdentity> ident;
	oldMail->DetermineRecvAddrAndIdentity( receivingAddr, ident);
	newMail->SetupFromIdentityAndRecvAddr( ident.Get(), receivingAddr);

	const BmAddressList& toAddrs 
		= oldMail->Header()->GetAddressList(BM_FIELD_TO);
	const BmAddressList& ccAddrs 
		= oldMail->Header()->GetAddressList(BM_FIELD_CC);
	if (mReplyMode == BM_REPLY_MODE_SMART) {
		// in DWIM-mode, we determine if it makes sense to do a reply-to-all 
		// (which is the case if there are more than one recipients of the
		// original message)
		// If the message has come through a list, we do this still, as
		// this makes it possible to include CC'ed addresses in the reply
		// (which is very helpful if those are externals, i.e. not list
		// subscribers).
		if (toAddrs.size() + ccAddrs.size() > 1)
			mReplyMode = BM_REPLY_MODE_ALL;
	}

	// if we are replying to all, we may need to include more addresses:
	if (mReplyMode == BM_REPLY_MODE_ALL) {
		BmAddrList::const_iterator addrIter;
		for( addrIter = ccAddrs.begin(); addrIter != ccAddrs.end(); ++addrIter) {
			// add address only if not already contained in To or Cc
			const BmString& addr = addrIter->AddrString();
			if (!newMail->Header()->AddressFieldContainsAddress(BM_FIELD_TO, addr)
			&& !newMail->Header()->AddressFieldContainsAddress(BM_FIELD_CC, addr))
				newMail->Header()->AddFieldVal( BM_FIELD_CC, addr);
		}
		for( addrIter = toAddrs.begin(); addrIter != toAddrs.end(); ++addrIter) {
			// add address only if not already contained in To or Cc
			const BmString& addr = addrIter->AddrString();
			if (!newMail->Header()->AddressFieldContainsAddress(BM_FIELD_TO, addr)
			&& !newMail->Header()->AddressFieldContainsAddress(BM_FIELD_CC, addr))
				newMail->Header()->AddFieldVal( BM_FIELD_CC, addr);
		}
		// remove the receiving address from list of recipients, since we
		// do not want to send ourselves a reply:
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_TO, receivingAddr);
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_CC, receivingAddr);
	}
	// massage subject, if neccessary:
	BmString subject = oldMail->GetFieldVal( BM_FIELD_SUBJECT);
	subject = CreateReplySubjectFor( subject);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, subject);
	bool usePersonalPhrase = demandNonPersonal
										? false
										: IsReplyToPersonOnly( oldMail);
	BmString intro = CreateReplyIntro( oldMail, usePersonalPhrase);
	CopyMailParts( newMail, oldMail, false, BM_IS_REPLY, intro, selectedText);

	newMail->SetBaseMailInfo( oldMail->MailRef(), BM_MAIL_STATUS_REPLIED);

	return newMail;
}



/******************************************************************************/
// #pragma mark --- BmForwardFactory ---
/******************************************************************************/

/*------------------------------------------------------------------------------*\
	BmForwardFactory()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmForwardFactory::BmForwardFactory( BmForwardMode forwardMode, bool join, 
												const BmString& selectedText)
	:	mForwardMode( forwardMode)
	,	mJoin( join)
	,	mSelectedText( selectedText)
{
}

/*------------------------------------------------------------------------------*\
	~BmForwardFactory()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmForwardFactory::~BmForwardFactory()
{
}

/*------------------------------------------------------------------------------*\
	Produce()
		-	generates forwards for all mailref's that have been added so far.
		-	depending on mJoin, multiple mails are joined into one single 
			forward or one forward is generated for each mail.
\*------------------------------------------------------------------------------*/
void BmForwardFactory::Produce()
{
	BmMailRef* mailRef;
	BmRef<BmMail> newMail;
	if (mJoin) {
		// sort mail-refs chronologically...
		sort( mBaseRefVect.begin(), mBaseRefVect.end(), LessTimewise());
		// ...and file mails into the different originator-slots:
		typedef map< BmString, BmRef<BmMail> >BmNewMailMap;
		BmNewMailMap newMailMap;
		for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
				!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
				++iter) {
			mailRef = iter->Get();
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					// couldn't read this mail, ignore it:
					continue;
				if (mForwardMode == BM_FORWARD_MODE_ATTACHED) {
					if (iter == mBaseRefVect.begin())
						newMail = CreateAttachedForward( mail);
					else
						newMail->AddAttachmentFromRef( mailRef->EntryRefPtr(),
																 mail->DefaultCharset());
				} else {
					if (iter == mBaseRefVect.begin())
						newMail = CreateInlineForward(
							mail, mForwardMode == BM_FORWARD_MODE_INLINE_ATTACH, 
							mSelectedText
						);
					else {
						BmString intro = CreateForwardIntro( mail);
						CopyMailParts( 
							newMail, mail, 
							mForwardMode == BM_FORWARD_MODE_INLINE_ATTACH,
							BM_IS_FORWARD, intro
						);
					}
				}
				if (iter == mBaseRefVect.begin()) {
					// set subject for multiple forwards:
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString newSub = CreateForwardSubjectFor( oldSub);
					newMail->SetFieldVal( BM_FIELD_SUBJECT, newSub);
				} else {
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString newSub = newMail->GetFieldVal( BM_FIELD_SUBJECT);
					if (newSub != oldSub) {
						BmString suffix(" [...]");
						if (newSub.FindFirst( suffix) < B_OK) {
							newSub << suffix;
							newMail->SetFieldVal( BM_FIELD_SUBJECT, newSub);
						}
					}
				}
			}
		}
		if (newMail)
			TheMails.push_back( newMail);
	} else {
		// create one forward per mail:
		BmRef<BmMail> newMail;
		for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
				!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
				++iter) {
			mailRef = iter->Get();
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					// couldn't read this mail, ignore it:
					continue;
				if (mForwardMode == BM_FORWARD_MODE_ATTACHED) {
					newMail = CreateAttachedForward( mail);
				} else {
					newMail = CreateInlineForward( 
						mail, mForwardMode == BM_FORWARD_MODE_INLINE_ATTACH, 
						iter == mBaseRefVect.begin() 
									? mSelectedText 
									: BM_DEFAULT_STRING
					);
				}
				if (newMail)
					TheMails.push_back( newMail);
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateInlineForward()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmForwardFactory::CreateInlineForward( 
	BmRef<BmMail>& mail, bool withAttachments, const BmString& selectedText) 
{
	BmRef<BmMail> newMail = new BmMail( true);
	// massage subject, if neccessary:
	BmString subject = mail->GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));
	BmString intro = CreateForwardIntro( mail);
	CopyMailParts( newMail, mail, withAttachments, BM_IS_FORWARD, intro, 
						selectedText);

	BmString receivingAddr;
	BmRef<BmIdentity> ident;
	mail->DetermineRecvAddrAndIdentity( receivingAddr, ident);
	newMail->SetupFromIdentityAndRecvAddr( ident.Get(), receivingAddr);

	newMail->SetBaseMailInfo( mail->MailRef(), BM_MAIL_STATUS_FORWARDED);

	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateAttachedForward()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmForwardFactory::CreateAttachedForward( BmRef<BmMail>& mail) 
{
	BmRef<BmMail> newMail = new BmMail( true);
	if (mail->MailRef()->InitCheck() == B_OK)
		newMail->Body()->AddAttachmentFromRef( mail->MailRef()->EntryRefPtr(), 
															mail->DefaultCharset());
	// massage subject, if neccessary:
	BmString subject = mail->GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));

	BmString receivingAddr;
	BmRef<BmIdentity> ident;
	mail->DetermineRecvAddrAndIdentity( receivingAddr, ident);
	newMail->SetupFromIdentityAndRecvAddr( ident.Get(), receivingAddr);

	newMail->SetBaseMailInfo( mail->MailRef(), BM_MAIL_STATUS_FORWARDED);

	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateForwardSubjectFor()
	-	
\*------------------------------------------------------------------------------*/
BmString BmForwardFactory::CreateForwardSubjectFor( const BmString& subject) 
{
	BmString isForwardRX 
		= ThePrefs->GetString( "ForwardSubjectRX", 
									  "^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	Regexx rx;
	if (!rx.exec( subject, isForwardRX, Regexx::nocase|Regexx::nomatch)) {
		BmString subjectStr 
			= ThePrefs->GetString( "ForwardSubjectStr", "[Fwd: %s]");
		subjectStr = rx.replace( subjectStr, "%s", subject, 
										 Regexx::nocase|Regexx::global|Regexx::noatom);
		return subjectStr;
	}
	return subject;
}

/*------------------------------------------------------------------------------*\
	CreateForwardIntro()
		-	creates an appropriate intro-line for a forwarded message
		-	the returned string is the intro in UTF8
\*------------------------------------------------------------------------------*/
BmString BmForwardFactory::CreateForwardIntro( BmRef<BmMail>& mail) 
{
	BmString intro = ThePrefs->GetString( "ForwardIntroStr");
	ExpandIntroMacros( mail, intro, false);
	return intro;
}



/******************************************************************************/
// #pragma mark --- BmRedirectFactory ---
/******************************************************************************/

/*------------------------------------------------------------------------------*\
	BmRedirectFactory()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmRedirectFactory::BmRedirectFactory()
{
}

/*------------------------------------------------------------------------------*\
	~BmRedirectFactory()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmRedirectFactory::~BmRedirectFactory()
{
}

/*------------------------------------------------------------------------------*\
	Produce()
		-	generates redirections for all mailref's that have been added so far.
\*------------------------------------------------------------------------------*/
void BmRedirectFactory::Produce()
{
	BmMailRef* mailRef;
	BmRef<BmMail> newMail;

	// create one redirection per mail:
	for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
			!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
			++iter) {
		mailRef = iter->Get();
		BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
		if (mail) {
			if (mail->InitCheck() != B_OK)
				mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
			if (mail->InitCheck() != B_OK)
				// couldn't read this mail, ignore it:
				continue;
			newMail = CreateRedirect( mail);
			if (newMail)
				TheMails.push_back( newMail);
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateRedirect()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmRedirectFactory::CreateRedirect( BmRef<BmMail>& mail) {
	BmRef<BmMail> newMail = new BmMail( true);
	BmString msgText( mail->RawText());
	newMail->SetTo( msgText, "");
	if (newMail->IsRedirect()) {
		// oops, mail already had been redirected, we clobber the existing
		// Resent-fields, since STD11 says multiple Resent-fields result in 
		// undefined behaviour:
		//
		// TODO: time is flying, and the mail header fields are now defined by
		// RFC2822, not STD11. RFC2822 states that new Resent-fields should be
		// prepended to the header. Unfortunately, this would require rather
		// a lot of changes in BmMailEditWin, as that only allows for editing
		// one set of Resent-fields (it would drop the older ones). As I suppose
		// the difference won't ever be noticed, we simply go with a single
		// set of Resent-fields (for now):
		newMail->RemoveField( BM_FIELD_RESENT_BCC);
		newMail->RemoveField( BM_FIELD_RESENT_CC);
		newMail->RemoveField( BM_FIELD_RESENT_DATE);
		newMail->RemoveField( BM_FIELD_RESENT_FROM);
		newMail->RemoveField( BM_FIELD_RESENT_MESSAGE_ID);
		newMail->RemoveField( BM_FIELD_RESENT_REPLY_TO);
		newMail->RemoveField( BM_FIELD_RESENT_SENDER);
		newMail->RemoveField( BM_FIELD_RESENT_TO);
	}
	newMail->IsRedirect( true);
	newMail->SetFieldVal( BM_FIELD_RESENT_DATE, 
								 TimeToString( time( NULL), 
								 					"%a, %d %b %Y %H:%M:%S %z"));

	BmString receivingAddr;
	BmRef<BmIdentity> ident;
	mail->DetermineRecvAddrAndIdentity( receivingAddr, ident);
	if (ident && receivingAddr.Length()) {
		newMail->SetFieldVal( BM_FIELD_RESENT_FROM, receivingAddr);
		newMail->SetSignatureByName( ident->SignatureName());
		newMail->AccountName( ident->SMTPAccount());
		newMail->IdentityName( ident->Key());
	}

	newMail->SetBaseMailInfo( mail->MailRef(), BM_MAIL_STATUS_REDIRECTED);

	return newMail;
}



/******************************************************************************/
// #pragma mark --- BmCopyMailFactory ---
/******************************************************************************/

/*------------------------------------------------------------------------------*\
	BmCopyMailFactory()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmCopyMailFactory::BmCopyMailFactory()
{
}

/*------------------------------------------------------------------------------*\
	~BmCopyMailFactory()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmCopyMailFactory::~BmCopyMailFactory()
{
}

/*------------------------------------------------------------------------------*\
	Produce()
		-	generates copies for all mailref's that have been added so far.
\*------------------------------------------------------------------------------*/
void BmCopyMailFactory::Produce()
{
	BmMailRef* mailRef;
	BmRef<BmMail> newMail;

	// create one copy per mail:
	for(  BmMailRefVect::const_iterator iter = mBaseRefVect.begin(); 
			!BeamRoster->IsQuitting() && iter != mBaseRefVect.end(); 
			++iter) {
		mailRef = iter->Get();
		BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
		if (mail) {
			if (mail->InitCheck() != B_OK)
				mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
			if (mail->InitCheck() != B_OK)
				// couldn't read this mail, ignore it:
				continue;
			newMail = CreateCopy( mail);
			if (newMail)
				TheMails.push_back( newMail);
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateCopy()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmCopyMailFactory::CreateCopy( BmRef<BmMail>& oldMail) {
	BmRef<BmMail> newMail = new BmMail( true);
	BmString msgText( oldMail->RawText());
	newMail->SetTo( msgText, "");

	// Remove anything but the basic fields from the mail header.
	// Apart from being cleaner, this will remove any fields that might 
	// inhibit the processing of this mail by any mailing list software 
	// (they might consider this mail having looped and would drop it):
	set<BmString> basicFields;
	basicFields.insert(BM_FIELD_BCC);
	basicFields.insert(BM_FIELD_CC);
	basicFields.insert(BM_FIELD_FROM);
	basicFields.insert(BM_FIELD_REPLY_TO);
	basicFields.insert(BM_FIELD_SUBJECT);
	basicFields.insert(BM_FIELD_TO);
	vector<BmString> fieldNames;
	newMail->Header()->GetAllFieldNames(fieldNames);
	vector<BmString>::iterator iter;
	for( iter = fieldNames.begin(); iter != fieldNames.end(); ++iter) {
		if (basicFields.find(*iter) == basicFields.end())
			newMail->Header()->RemoveField(*iter);
	}

	// re-set the identity to update the fields depending on it:
	BmRef<BmListModelItem> identRef 
		= TheIdentityList->FindItemByKey( oldMail->IdentityName());
	BmIdentity* ident = dynamic_cast<BmIdentity*>( identRef.Get());
	if (ident) {
		newMail->AccountName( ident->SMTPAccount());
		newMail->SetSignatureByName( ident->SignatureName());
		newMail->IdentityName( ident->Key());
	}
	return newMail;
}

