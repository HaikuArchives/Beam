/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmNetEndpoint_h
#define _BmNetEndpoint_h

#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif
#include <Message.h>
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
	virtual status_t StartEncryption(const char* encType);
	virtual status_t StopEncryption();
	virtual bool EncryptionIsActive();
	//
	virtual void SetAdditionalInfo(const BMessage* msg);
	//
	virtual int32 Send( const void* buffer, size_t size, int flags = 0);
	virtual int32 Receive( void* buffer, size_t size, int flags = 0);
	virtual bool IsDataPending( bigtime_t timeout = 0);
	virtual void SetTimeout(int32 timeout);

	inline bool IsStopRequested()			{ return mStopRequested; }

	// message component definitions for additional info:
	static const char* const MSG_CLIENT_CERT_NAME;
	static const char* const MSG_SERVER_NAME;
protected:
	BmNetEndpoint();

	BNetEndpoint* mSocket;
	BMessage mAdditionalInfo;
	bool mStopRequested;
};

#endif
