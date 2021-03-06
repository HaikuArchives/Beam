/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmSieveFilter_h
#define _BmSieveFilter_h

#include <Archivable.h>
#include <Autolock.h>

extern "C" {
	#include "sieve_interface.h"
}

#include "BmFilterAddon.h"
#include "BmFilterAddonPrefs.h"

const int BM_MAX_MATCH_COUNT = 20;

/*------------------------------------------------------------------------------*\
	BmSieveFilter 
		-	implements filtering through SIEVE
\*------------------------------------------------------------------------------*/
class BmSieveFilter : public BmFilterAddon {
	typedef BmFilterAddon inherited;
	
	friend class SieveTest;

public:
	BmSieveFilter( const BmString& name, const BMessage* archive);
	virtual ~BmSieveFilter();
	
	// native methods:
	bool CompileScript();
	BLocker* SieveLock();
	virtual bool AskBeforeFileInto()		{ return false; }

	// implementations for abstract BmFilterAddon-methods:
	bool Execute( BmMsgContext* msgContext, 
					  const BMessage* jobSpecs = NULL);
	virtual void Initialize();
	bool SanityCheck( BmString& complaint, BmString& fieldName);
	status_t Archive( BMessage* archive, bool deep = true) const;
	BmString ErrorString() const;

	// SIEVE-callbacks:
	static int sieve_redirect( void* action_context, void* interp_context, 
			   						void* script_context, void* message_context, 
			   						const char** errmsg);
	static int sieve_keep( void* action_context, void* interp_context, 
			   				  void* script_context, void* message_context, 
			   				  const char** errmsg);
	static int sieve_discard( void* action_context, void* interp_context, 
			   				     void* script_context, void* message_context, 
			   				     const char** errmsg);
	static int sieve_fileinto( void* action_context, void* interp_context, 
			   				  		void* script_context, void* message_context, 
			   				  		const char** errmsg);
	static int sieve_reject( void* action_context, void* interp_context, 
			   					 void* script_context, void* message_context, 
			   					 const char** errmsg);
	static int sieve_notify( void* action_context, void*, 
			   				  	 void*, void* message_context, 
			   				  	 const char**);
	static int sieve_get_size( void* message_context, int* size);
	static int sieve_get_header( void* message_context, const char* header,
			  							  const char*** contents);
	static int sieve_parse_error( int lineno, const char *msg, 
											void *interp_context, void *script_context);
	// SIEVE-helpers:
	static void SetMailFlags( sieve_imapflags_t* flags, 
									  BmMsgContext* msgContext);

	static int sieve_execute_error( const char* msg, void* interp_context,
											  void* script_context, void* message_context);

	// getters:
	inline const BmString &Content() const	
													{ return mContent; }
	inline const BmString &Name() const	{ return mName; }

	inline int LastErrVal() const			{ return mLastErrVal; }
	inline const BmString &LastErr() const	
													{ return mLastErr; }
	inline const BmString &LastSieveErr() const 
													{ return mLastSieveErr; }

	// setters:
	void Content( const BmString &s);

	// archivable components:
	static const char* const MSG_VERSION;
	static const char* const MSG_CONTENT;
	static const int16 nArchiveVersion;

protected:
	void RegisterCallbacks( sieve_interp_t* interp);

	BmString mName;
							// the name of this filter-implementation
	BmString mContent;
							// the SIEVE-script represented by this filter
	sieve_script_t* mCompiledScript;
							// the compiled SIEVE-script, ready to be thrown at mails
	sieve_interp_t* mSieveInterp;
							// the interpreter that compiled the SIEVE-script
	int mLastErrVal;
							// last error-value we got
	BmString mLastErr;
							// the last (general) error that occurred
	BmString mLastSieveErr;
							// the last SIEVE-error that occurred
	static BLocker* nSieveLock;

private:
	BmSieveFilter();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmSieveFilter( const BmSieveFilter&);
	BmSieveFilter operator=( const BmSieveFilter&);

};



/*------------------------------------------------------------------------------*\
	BmGraphicalSieveFilter 
		-	additionally supports graphical editing of the filter
\*------------------------------------------------------------------------------*/
class BmGraphicalSieveFilter : public BmSieveFilter {
	typedef BmSieveFilter inherited;

	friend class BmSieveFilterPrefs;

