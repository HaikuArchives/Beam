/*
	BmStorageUtil.cpp
		$Id$
*/

#include <stdio.h> 
#include <string.h> 

#include <Messenger.h> 
#include <Message.h> 
#include <Entry.h> 
#include <NodeInfo.h> 

#include "BmStorageUtil.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool MoveToTrash( entry_ref* refs, int32 count) {
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
bool CheckMimeType( entry_ref* eref, const char* type) {
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
