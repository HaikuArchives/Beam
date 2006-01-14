/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailFactory_h
#define _BmMailFactory_h

#include "BmMailKit.h"

#include <vector>

#include "BmMail.h"
#include "BmMailRef.h"

// convenience-consts for AddPartsFromMail()-param isForward:
const bool BM_IS_FORWARD = true;
const bool BM_IS_REPLY = false;

typedef vector< BmRef< BmMail> > BmMailVect;
/*------------------------------------------------------------------------------*\
	BmMailFactory
		-	this class encapsulates the generation of new mails that are based
			on other mails (like replies and forwards).
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailFactory {

public:
	BmMailFactory()							{}
	virtual ~BmMailFactory()				{}
	//
	void AddBaseMail( BmMail* mail);
	void AddBaseMailRef( BmMailRef* ref);
	//
	virtual void Produce()					= 0;
	//
	BmMailVect TheMails;
							// the resulting new mail(s)
	static const char* const BM_QUOTE_AUTO_WRAP;
	static const char* const BM_QUOTE_SIMPLE;
	static const char* const BM_QUOTE_PUSH_MARGIN;

protected:
	void CopyMailParts( BmRef<BmMail>& newMail, BmRef<BmMail>& oldMail,
							  bool withAttachments, bool isForward, 
							  const BmString& intro,
							  const BmString& selectedText = BM_DEFAULT_STRING);
	void ExpandIntroMacros( BmRef<BmMail> mail, BmString& intro, 
									bool usePersonalPhrase);

	// Static functions that try to reformat & quote a given multiline text
	// in a way that avoids the usual (ugly) quoting-mishaps.
	// The resulting int is the line-length needed to leave 
	// the formatting intact.
	static int32 QuoteText( const BmString& in, BmString& out, 
									const BmString quote, int maxLen);
	static int32 QuoteTextWithReWrap( const BmString& in, BmString& out, 
											    BmString quoteString, int maxLineLen);
	static int32 AddQuotedText( const BmString& text, BmString& out, 
										 const BmString& quote, 
										 const BmString& quoteString,
								 		 int maxTextLen);
	BmMailRefVect mBaseRefVect;
							// the mailref(s) that created us (via forward/reply)
};



/*------------------------------------------------------------------------------*\
	BmReplyFactory
		-	this class encapsulates the generation of reply-mails
\*------------------------------------------------------------------------------*/
enum BmReplyMode {
	BM_REPLY_MODE_SMART = 0,
	BM_REPLY_MODE_LIST,
	BM_REPLY_MODE_PERSON,
	BM_REPLY_MODE_ALL
};
class IMPEXPBMMAILKIT BmReplyFactory : public BmMailFactory {

public:
	BmReplyFactory( BmReplyMode replyMode, bool join, bool joinIntoOne,
						 const BmString& selectedText = BM_DEFAULT_STRING);
	~BmReplyFactory();
	
	void Produce();

private:	
	BmString DetermineReplyAddress( BmRef<BmMail>& mail, bool canonicalize);
	bool IsReplyToPersonOnly( BmRef<BmMail>& mail);

	BmString CreateReplySubjectFor( const BmString& subject);
	BmString CreateReplyIntro( BmRef<BmMail>& mail, bool usePersonalPhrase);
	BmRef<BmMail> CreateReplyTo( BmRef<BmMail>& oldMail, 
										  const BmString& selectedText,
										  bool demandNonPersonal = false);

	BmReplyMode mReplyMode;
		// mode of reply (smart, to-list, to-person, to-all)
	bool mJoin;
		// should the mails be joined at all?
	bool mJoinIntoOne;
		// should all mails be joined into one single mail or one per recipient?
	BmString mSelectedText;
		// the text that was selected from the original mail
};



/*------------------------------------------------------------------------------*\
	BmForwardFactory
		-	this class encapsulates the generation of forwarded mails
\*------------------------------------------------------------------------------*/
enum BmForwardMode {
	BM_FORWARD_MODE_INLINE = 0,
	BM_FORWARD_MODE_ATTACHED,
	BM_FORWARD_MODE_INLINE_ATTACH
};
class IMPEXPBMMAILKIT BmForwardFactory : public BmMailFactory {

public:
	BmForwardFactory( BmForwardMode forwardMode, bool join, 
						 	const BmString& selectedText = BM_DEFAULT_STRING);
	~BmForwardFactory();
	
	void Produce();

private:	
	BmString CreateForwardSubjectFor( const BmString& subject);
	BmString CreateForwardIntro( BmRef<BmMail>& mail);
	BmRef<BmMail> CreateInlineForward( BmRef<BmMail>& oldMail, 
												  bool withAttachments,
										  		  const BmString& selectedText);
	BmRef<BmMail> CreateAttachedForward( BmRef<BmMail>& oldMail);

	BmForwardMode mForwardMode;
		// mode of forward (inline, inline_with_attachments, attached)
	bool mJoin;
		// should the mails be joined at all?
	BmString mSelectedText;
		// the text that was selected from the original mail
};



/*------------------------------------------------------------------------------*\
	BmRedirectFactory
		-	this class encapsulates the redirection of mails (as new)
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmRedirectFactory : public BmMailFactory {

public:
	BmRedirectFactory();
	~BmRedirectFactory();
	
	void Produce();

private:	
	BmRef<BmMail> CreateRedirect( BmRef<BmMail>& oldMail);

};



/*------------------------------------------------------------------------------*\
	BmCopyMailFactory
		-	this class encapsulates the copying of mails (as new)
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmCopyMailFactory : public BmMailFactory {

public:
	BmCopyMailFactory();
	~BmCopyMailFactory();
	
	void Produce();

private:	
	BmRef<BmMail> CreateCopy( BmRef<BmMail>& oldMail);

};



#endif
