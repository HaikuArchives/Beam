/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 * 	Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmOpenSslNetEndpoint_h
#define _BmOpenSslNetEndpoint_h

#include "BmString.h"

#include "BmNetEndpoint.h"

// avoid inclusion of <openssl/ssl.h> here
struct ssl_st;

class BmOpenSslNetEndpoint : public BmNetEndpoint {
	typedef BmNetEndpoint inherited;
public:
	BmOpenSslNetEndpoint();
	virtual ~BmOpenSslNetEndpoint();

	virtual status_t Connect( const BNetAddress& address);
 	virtual void Close();

	virtual int Error() const;
	virtual BmString ErrorStr() const;

	virtual void SetEncryptionType(const char* encType);
	virtual status_t StartEncryption(const char* encType);
	virtual status_t StopEncryption();
	virtual bool EncryptionIsActive();

	virtual int32 Send( const void* buffer, size_t size, int flags = 0);
	virtual int32 Receive( void* buffer, size_t size, int flags = 0);
	virtual bool IsDataPending( bigtime_t timeout = 0);

	static status_t GetEncryptionInfo(BMessage* encryptionInfo);

private:
	status_t _StartEncryptionAfterConnecting();
	status_t _TranslateErrorCode( int result);

	ssl_st* mSSL;
	BmString mEncryptionType;
	int mError;
};

#endif
