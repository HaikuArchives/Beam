/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 * 	Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Font.h>
#include <Message.h>
#include <NetEndpoint.h>

#include "BmOpenSslNetEndpoint.h"
#include "BmRosterBase.h"

#include <openssl/err.h>
#include <openssl/pkcs12.h> 
#include <openssl/x509v3.h> 

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static BmString CollectSslErrorString()
{
	BmString errStr;
	uint32 err;
	char errBuf[256];
	while((err = ERR_get_error()) != 0) {
		ERR_error_string(err, errBuf);
		errStr << errBuf << "\n";
	}
	return errStr;
}

// #pragma mark - BmOpenSslNetEndpoint::ContextManager

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmOpenSslNetEndpoint::ContextManager::ContextManager()
	:	mLocker("OpenSslContextManagerLock")
	,	mTlsContext(NULL)
	,	mSslContext(NULL)
	,	mStatus(B_NO_INIT)
{
	if (!SSL_library_init())
		return;
	SSL_load_error_strings();

	// the following is required for pkcs12-stuff:
	PKCS12_PBE_add();

	// create TLS & SSL contexts:
	mTlsContext = SSL_CTX_new(TLSv1_client_method());
	status_t result;
	if ((result = _SetupContext(mTlsContext)) != B_OK) {
		SSL_CTX_free( mTlsContext);
		mTlsContext = NULL;
		mStatus = result;
		return;
	}
	mSslContext = SSL_CTX_new(SSLv3_client_method());
	if ((result = _SetupContext(mSslContext)) != B_OK) {
		SSL_CTX_free( mSslContext);
		mSslContext = NULL;
		mStatus = result;
		return;
	}
	mStatus = B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmOpenSslNetEndpoint::ContextManager::~ContextManager()
{
	if (mTlsContext)
		SSL_CTX_free( mTlsContext);
	if (mSslContext)
		SSL_CTX_free( mSslContext);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmOpenSslNetEndpoint::ContextManager
::SetUserdataForCurrentThread(BmOpenSslNetEndpoint* userdata)
{
	if (mLocker.Lock()) {
		mUserdataMap[find_thread(NULL)] = userdata;
		mLocker.Unlock();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmOpenSslNetEndpoint* BmOpenSslNetEndpoint::ContextManager
::GetUserdataForCurrentThread()
{
	BmOpenSslNetEndpoint* userdata = NULL;
	if (mLocker.Lock()) {
		UserdataMap::iterator pos = mUserdataMap.find(find_thread(NULL));
		if (pos != mUserdataMap.end())
			userdata = pos->second;
		mLocker.Unlock();
	}
	return userdata;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmOpenSslNetEndpoint::ContextManager::RemoveUserdataForCurrentThread()
{
	if (mLocker.Lock()) {
		mUserdataMap.erase(find_thread(NULL));
		mLocker.Unlock();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::ContextManager::_SetupContext(SSL_CTX* context)
{
	if (!context)
		return B_ERROR;
	if (SSL_CTX_set_default_verify_paths(context) != 1) {
		mErrorStr = BmString("couldn't set default verify paths\n")
							<< CollectSslErrorString();
		return B_ERROR;
	}
	const char* cipherList = "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH";
		// as in  "Network Security with OpenSSL", page 146f
	if (SSL_CTX_set_cipher_list(context, cipherList) != 1) {
		mErrorStr = BmString("couldn't set default verify paths\n")
							<< CollectSslErrorString();
		return B_ERROR;
	}

	SSL_CTX_set_client_cert_cb(context, 
										BmOpenSslNetEndpoint::ClientCertCallback);

	SSL_CTX_set_verify(context, SSL_VERIFY_PEER,
							 BmOpenSslNetEndpoint::VerifyCallback);

	return B_OK;
}

// #pragma mark - BmOpenSslNetEndpoint

BmOpenSslNetEndpoint::ContextManager BmOpenSslNetEndpoint::nContextManager;

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
	if (mSSL)
		SSL_free( mSSL);

	// cleanup error queue of current thread:
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
	return error;
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
	BmString errStr;
	if (mVerificationError.Length())
		errStr << mVerificationError << "\n";
	if (mError == 0) 
		errStr << inherited::ErrorStr();
	else if (mError >= 2 && mError <= 4) {
		const char* ssl_errs[] = {
			"Want Read",
			"Want Write",
			"Want X509 lookup, but don't know how to do that!"
		};
		errStr << ssl_errs[mError-2];
	} else {
		errStr << CollectSslErrorString();
	}
	return errStr;
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

	// fetch context corresponding to requested type of connection
	SSL_CTX* context;
	if (mEncryptionType == "TLS")
		context = nContextManager.TlsContext();
	else if (mEncryptionType == "SSL")
		context = nContextManager.SslContext();
	else
		return B_BAD_VALUE;

	// create SSL structure
	mSSL = SSL_new( context);
	if (!mSSL)
		return B_ERROR;

	// set ourselves as userdata for the new ssl object:
	nContextManager.SetUserdataForCurrentThread(this);

	int result;
	// set SSL file descriptor
	if ((result = SSL_set_fd( mSSL, socket)) != 1) {
		mError = SSL_get_error( mSSL, result);
		return B_ERROR;
	}

	// connect
	_ClearErrorState();
	if ((result = SSL_connect( mSSL)) != 1) {
		mError = SSL_get_error( mSSL, result);
		return B_ERROR;
	}
	
	if ((result = _PostHandshakeCheck()) != X509_V_OK) {
		mError = result;
		return B_ERROR;
	}

	return B_OK;
}

/*------------------------------------------------------------------------------*\
	StopEncryption()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::StopEncryption() {
	if (!mSSL)
		return B_BAD_VALUE;

	nContextManager.RemoveUserdataForCurrentThread();

	// shutdown connection
	if (mError == 0) {
		if (SSL_shutdown(mSSL) == 0)
			SSL_shutdown(mSSL);	// as per documentation (bidirectional shutdown)
	} else
		SSL_clear(mSSL);

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
bool BmOpenSslNetEndpoint::IsDataPending( bigtime_t timeout) 
{
	// TODO: We might actually need to do something here. Data pending on
	// the socket doesn't mean that we could also read something from the
	// SSL connection.
	return inherited::IsDataPending( timeout);
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
	encryptionInfo->AddString("cert-path", X509_get_default_cert_dir());
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	ClientCertCallback()
		-	
\*------------------------------------------------------------------------------*/
int BmOpenSslNetEndpoint
::ClientCertCallback(SSL* ssl, X509 **certP, EVP_PKEY **pkeyP)
{
	BmOpenSslNetEndpoint* endpoint 
		= nContextManager.GetUserdataForCurrentThread();
	if (endpoint) {
		if (endpoint->_FetchClientCertificateAndKey(ssl, certP, pkeyP) == B_OK)
			return 1;
	}
	return 0;
}

/*------------------------------------------------------------------------------*\
	VerifyCallback()
		-	
\*------------------------------------------------------------------------------*/
int BmOpenSslNetEndpoint::VerifyCallback(int ok, X509_STORE_CTX *store)
{
	if (!ok) {
		BmOpenSslNetEndpoint* endpoint 
			= nContextManager.GetUserdataForCurrentThread();
		if (endpoint)
			ok = endpoint->_FetchVerificationError(store);
	}
	return ok;
}

/*------------------------------------------------------------------------------*\
	_FetchClientCertificateAndKey()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint
::_FetchClientCertificateAndKey(SSL* ssl, X509 **certP, EVP_PKEY **pkeyP)
{
	if (!ssl || !certP  || !pkeyP)
		return B_BAD_VALUE;
	BmString name = mAdditionalInfo.FindString(MSG_CLIENT_CERT_NAME);
	if (!name.Length())
		return 0;
	BmString certDir = X509_get_default_cert_dir();
	BmString certFile = certDir + "/" + name;

	BIO* p12bio = NULL;
	PKCS12* pkcs12 = NULL;
	int result = 0;
	int errLib = 0;
	int errReason = 0;
	bool stop = false;
	while(!stop) {
		BmString pwd;
		BmString text 
			= BmString("Please give passphrase for client certificate '")
								<< name << "':";
		if (!BeamGuiRoster->AskUserForPwd(text, pwd)) {
			mStopRequested = true;
			return B_INTERRUPTED;
		}

		if ((p12bio = BIO_new_file(certFile.String(), "rb")) != NULL) {
			if ((pkcs12 = d2i_PKCS12_bio(p12bio, NULL)) != NULL) {
				result = PKCS12_parse(pkcs12, pwd.String(), pkeyP, certP, NULL);
			}
		}
		if (result != 1) {
			uint32 err = ERR_peek_error();
			errLib = ERR_GET_LIB(err);
			errReason = ERR_GET_REASON(err);
			if (errLib == ERR_LIB_PKCS12 && errReason == SSL_R_BAD_MAC_DECODE) {
				// wrong passphrase was given, but before we can try again,
				// we need to remove that error from the error queue:
				ERR_get_error();
			} else {
				mError = err;
				stop = true;
			}
		} else
			stop = true;
	}
	if (pkcs12);
		PKCS12_free(pkcs12);
	if (p12bio)
		BIO_free(p12bio);
	return result == 1 ? B_OK : B_ERROR;
}

/*------------------------------------------------------------------------------*\
	_FetchVerificationError()
		-	
\*------------------------------------------------------------------------------*/
int BmOpenSslNetEndpoint::_FetchVerificationError(X509_STORE_CTX *store)
{
	int err = X509_STORE_CTX_get_error(store);
	mVerificationError
		<< "   " << err << " - " << X509_verify_cert_error_string(err) << "\n";
	// in order to delegate error handling to _PostHandshakeCheck(), 
	// we always pretend that the certificate is ok:
	return 1;
}

/*------------------------------------------------------------------------------*\
	_FingerprintForCert()
		-	
\*------------------------------------------------------------------------------*/
BmString BmOpenSslNetEndpoint::_FingerprintForCert(X509* cert)
{
	unsigned int n;
	unsigned char md[EVP_MAX_MD_SIZE];
	
	if (!X509_digest(cert, EVP_md5(), md, &n))
		return "";

	BmString fp;
	char* buf = fp.LockBuffer(n*3);
	if (!buf)
		return "";
	for (unsigned int j=0; j < n; ++j) {
  		unsigned char c = md[j];
		*buf++ = (c > 0x9F ? 'A'-10 : '0')+(c>>4);
		*buf++ = ((c&0x0F) > 9 ? 'A'-10 : '0')+(c&0x0F);
		if (j+1 < n)
			*buf++ = ':';
	}
	*buf = '\0';
	fp.UnlockBuffer();
	return fp;
}

/*------------------------------------------------------------------------------*\
	_MatchHostname()
		-	
\*------------------------------------------------------------------------------*/
bool BmOpenSslNetEndpoint::_MatchHostname(const BmString& hostname,
														const BmString& pattern)
{
	if (hostname.ICompare(pattern) == 0)
		return true;
	BmString hPart, pPart;
	int32 hsPos = 0;
	int32 hePos = hostname.Length();
	int32 psPos = 0;
	int32 pePos = pattern.Length();
	bool stop = false;
	while(!stop) {
		if (hePos >= 0) {
			hsPos = hostname.FindLast('.', hePos-1);
			if (hsPos >= 0) {
				hPart.SetTo(hostname.String()+hsPos+1, hePos-(hsPos+1));
				hePos = hsPos;
			} else {
				hPart.SetTo(hostname.String(), hePos);
				stop = true;
			}
		} else {
			hPart.SetTo("");
			stop = true;
		}
		if (pePos >= 0) {
			psPos = pattern.FindLast('.', pePos-1);
			if (psPos >= 0) {
				pPart.SetTo(pattern.String()+psPos+1, pePos-(psPos+1));
				pePos = psPos;
			} else {
				pPart.SetTo(pattern.String(), pePos);
				stop = true;
			}
		} else {
			pPart.SetTo("");
			stop = true;
		}
		int32 asteriskPos = pPart.FindFirst('*');
		int32 matchLen = asteriskPos<0 ? pPart.Length() : asteriskPos;
		if (hPart.ICompare(pPart, matchLen) != 0)
			return false;
	}
	return hsPos < 0 && psPos < 0;
}

/*------------------------------------------------------------------------------*\
	_VerifyHostname()
		-	searches the given certificate for any hostname specification:
			+ highest priority has any DNS-value in a subjectAltName extension
			+ if no match is found, commonName is tried.
		-	implementation ripped from cURL (ssluse.c) [thanks, Daniel et al.!]
\*------------------------------------------------------------------------------*/
bool BmOpenSslNetEndpoint::_VerifyHostname(X509* cert, const BmString& hostname,
														 BmString& namesFoundInCert)
{
	namesFoundInCert.Truncate(0);
	if (!cert)
		return "";
	bool matched = false;

	/* get a "list" of alternative names */
	STACK_OF(GENERAL_NAME) *altNames;
	altNames = static_cast<STACK_OF(GENERAL_NAME)*>(
		X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL)
	);
	if (altNames) {
		/* get amount of alternatives, RFC2459 claims there MUST be at least
		   one, but we don't depend on it... */
		int numAlts = sk_GENERAL_NAME_num(altNames);

		/* loop through all alternatives while none has matched */
		for(int i=0; i<numAlts && !matched; i++) {
			/* get a handle to alternative name number i */
			const GENERAL_NAME *check = sk_GENERAL_NAME_value(altNames, i);

			/* only check alternatives of the same type the target is */
			if (check->type == GEN_DNS) {
				/* get data and length */
				const char *altPtr = (char *)ASN1_STRING_data(check->d.ia5);
				/* The OpenSSL man page explicitly says: "In general it cannot be
			     assumed that the data returned by ASN1_STRING_data() is null
			     terminated or does not contain embedded nulls." But also that
			     "The actual format of the data will depend on the actual string
			     type itself: for example for and IA5String the data will be ASCII"
			
			     Gisle researched the OpenSSL sources:
			     "I checked the 0.9.6 and 0.9.8 sources before my patch and
			     it always 0-terminates an IA5String."
				*/
				if (_MatchHostname(hostname, altPtr))
					matched = true;
				namesFoundInCert 
					<< (namesFoundInCert.Length() ? ", " : "")
					<< altPtr;
			}
		}
		GENERAL_NAMES_free(altNames);
	} 
	if (!matched) {
		X509_NAME* subj = X509_get_subject_name(cert);
		if (subj) {
			char data[256];
			int len = X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256);
			if (len > -1) {
				BmString commonName(data, len);
				if (_MatchHostname(hostname, commonName))
					matched = true;
				namesFoundInCert 
					<< (namesFoundInCert.Length() ? ", " : "")
					<< commonName;
			}
		}
	}
	return matched;
}

/*------------------------------------------------------------------------------*\
	_CertAsString()
		-	
\*------------------------------------------------------------------------------*/
BmString BmOpenSslNetEndpoint::_CertAsString(X509* cert)
{
	char data[256];
	BmString str;
	int len;
	str << "Issued to:\n";
	X509_NAME* subj = X509_get_subject_name(cert);
	if (!subj)
		str << "<subject not found>";
	else {
		len = X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256);
		if (len > -1)
			str << "   Common Name:   " << BmString(data, len) << "\n";
		len = X509_NAME_get_text_by_NID(subj, NID_organizationName, data, 256);
		if (len > -1)
			str << "   Organization:   " << BmString(data, len) << "\n";
		len = X509_NAME_get_text_by_NID(subj, NID_organizationalUnitName, data, 256);
		if (len > -1)
			str << "   Organizational Unit:   " << BmString(data, len) << "\n";
		len = X509_NAME_get_text_by_NID(subj, NID_serialNumber, data, 256);
		if (len > -1)
			str << "   Serial Number:   " << BmString(data, len) << "\n";
	}

	str << "by:\n";
	X509_NAME* iss = X509_get_issuer_name(cert);
	if (!iss)
		str << "<issuer not found>\n";
	else {
		len = X509_NAME_get_text_by_NID(iss, NID_commonName, data, 256);
		if (len > -1)
			str << "   Common Name:   " << BmString(data, len) << "\n";
		len = X509_NAME_get_text_by_NID(iss, NID_organizationName, data, 256);
		if (len > -1)
			str << "   Organization:   " << BmString(data, len) << "\n";
		len = X509_NAME_get_text_by_NID(iss, NID_organizationalUnitName, data, 256);
		if (len > -1)
			str << "   Organizational Unit:   " << BmString(data, len) << "\n";
	}
	return str;
}

/*------------------------------------------------------------------------------*\
	_ClearErrorState()
		-	
\*------------------------------------------------------------------------------*/
void BmOpenSslNetEndpoint::_ClearErrorState()
{
	mVerificationError.Truncate(0);
	mError = 0;
}

/*------------------------------------------------------------------------------*\
	_PostHandshakeCheck()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::_PostHandshakeCheck()
{
	BmString serverName = mAdditionalInfo.FindString(MSG_SERVER_NAME);
	if (!serverName.Length())
		return B_BAD_VALUE;
	X509* cert = SSL_get_peer_certificate(mSSL);
	status_t result = X509_V_ERR_APPLICATION_VERIFICATION;
	if (cert) {
		// since the user can setup Beam such that it blindly accepts a server
		// certificate given a specific id, we need to be able to *uniquely* 
		// identify each and every server certificate. In order to get such an
		// id, we compute the certificate fingerprint and use it for that purpose:
		BmString certID = _FingerprintForCert(cert);
		BmString acceptedCertID
			= mAdditionalInfo.FindString(MSG_ACCEPTED_CERT_ID);
		if (certID == acceptedCertID)
			return X509_V_OK;
		
		result = SSL_get_verify_result(mSSL);
		BmString namesFoundInCert;
		bool hostVerified = _VerifyHostname(cert, serverName, namesFoundInCert);
		X509_free(cert);
		if (!hostVerified) {
			mVerificationError 
				<< "   The given hostname\n"
				<< "      " << serverName << "\n"
				<< "   doesn't match any of the names found in the certificate:\n"
				<< "      " << namesFoundInCert << "\n";
		}
		if (mVerificationError.Length()) {
			// if any error occured, be it during openssl-internal verification
			// or our own, we create an intro for the verification problem
			// message...
			BmString boundary;
			float count = 296.0 / be_plain_font->StringWidth("-");
				// hack to avoid wrapping of boundary
			boundary.SetTo('-', count);
			BmString problemMsg;
			problemMsg
				<< "The server-certificate received from " << serverName << "\n"
				<< "could not be validated properly!\n"
				<< boundary << "\n"
				<< "Certificate Info\n"
				<< "   MD5-Fingerprint:   " << certID << "\n"
				<< _CertAsString(cert)
				<< boundary << "\n"
				<< "During validation, these problems occured:\n"
				<< mVerificationError
				<< boundary << "\n"
				<< "Would you still like to accept this certificate and proceed?";
			// ...and tell the user and ask if we should go on:
			int32 choice = BeamGuiRoster->ShowAlert(problemMsg, 
																 "Cancel", 
																 "Accept Permanently", 
																 "Accept");
			result = X509_V_OK;
			if (choice == 1) {
				// set new accepted certID such that it gets stored
				NewAcceptedCertID(certID);
			} else if (choice == 0) {
				mStopRequested = true;
				result = X509_V_ERR_APPLICATION_VERIFICATION;
			}
			_ClearErrorState();
		}
	}
	return result;
}

/*------------------------------------------------------------------------------*\
	_TranslateErrorCode()
		-	
\*------------------------------------------------------------------------------*/
status_t BmOpenSslNetEndpoint::_TranslateErrorCode( int result)
{
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