	// archivable components:
	static const char* const MSG_MATCH_COUNT;
	static const char* const MSG_MATCH_ANYALL;
	static const char* const MSG_MATCH_MAILPART;
	static const char* const MSG_MATCH_ADDRPART;
	static const char* const MSG_MATCH_FIELDNAME;
	static const char* const MSG_MATCH_OPERATOR;
	static const char* const MSG_MATCH_VALUE;
	static const char* const MSG_FILEINTO;
	static const char* const MSG_FILEINTO_VALUE;
	static const char* const MSG_FILEINTO_ASK;
	static const char* const MSG_DISCARD;
	static const char* const MSG_SET_STATUS;
	static const char* const MSG_SET_STATUS_VALUE;
	static const char* const MSG_SET_IDENTITY;
	static const char* const MSG_SET_IDENTITY_VALUE;
	static const char* const MSG_SET_SPAM_TOFU;
	static const char* const MSG_SET_SPAM_TOFU_VALUE;
	static const char* const MSG_STOP_PROCESSING;
	static const char* const MSG_SET_LIST_ID;
	static const char* const MSG_SET_LIST_ID_VALUE;

public:
	BmGraphicalSieveFilter( const BmString& name, const BMessage* archive);
	
	// native methods:
	bool BuildScriptFromStrings();
	bool IsAddrField( const BmString& addrField);

	// overrides of BmSieve-base:
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, 
									const BmString& newVal);
	bool SanityCheck( BmString& complaint, BmString& fieldName);
	status_t Archive( BMessage* archive, bool deep = true) const;
	void SetupFromMailData( const BmString& subject, 
									const BmString& from, 
									const BmString& To);
	bool AskBeforeFileInto()				{ return mActionFileIntoAsk; }

private:

	// stuff used by graphical editor:
	int16 mMatchCount;
	BmString mMatchAnyAll;
	BmString mMatchMailPart[BM_MAX_MATCH_COUNT];
	BmString mMatchAddrPart[BM_MAX_MATCH_COUNT];
	BmString mMatchFieldName[BM_MAX_MATCH_COUNT];
	BmString mMatchOperator[BM_MAX_MATCH_COUNT];
	BmString mMatchValue[BM_MAX_MATCH_COUNT];
	bool mActionFileInto;
	BmString mActionFileIntoValue;
	bool mActionFileIntoAsk;
	bool mActionDiscard;
	bool mActionSetStatus;
	BmString mActionSetStatusValue;
	bool mActionSetIdentity;
	BmString mActionSetIdentityValue;
	bool mActionSetSpamTofu;
	BmString mActionSetSpamTofuValue;
	bool mStopProcessing;
	bool mActionSetListId;
	BmString mActionSetListIdValue;

	BmGraphicalSieveFilter();				// hide default constructor
	// Hide copy-constructor and assignment:
	BmGraphicalSieveFilter( const BmGraphicalSieveFilter&);
	BmGraphicalSieveFilter operator=( const BmGraphicalSieveFilter&);

};



/*------------------------------------------------------------------------------*\
	BmSieveFilterPrefs
		-	
\*------------------------------------------------------------------------------*/

class BmCheckControl;
class BmMenuControl;
class BmTextControl;
class BmMultiLineTextControl;
class LayeredGroup;
class BmFilterScrollView;
class BmFilterGroup;
class Space;
class MButton;
class HGroup;

enum {
	BM_ANY_ALL_SELECTED			= 'bmTa',
	BM_ACTION_SELECTED			= 'bmTb',
	BM_MAILPART_SELECTED			= 'bmTc',
	BM_OPERATOR_SELECTED			= 'bmTd',
	BM_SELECT_FILTER_VALUE		= 'bmTe',
	BM_SELECT_ACTION_VALUE		= 'bmTf',
	BM_ADD_FILTER_LINE			= 'bmTg',
	BM_REMOVE_FILTER_LINE		= 'bmTh',

	BM_FILEINTO_CHANGED			= 'bmTi',
	BM_FILEINTO_SELECTED			= 'bmTj',
	BM_DISCARD_CHANGED			= 'bmTk',
	BM_SET_STATUS_CHANGED		= 'bmTl',
	BM_SET_STATUS_SELECTED		= 'bmTm',
	BM_SET_IDENTITY_CHANGED		= 'bmTn',
	BM_SET_IDENTITY_SELECTED	= 'bmTo',
	BM_STOP_PROCESSING_CHANGED	= 'bmTp',
	BM_SET_SPAM_TOFU_CHANGED	= 'bmTq',
	BM_SET_SPAM_TOFU_SELECTED	= 'bmTr',
	BM_SET_LIST_ID_CHANGED		= 'bmTs',

