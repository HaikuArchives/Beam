/*
	BmStorageUtil.cpp
		$Id$
*/

#include <Messenger.h> 
#include <Message.h> 
#include <Entry.h> 
#include <stdio.h> 
#include <string.h> 

#include "BmStorageUtil.h"

bool MoveToTrash( entry_ref eref) { 
	// this is code I got from Tim Vernum's Website. thx!
	BMessenger tracker("application/x-vnd.Be-TRAK" ); 
	if (tracker.IsValid()) { 
		BMessage msg( B_DELETE_PROPERTY ) ; 

		BMessage specifier( 'sref' ) ; 
		specifier.AddRef( "refs", &eref ) ; 
		specifier.AddString( "property", "Entry" ) ; 
		msg.AddSpecifier( &specifier ) ; 

		msg.AddSpecifier( "Poses" ) ; 
		msg.AddSpecifier( "Window", 1 ) ; 

		return tracker.SendMessage( &msg) == B_OK; 
	} else 
		return false; 
} 
