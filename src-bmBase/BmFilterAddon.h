/*
	BmFilterAddon.h

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


#ifndef _BmFilterAddon_h
#define _BmFilterAddon_h

#include <Message.h>

#include "BmBase.h"
#include "BmString.h"

class BmMail;
struct IMPEXPBMBASE BmHeaderInfo {
	BmString fieldName;
	const char** values;
};
/*------------------------------------------------------------------------------*\
	BmMsgContext
		-	
\*------------------------------------------------------------------------------*/
struct IMPEXPBMBASE BmMsgContext {
	BmMsgContext( BmMail* mail);
	~BmMsgContext();

	// info fields, are set before filtering starts:
	BmMail* mail;
	int32 headerInfoCount;
	BmHeaderInfo *headerInfos;
	
	// result fields, are set by filtering process:
	BmString folderName;
	BmString status;
	BmString identity;
	BmString rejectMsg;
	uint8 spamValue;
	bool moveToTrash;
	bool stopProcessing;
};



/*------------------------------------------------------------------------------*\
	BmFilterAddon 
		-	base class for all filter-addons, this is used as filter-addon-API
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmFilterAddon {

public:
	BmFilterAddon();
	virtual ~BmFilterAddon();
	
	// native methods:
	virtual bool Execute( BmMsgContext* msgContext, 
								 const BmString& jobSpecifier = BM_DEFAULT_STRING) = 0;
	virtual void Initialize()				{}
	virtual bool SanityCheck( BmString& complaint, BmString& fieldName) = 0;
	virtual status_t Archive( BMessage* archive, bool deep = true) const = 0;
	virtual BmString ErrorString() const = 0;

	virtual void ForeignKeyChanged( const BmString& /* key */, 
											  const BmString& /* oldVal */, 
											  const BmString& /* newVal */) 
											  		{}

	virtual void SetupFromMailData( const BmString& /* subject */, 
											  const BmString& /* from */, 
											  const BmString& /* To */)	  
											  		{}

	// foreign-key identifiers:
	static const char* const FK_FOLDER;
	static const char* const FK_IDENTITY;

private:

	// Hide copy-constructor:
	BmFilterAddon( const BmFilterAddon&);
};

typedef BmFilterAddon* (*BmInstantiateFilterFunc)( const BmString& name, 
																	const BMessage* archive,
																	const BmString& kind);

class BmFilterAddonPrefsView;
typedef BmFilterAddonPrefsView* (*BmInstantiateFilterPrefsFunc)( 
	float minX, float minY, float maxX, float maxY, const BmString& kind
);

#endif
