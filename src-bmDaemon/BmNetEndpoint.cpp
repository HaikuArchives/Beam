/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "BmNetEndpoint.h"

//*****************************************************************************
// #pragma mark - BmNetEndpoint
//*****************************************************************************

const char* const BmNetEndpoint::MSG_CLIENT_CERT_NAME = 	"bm:clcrtnm";
const char* const BmNetEndpoint::MSG_SERVER_NAME = 		"bm:servnm";
/*------------------------------------------------------------------------------*\
	BmNetEndpoint()
		-	constructor
\*------------------------------------------------------------------------------*/
BmNetEndpoint::BmNetEndpoint()
	:	mSocket( new BNetEndpoint( SOCK_STREAM))
	,	mStopRequested( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmNetEndpoint()
		-	destructor
\*------------------------------------------------------------------------------*/
BmNetEndpoint::~BmNetEndpoint() 
{
	Close();
	delete mSocket;
}

/*------------------------------------------------------------------------------*\
	InitCheck()
		-	
\*------------------------------------------------------------------------------*/
status_t BmNetEndpoint::InitCheck() const
{
	return mSocket->InitCheck();
}

/*------------------------------------------------------------------------------*\
	Error()
		-	
\*------------------------------------------------------------------------------*/
int BmNetEndpoint::Error() const
{
	return mSocket->Error();
}

/*------------------------------------------------------------------------------*\
	ErrorStr()
		-	
\*------------------------------------------------------------------------------*/
BmString BmNetEndpoint::ErrorStr() const
{
	return mSocket->ErrorStr();
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	
\*------------------------------------------------------------------------------*/
status_t BmNetEndpoint::Connect( const BNetAddress& address) 
{
	return mSocket->Connect( address);
}

/*------------------------------------------------------------------------------*\
	Close()
		-	
\*------------------------------------------------------------------------------*/
void BmNetEndpoint::Close() 
{
	mSocket->Close();
}

/*------------------------------------------------------------------------------*\
	StartEncryption()
		-	
\*------------------------------------------------------------------------------*/
status_t BmNetEndpoint::StartEncryption(const char* encType) 
{
	return B_ERROR;
}

/*------------------------------------------------------------------------------*\
	StopEncryption()
		-	
\*------------------------------------------------------------------------------*/
status_t BmNetEndpoint::StopEncryption() 
{
	return B_ERROR;
}

/*------------------------------------------------------------------------------*\
	EncryptionIsActive()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetEndpoint::EncryptionIsActive() 
{
	return false;
}

/*------------------------------------------------------------------------------*\
	SetAdditionalInfo()
		-	
\*------------------------------------------------------------------------------*/
void BmNetEndpoint::SetAdditionalInfo(const BMessage* msg)
{
	if (msg)
		mAdditionalInfo = *msg;
}

/*------------------------------------------------------------------------------*\
	Send()
		-	
\*------------------------------------------------------------------------------*/
int32 BmNetEndpoint::Send( const void* buffer, size_t size, int flags) 
{
	return mSocket->Send(buffer, size, flags);
}

/*------------------------------------------------------------------------------*\
	Receive()
		-	
\*------------------------------------------------------------------------------*/
int32 BmNetEndpoint::Receive( void* buffer, size_t size, int flags) 
{
	return mSocket->Receive(buffer, size, flags);
}

/*------------------------------------------------------------------------------*\
	IsDataPending()
		-	
\*------------------------------------------------------------------------------*/
bool BmNetEndpoint::IsDataPending( bigtime_t timeout) 
{
	return mSocket->IsDataPending( timeout);
}

/*------------------------------------------------------------------------------*\
	SetTimeout()
		-	
\*------------------------------------------------------------------------------*/
void BmNetEndpoint::SetTimeout(int32 timeout)
{
	mSocket->SetTimeout(timeout);
}

