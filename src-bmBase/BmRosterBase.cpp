/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include "BmRosterBase.h"

BmRosterBase* BeamRoster = 0;

BmGuiRosterBase* BeamGuiRoster = 0;

BLooper* TheJobMetaController = 0;

bool BmRosterBase::IsSupportedEmailMimeType( const BmString& mimetype)
{
	return mimetype.ICompare( "text/x-email")==0 
		|| mimetype.ICompare( "message/rfc822")==0
		|| mimetype.ICompare( "message/rfc822-headers")==0
		|| mimetype.ICompare( "text/rfc822-headers")==0
		|| mimetype.ICompare( "message/delivery-status")==0;
}

BLooper* BmGuiRosterBase::JobMetaController()
{
	return TheJobMetaController;
}
