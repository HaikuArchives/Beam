/*
 * Copyright 2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailNavigator_h
#define _BmMailNavigator_h

class BmMailRef;
class BmMailRefView;
class BmMailView;

struct BmMailRefSelector {
	virtual ~BmMailRefSelector()			{}
	virtual bool operator() (BmMailRef* mailRef) const;
};

struct BmNewMailRefSelector : public BmMailRefSelector {
	virtual ~BmNewMailRefSelector()		{}
	virtual bool operator() (BmMailRef* mailRef) const;
};

class BmMailNavigator {
public:
	BmMailNavigator(BmMailRefView* refView, 
						 BmMailView* mailView,
						 const BmMailRefSelector& selector);
	void MoveBackward();
	void MoveForward();
private:
	bool _MoveMailView();
	bool _MoveRefView(bool backward);
	BmMailRefView* mRefView;
	BmMailView* mMailView;
	const BmMailRefSelector& mMailRefSelector;
};

#endif
