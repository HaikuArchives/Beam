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

#include <openssl/ssl.h>

class BmOpenSslNetEndpoint : public BmNetEndpoint {
	typedef BmNetEndpoint inherited;
	
public:
	BmOpenSslNetEndpoint();
	virtual ~BmOpenSslNetEndpoint();

	virtual status_t Connect( const BNetAddress& address);
 	virtual void Close();

	virtual int Error() const;
	virtual BmString ErrorStr() const;

	virtual status_t StartEncryption(const char* encType);
	virtual status_t StopEncryption();
	virtual bool EncryptionIsActive();

	virtual int32 Send( const void* buffer, size_t size, int flags = 0);
	virtual int32 Receive( void* buffer, size_t size, int flags = 0);
	virtual bool IsDataPending( bigtime_t timeout = 0);

	static status_t GetEncryptionInfo(BMessage* encryptionInfo);

	static int ClientCertCallback(SSL* ssl, X509 **certP, EVP_PKEY **pkeyP);
	static int VerifyCallback(int ok, X509_STORE_CTX *store);

	static void LockingCallback(int mode, int type, const char *file, int line);
	static unsigned long ThreadIdCallback(void);

private:
	status_t _StartEncryptionAfterConnecting();
	status_t _TranslateErrorCode( int result);
	status_t _FetchClientCertificateAndKey(SSL* ssl, X509 **x509, 
														EVP_PKEY **pkey);
	BmString _FingerprintForCert(X509* cert);
	int _FetchVerificationError(X509_STORE_CTX *store);
	bool _MatchHostname(const BmString& hostname, const BmString& pattern);
	bool _VerifyHostname(X509* cert, const BmString& hostname, 
								BmString& namesFoundInCert);
	BmString _CertAsString(X509* cert);
	void _ClearErrorState();
	status_t _PostHandshakeCheck();

	SSL* mSSL;
	BmString mEncryptionType;

	int mError;
	BmString mVerificationError;

	class ContextManager {
		typedef std::map<thread_id, BmOpenSslNetEndpoint*> UserdataMap;
	public:
		ContextManager();
		~ContextManager();
	
		SSL_CTX* TlsContext()					{ return mTlsContext; }
		SSL_CTX* SslContext()					{ return mSslContext; }
	
		void SetUserdataForCurrentThread(BmOpenSslNetEndpoint* userdata);
		BmOpenSslNetEndpoint* GetUserdataForCurrentThread();
		void RemoveUserdataForCurrentThread();
		void LockingCallback(int mode, int type, const char *file, int line);
		unsigned long ThreadIdCallback(void);
	
	private:
		status_t _SetupContext(SSL_CTX* context);
		void _SetupSslLocks();
		void _CleanupSslLocks();
	
		BLocker mLocker;
		SSL_CTX* mTlsContext;
		SSL_CTX* mSslContext;
		status_t mStatus;
		BmString mErrorStr;
		UserdataMap mUserdataMap;
		std::vector<BLocker*> mSslLocks;
	};
	static ContextManager nContextManager;
};

#endif
