/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
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
