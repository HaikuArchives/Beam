/*
	BmSpamFilter.cpp

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


#include <Alert.h>
#include <File.h>
#include <Message.h>

#include <MButton.h>
#include <Space.h>

#include "BubbleHelper.h"

#include "BmLogHandler.h"
#include "BmSpamFilter.h"


extern "C" __declspec(dllexport)
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind);

extern "C" __declspec(dllexport) 
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind);

/********************************************************************************\
	BmSpamFilter
\********************************************************************************/

const char* const BmSpamFilter::MSG_VERSION = 		"bm:version";
const int16 BmSpamFilter::nArchiveVersion = 1;
BLocker* BmSpamFilter::nLock = NULL;

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

const char* FILTER_SPAM 			= "Spam";

extern "C" __declspec(dllexport) 
const char* FilterKinds[] = {
	FILTER_SPAM,
	NULL
};

extern "C" __declspec(dllexport) 
const char* DefaultFilterName = "<<< SPAM-filter >>>";

/*------------------------------------------------------------------------------*\
	BmSpamFilter( archive)
		-	c'tor
		-	constructs a BmSpamFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmSpamFilter::BmSpamFilter( const BmString& name, const BMessage* archive) 
	:	mName( name)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
}

/*------------------------------------------------------------------------------*\
	~BmSpamFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmSpamFilter::~BmSpamFilter() {
}

/*------------------------------------------------------------------------------*\
	SpamLock()
\*------------------------------------------------------------------------------*/
BLocker* BmSpamFilter::Lock() {
	if (!nLock)
		nLock = new BLocker( "SpamLock", true);
	return nLock;
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSpamFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::Archive( BMessage* archive, bool) const {
	status_t ret = (archive->AddInt16( MSG_VERSION, nArchiveVersion));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Execute()
		-	
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::Execute( void* message_context) {
	BmMsgContext* msgContext = static_cast< BmMsgContext*>( message_context);
	BmString mailId;
	if (msgContext)
		mailId = msgContext->mailId;
	BM_LOG2( BM_LogFilter, BmString("Spam-Addon: asked to execute filter <") 
									<< Name() 
									<< "> on mail with Id <" << mailId << ">");
	BM_LOG2( BM_LogFilter, "Spam-Addon: starting execution...");
//	int res = sieve_execute_script( mCompiledScript, msgContext);
	BM_LOG2( BM_LogFilter, "Spam-Addon: done.");
//	return res == SIEVE_OK;
	return true;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::SanityCheck( BmString& complaint, BmString& fieldName) {
	return true;
}

/*------------------------------------------------------------------------------*\
	ErrorString()
		-	
\*------------------------------------------------------------------------------*/
BmString BmSpamFilter::ErrorString() const {
	return mLastErr;
}

/*------------------------------------------------------------------------------*\
	InstantiateFilter()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport)
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind) {
	if (!kind.ICompare( FILTER_SPAM))
		return new BmSpamFilter( name, archive);
	else
		return NULL;
}



/********************************************************************************\
	BmSpamFilterPrefs
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmSpamFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilterPrefs::BmSpamFilterPrefs( minimax minmax)
	:	inherited( minmax.mini.x, minmax.mini.y, minmax.maxi.x, minmax.maxi.y)
	,	mCurrFilterAddon( NULL)
{
	VGroup* vgroup = 
		new VGroup( 
			mStatisticsButton = new MButton( 
				"Show Statistics...", 
				new BMessage( BM_SHOW_STATISTICS),
				this, minimax(-1,-1,-1,-1)
			),
			new Space(minimax(0,0,1E5,1E5)),
			0
		);
	
	AddChild( dynamic_cast<BView*>( vgroup));
}

/*------------------------------------------------------------------------------*\
	~BmSpamFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilterPrefs::~BmSpamFilterPrefs() {
	TheBubbleHelper->SetHelp( mStatisticsButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::Initialize() {
	TheBubbleHelper->SetHelp( 
		mStatisticsButton, 
		""
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::Activate() {
}

/*------------------------------------------------------------------------------*\
	Kind()
		-	
\*------------------------------------------------------------------------------*/
const char* BmSpamFilterPrefs::Kind() const { 
	return FILTER_SPAM;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_SHOW_STATISTICS: {
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	ShowFilter()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::ShowFilter( BmFilterAddon* addon) {
}



/*------------------------------------------------------------------------------*\
	InstantiateFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) 
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind) {
	if (!kind.ICompare( FILTER_SPAM))
		return new BmSpamFilterPrefs( minimax( minx, miny, maxx, maxy));
	else
		return NULL;
}
