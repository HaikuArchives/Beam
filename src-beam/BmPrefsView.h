/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsView_h
#define _BmPrefsView_h

#include <Message.h>
#include "BmString.h"
#include <View.h>

#include <layout.h>
#include <MBorder.h>

enum {
	BM_SELECTION_CHANGED 		= 'bm_S',
	BM_COMPLAIN_ABOUT_FIELD 	= 'bm_C',
	BM_PREFS_CHANGED 				= 'bm_!'
};

class MStringView;
class BmListModel;
/*------------------------------------------------------------------------------*\
	BmPrefsView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsView : public MBorder {
	typedef MBorder inherited;

public:
	// c'tors and d'tor:
	BmPrefsView( BmString label);
	virtual ~BmPrefsView();

	// native methods:
	virtual void Activated();
	virtual void Initialize();
	virtual void Update()					{}
	virtual void SaveData()					{}
	virtual void UndoChanges()				{}
	virtual void SetDefaults()				{}
	virtual bool SanityCheck()				{ return true; }
	virtual void WriteStateInfo()			{}

	void NoticeChange();
	void ResetChanged()						{ mChanged = false; }

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:
	bool InitDone() const					{ return mInitDone; }
	const char* Name();

	// message-fields:
	static const char* const MSG_ITEM;
	static const char* const MSG_FIELD_NAME;
	static const char* const MSG_COMPLAINT;

protected:
	bool DoSanityCheck( BmListModel* list, const BmString& viewName);

	BView* mGroupView;
	bool mChanged;

private:
	MStringView* mLabelView;
	bool mInitDone;
	
	// Hide copy-constructor and assignment:
	BmPrefsView( const BmPrefsView&);
	BmPrefsView operator=( const BmPrefsView&);
};



/*------------------------------------------------------------------------------*\
	BmPrefsViewContainer
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsViewContainer : public MBorder
{

public:
	BmPrefsViewContainer( LayeredGroup* group);
	virtual ~BmPrefsViewContainer() 				{}
	
	// native methods:
	void ShowPrefs( int index);
	void WriteStateInfo();
	bool ApplyChanges();
	void RevertChanges();
	void SetDefaults();
	BmPrefsView* ShowPrefsByName( const BmString name, int32& indexOut);

private:
	LayeredGroup* mLayeredGroup;

};

#endif
