/*
	BmStorageUtil.cpp
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


#include <stdio.h> 
#include <string.h> 

#include <Messenger.h> 
#include <Message.h> 
#include <Entry.h> 
#include <NodeInfo.h> 

#include "BmStorageUtil.h"

BmTempFileList TheTempFileList;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool MoveToTrash( const entry_ref* refs, int32 count) {
	// this is basically code I got from Tim Vernum's Website. thx!
	BMessenger tracker("application/x-vnd.Be-TRAK" );
	if (refs && tracker.IsValid()) {
		BMessage msg( B_DELETE_PROPERTY );

		BMessage specifier( 'sref' );
		for( int i=0; i<count; ++i) {
			specifier.AddRef( "refs", &refs[i]);
		}
		specifier.AddString( "property", "Entry" );
		msg.AddSpecifier( &specifier );

		msg.AddSpecifier( "Poses" );
		msg.AddSpecifier( "Window", 1 );

		return tracker.SendMessage( &msg) == B_OK;
	} else
		return false;
} 


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool CheckMimeType( const entry_ref* eref, const char* type) {
	BNode node( eref);
	if (node.InitCheck() == B_OK) {
		BNodeInfo nodeInfo( &node);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimetype[B_MIME_TYPE_LENGTH+1];
			nodeInfo.GetType( mimetype);
			return strcasecmp( type, mimetype) == 0;
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	~BmTempFileList()
		-	
\*------------------------------------------------------------------------------*/
BmTempFileList::~BmTempFileList() {
	int count = mFiles.size();
	for( int i=0; i<count; ++i) {
		BEntry tmpFile( mFiles[i].String());
		tmpFile.Remove();
	}
}

/*------------------------------------------------------------------------------*\
	AddFile()
		-	
\*------------------------------------------------------------------------------*/
void BmTempFileList::AddFile( BString fileWithPath) {
	mFiles.push_back( fileWithPath);
}
