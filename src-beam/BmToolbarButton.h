/*
	BmToolbarButton.h
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


#ifndef _BmToolbarButton_h
#define _BmToolbarButton_h

#include <set>
#include <vector>

#include <Locker.h>
#include <Message.h>

#include <MBorder.h>
#include <MPictureButton.h>
#include <Space.h>

#include "BmString.h"

class BmToolbar :  public MBorder
{
	typedef MBorder inherited;
public:
	BmToolbar(MView* kid);
	~BmToolbar();
	void UpdateLayout();
};

class BmToolbarManager
{
	typedef set<BmToolbar*> BmToolbarSet;
public:
	BmToolbarManager();
	void AddToolbar( BmToolbar* tb);
	void RemoveToolbar( BmToolbar* tb);
	void UpdateAllToolbars();
	static BmToolbarManager* Instance();
private:
	static BmToolbarManager* theInstance;
	BmToolbarSet mToolbarSet;
	BLocker mToolbarLock;
};

#define TheToolbarManager (BmToolbarManager::Instance())

class BmToolbarSpace :  public Space
{
public:
	void Draw( BRect updateRect);
};

class BmToolbarButton;

typedef void (BmUpdateVariationsFunc)( BmToolbarButton* button);

struct BmVariation {
	BmString label;
	BMessage* msg;
	BmVariation() 							{ msg=NULL; }
	BmVariation( const BmString l, BMessage* m) 
												{ label=l; msg=m; }
};
typedef vector<BmVariation> BmVariationVect;

class BmToolbarButton : public MPictureButton
{
	typedef MPictureButton inherited;
	friend class BmToolbar;

public:
	// creator-func, c'tors and d'tor:
	BmToolbarButton( const char *label, float width, float height, 
						  BMessage *message, BHandler *handler, 
						  const char* tipText=NULL, bool needsLatch=false,
						  const char* resourceName=NULL);
	~BmToolbarButton();
	
	// native methods:
	static void CalcMaxSize( float& width, float& height, const char* label, 
									 bool needsLatch=false);
	void AddActionVariation( const BmString label, BMessage* msg);
	void ShowMenu( BPoint point);
	void SetUpdateVariationsFunc( BmUpdateVariationsFunc* updFunc);
	
	// getters:
	const BmString& Label() const			{ return mLabel; }
	bool NeedsLatch() const					{ return mNeedsLatch; }

	// overrides of Button base:
	void MouseDown( BPoint point);

private:
	bool mHighlighted;
							// intended for mouse-over highlighting, but currently not used
	BmVariationVect mVariations;
							// the different actions that can be started through this button
	bool mNeedsLatch;
	BRect mLatchRect;
	BPoint mMenuPoint;
	BmString mLabel;
	BmString mResourceName;
	
	BmUpdateVariationsFunc* mUpdateVariationsFunc;

	void CreateAllPictures( float width, float height);
	BPicture* CreatePicture( int32 mode, float width, float height);

	// Hide copy-constructor and assignment:
	BmToolbarButton( const BmToolbarButton&);
	BmToolbarButton operator=( const BmToolbarButton&);
};


#endif
