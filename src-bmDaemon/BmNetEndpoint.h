/*
	BmNetEndpoint.cpp

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

#ifndef _BmNetEndpoint_h
#define _BmNetEndpoint_h

#include <NetAddress.h>

#include "BmDaemon.h"

#include "BmString.h"

class BNetEndpoint;

class IMPEXPBMDAEMON BmNetEndpoint {
	friend class BmNetEndpointRoster;
public:
	virtual ~BmNetEndpoint();

	virtual status_t InitCheck() const;
	virtual int Error() const;
	virtual BmString ErrorStr() const;
	//
	virtual status_t Connect( const BNetAddress& address);
 	virtual void Close();
	//
	virtual void SetEncryptionType(const char* encType);
	virtual status_t StartEncryption(const char* encType);
	virtual status_t StopEncryption();
	virtual bool EncryptionIsActive();
	//
	virtual int32 Send( const void* buffer, size_t size, int flags = 0);
	virtual int32 Receive( void* buffer, size_t size, int flags = 0);
	virtual bool IsDataPending( bigtime_t timeout = 0);
	virtual void SetTimeout(int32 timeout);
 	// static 
protected:
	BmNetEndpoint();

	BNetEndpoint* mSocket;
};

#endif
