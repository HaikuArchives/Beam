/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefViewFilterJob_h
#define _BmMailRefViewFilterJob_h

#include "BmListController.h"
#include "BmMailRefView.h"

/*------------------------------------------------------------------------------*\
	BmRefItemFilter
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefItemFilter : public BmViewItemFilter
{
public:
	BmMailRefItemFilter(const BmString& filterKind, const BmString& filterText);
	virtual ~BmMailRefItemFilter();
	
	// overrides of base
	virtual bool Matches(const BmListViewItem* viewItem) const;

	static const char* const FILTER_SUBJECT_OR_ADDRESS;
	static const char* const FILTER_MAILTEXT;

private:
	BmString mFilterKind;
	BmString mFilterText;
};

/*------------------------------------------------------------------------------*\
	BmMailRefViewFilter
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefViewFilterJob : public BmJobModel {
	typedef BmJobModel inherited;

	typedef vector< BmRef< BmMail> > BmMailVect;
	typedef vector< const char**> BmHeaderVect;
	
public:
	BmMailRefViewFilterJob(BmViewItemFilter* filter, BmMailRefView* mailRefView);
	virtual ~BmMailRefViewFilterJob();

	// overrides of BmJobModel base:
	bool StartJob();

private:
	BmViewItemFilter* mFilter;
	BmMailRefView* mMailRefView;

	// Hide copy-constructor and assignment:
	BmMailRefViewFilterJob( const BmMailRefViewFilterJob&);
	BmMailRefViewFilterJob operator=( const BmMailRefViewFilterJob&);
};

#endif