	BM_ADDRPART_SELECTED			= 'bmTu',
	BM_FILEINTO_ASK_CHANGED		= 'bmTv'
};


class BmSieveFilterPrefs : public BmFilterAddonPrefsView {
	typedef BmFilterAddonPrefsView inherited;

public:
	BmSieveFilterPrefs( minimax minmax);
	virtual ~BmSieveFilterPrefs();
	
	// native methods:
	void AdjustScrollView();
	void AdjustSizeOfValueControl( BmMultiLineTextControl* control);

	// implementations for abstract base-class methods:
	const char *Kind() const;
	void ShowFilter( BmFilterAddon* addon);
	void Initialize();
	void Activate();

	// BView overrides:
	void MessageReceived( BMessage* msg);

private:

	static const char* const MSG_IDX;

	void AddFilterLine();
	void RemoveMarkedFilterLines();
	void UpdateState();

	BmFilterGroup* mFilterGroup;
	VGroup* mActionGroup;
	BmFilterScrollView* mFilterScrollView;
	Space* mSpaceAtBottom;
	
	BmMenuControl* mAnyAllControl;
	MButton* mAddButton;
	MButton* mRemoveButton;
	HGroup* mFilterLine[BM_MAX_MATCH_COUNT];
	BmMenuControl* mMailPartControl[BM_MAX_MATCH_COUNT];
	LayeredGroup* mFieldSpecLayer[BM_MAX_MATCH_COUNT];
	BmMenuControl* mAddrPartControl[BM_MAX_MATCH_COUNT];
	BmTextControl* mFieldNameControl[BM_MAX_MATCH_COUNT];
	BmMenuControl* mOperatorControl[BM_MAX_MATCH_COUNT];
	BmMultiLineTextControl* mValueControl[BM_MAX_MATCH_COUNT];
	BmCheckControl* mMarkControl[BM_MAX_MATCH_COUNT];
	int32 mVisibleLines;
	
	BmCheckControl* mFileIntoControl;
	BmMenuControl* mFileIntoValueControl;
	BmCheckControl* mFileIntoAskControl;
	BmCheckControl* mDiscardControl;
	BmCheckControl* mSetStatusControl;
	BmMenuControl* mSetStatusValueControl;
	BmCheckControl* mSetIdentityControl;
	BmMenuControl* mSetIdentityValueControl;
	BmCheckControl* mSetSpamTofuControl;
	BmMenuControl* mSetSpamTofuValueControl;
	BmCheckControl* mStopProcessingControl;
	BmCheckControl* mSetListIdControl;
	BmTextControl* mSetListIdValueControl;

	BmGraphicalSieveFilter* mCurrFilterAddon;

	// Hide copy-constructor and assignment:
	BmSieveFilterPrefs( const BmSieveFilterPrefs&);
	BmSieveFilterPrefs operator=( const BmSieveFilterPrefs&);
};



/*------------------------------------------------------------------------------*\
	BmSieveScriptFilterPrefs
		-	
\*------------------------------------------------------------------------------*/

class BmMultiLineTextControl;

enum {
	BM_TEST_FILTER			= 'bmTz'
};

class BmSieveScriptFilterPrefs : public BmFilterAddonPrefsView {
	typedef BmFilterAddonPrefsView inherited;

public:
	BmSieveScriptFilterPrefs( minimax minmax);
	virtual ~BmSieveScriptFilterPrefs();
	
	// native methods:

	// implementations for abstract base-class methods:
	const char *Kind() const;
	void ShowFilter( BmFilterAddon* addon);
	void Initialize();

	// BView overrides:
	void MessageReceived( BMessage* msg);

private:

	BmMultiLineTextControl* mContentControl;
	MButton* mTestButton;

	BmSieveFilter* mCurrFilterAddon;

	// Hide copy-constructor and assignment:
	BmSieveScriptFilterPrefs( const BmSieveScriptFilterPrefs&);
	BmSieveScriptFilterPrefs operator=( const BmSieveScriptFilterPrefs&);
};

#endif
