/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 * 	Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Message.h>
#include <NetEndpoint.h>

#include "BmOpenSslNetEndpoint.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

// Automatically initialize libopenssl.
struct _InitOpenSSL {
	_InitOpenSSL()
	{
		SSL_library_init();
		SSL_load_error_strings();
	}
};
static _InitOpenSSL sInitOpenSSL;

/*------------------------------------------------------------------------------*\
	BmOpenSslNetEndpoint()
		-	constructor
\*------------------------------------------------------------------------------*/
BmOpenSslNetEndpoint::BmOpenSslNetEndpoint()
	:	mSSL(NULL)
	,	mError(0)
{
}

/*------------------------------------------------------------------------------*\
	~BmOpenSslNetEndpoint()
		-	destructor
\*------------------------------------------------------------------------------*/
BmOpenSslNetEndpoint::~BmOpenSslNetEndpoint() 
{
	// cleanup error queue from current thread's TLS:
	ERR_remove_state(0);
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::Connect( const BNetAddress& address) {
	// already connected?
	if (mSSL)
		return B_BAD_VALUE;

	// establish the underlying socket connection
	status_t error = inherited::Connect( address);
	if (error != B_OK)
		return error;

	return _StartEncryptionAfterConnecting();
}

/*------------------------------------------------------------------------------*\
	Close()
		-	
\*------------------------------------------------------------------------------*/
void BmOpenSslNetEndpoint::Close() {
	StopEncryption();
	inherited::Close();
}

/*------------------------------------------------------------------------------*\
	Error()
		-	
\*------------------------------------------------------------------------------*/
int BmOpenSslNetEndpoint::Error() const
{
	if (mError != 0)
		return mError;
	else
		return inherited::Error();
}

/*------------------------------------------------------------------------------*\
	ErrorStr()
		-	
\*------------------------------------------------------------------------------*/
BmString BmOpenSslNetEndpoint::ErrorStr() const
{
	if (mError != 0) {
		BmString errStr;
		uint32 err;
		char errBuf[128];
		while((err = ERR_get_error()) != 0) {
			ERR_error_string(err, errBuf);
			errStr << errBuf << "\n";
		}
		return errStr;
	} else
		return inherited::ErrorStr();
}

/*------------------------------------------------------------------------------*\
	StartEncryption()
		-	
\*------------------------------------------------------------------------------*/
void BmOpenSslNetEndpoint::SetEncryptionType(const char* encType)
{
	mEncryptionType = encType;
}

/*------------------------------------------------------------------------------*\
	StartEncryption()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::StartEncryption(const char* encType) {
	// encryption already on?
	if (mSSL)
		return B_NOT_ALLOWED;

	// connected?
	int socket = mSocket->Socket();
	if (socket < 0)
		return ENOTCONN;

	if (mEncryptionType != encType)
		mEncryptionType = encType;

	// check type
	SSL_METHOD* method;
	if (mEncryptionType == "TLS")
		method = TLSv1_client_method();
	else if (mEncryptionType == "SSL")
		method = SSLv3_client_method();
	else
		return B_BAD_VALUE;

	// create SSL context
	SSL_CTX* context = SSL_CTX_new( method);
	if (!context)
		return B_ERROR;

	// create SSL structure
	ssl_st* ssl = SSL_new( context);
	if (!ssl) {
		SSL_CTX_free( context);
		return B_ERROR;
	}

	// free the context (decrement its ref count); the SSL structure holds a
	// reference now
	SSL_CTX_free( context);

	// set SSL file descriptor
	if (SSL_set_fd( ssl, socket) != 1) {
		SSL_free( ssl);
		return B_ERROR;
	}

	// connect
	int result = SSL_connect( ssl);
	if (result != 1) {
		mError = SSL_get_error( mSSL, result);
		SSL_free( ssl);
		return B_ERROR;
	}

	// Everything went fine; keep the SSL structure.
	mSSL = ssl;

	return B_OK;
}

/*------------------------------------------------------------------------------*\
	StopEncryption()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::StopEncryption() {
	if (!mSSL)
		return B_BAD_VALUE;

	// shutdown connection
	if (SSL_shutdown( mSSL) == 0)
		SSL_shutdown( mSSL);	// as per documentation (bidirectional shutdown)

	// free SSL structure
	SSL_free( mSSL);
	mSSL = NULL;

	return B_OK;
}

/*------------------------------------------------------------------------------*\
	EncryptionIsActive()
		-	
\*------------------------------------------------------------------------------*/
bool BmOpenSslNetEndpoint::EncryptionIsActive() 
{
	return mSSL != NULL;
}

/*------------------------------------------------------------------------------*\
	Send()
		-	
\*------------------------------------------------------------------------------*/
int32 BmOpenSslNetEndpoint::Send( const void* buffer, size_t size, int flags) {
	if (!mSSL)
		return inherited::Send(buffer, size, flags);

	if (!buffer)
		return B_BAD_VALUE;

	int result = SSL_write( mSSL, buffer, size);
	if (result > 0)
		return result;

	// translate the error
	return _TranslateErrorCode( result);
}

/*------------------------------------------------------------------------------*\
	Receive()
		-	
\*------------------------------------------------------------------------------*/
int32 BmOpenSslNetEndpoint::Receive( void* buffer, size_t size, int flags) {
	if (!mSSL)
		return inherited::Receive(buffer, size, flags);

	if (!buffer)
		return B_BAD_VALUE;

	int result = SSL_read( mSSL, buffer, size);
	if (result > 0)
		return result;

	// translate the error
	return _TranslateErrorCode(result);
}

/*------------------------------------------------------------------------------*\
	IsDataPending()
		-	
\*------------------------------------------------------------------------------*/
bool BmOpenSslNetEndpoint::IsDataPending( bigtime_t timeout) {
	// TODO: We might actually need to do something here. Data pending on
	// the socket doesn't mean that we could also read something from the
	// SSL connection.
	return inherited::IsDataPending( timeout);
}

/*------------------------------------------------------------------------------*\
	_StartEncryptionAfterConnecting()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::_StartEncryptionAfterConnecting() {
	if (mSSL)
		return B_NOT_ALLOWED;

	status_t error = B_OK;
	if (mEncryptionType.Length()) {
		error = StartEncryption( mEncryptionType.String());
		if (error != B_OK)
			inherited::Close();
	}
	return error;
}

/*------------------------------------------------------------------------------*\
	_TranslateErrorCode()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::_TranslateErrorCode( int result) {
	if (!mSSL)
		return B_NO_INIT;

	if (result > 0)
		return B_OK;

	switch (SSL_get_error( mSSL, result)) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
			return B_WOULD_BLOCK;

		default:
			return B_ERROR;
	}

	// TODO: Set errno? Will that make Error() return the value we set here?
}

/*------------------------------------------------------------------------------*\
	GetEncryptionInfo()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::GetEncryptionInfo(BMessage* encryptionInfo)
{
	if (!encryptionInfo)
		return B_BAD_VALUE;
	encryptionInfo->MakeEmpty();
	encryptionInfo->AddBool("supports-encryption", true);
	encryptionInfo->AddString("type", "TLS");
	encryptionInfo->AddString("type", "SSL");
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	InstantiateNetEndpoint()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport)
BmNetEndpoint* InstantiateNetEndpoint()
{
	return new BmOpenSslNetEndpoint();
}

/*------------------------------------------------------------------------------*\
	GetEncryptionInfo()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport)
status_t GetEncryptionInfo(BMessage* encryptionInfo)
{
	return BmOpenSslNetEndpoint::GetEncryptionInfo(encryptionInfo);
}

