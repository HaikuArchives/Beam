/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _BmNetEndpointRoster_h
#define _BmNetEndpointRoster_h

#include <Locker.h>
#include <Message.h>

#include "BmDaemon.h"

class BmNetEndpoint;

class IMPEXPBMDAEMON BmNetEndpointRoster {
	typedef BmNetEndpoint* (*BmInstantiateNetEndpointFunc)();
	typedef status_t (*BmGetEncryptionInfoFunc)(BMessage*);
public:
	BmNetEndpointRoster();
	~BmNetEndpointRoster();

	BmNetEndpoint* CreateEndpoint();
	//
 	bool SupportsEncryption();
 	bool SupportsEncryptionType(const char* encType);
	status_t GetEncryptionInfo(BMessage* encryptionInfo);
	BmString GetCertPath();
private:
	void _InitializeIfNeeded();
	void _Cleanup();

	bool mNeedInit;
	image_id mAddonImage;
	BmString mAddonName;
	BmInstantiateNetEndpointFunc mAddonInstantiateFunc;
	BmGetEncryptionInfoFunc mAddonGetEncryptionInfoFunc;
	BMessage mEncryptionInfo;
	BLocker mLocker;
};

extern IMPEXPBMDAEMON BmNetEndpointRoster* TheNetEndpointRoster;

#endif
