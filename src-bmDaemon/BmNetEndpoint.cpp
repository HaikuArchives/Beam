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

#include <NetEndpoint.h>

#include "BmNetEndpoint.h"

//*****************************************************************************
// #pragma mark - BmNetEndpoint
//*****************************************************************************

/*------------------------------------------------------------------------------*\
	BmNetEndpoint()
		-	constructor
\*------------------------------------------------------------------------------*/
BmNetEndpoint::BmNetEndpoint()
	:	mSocket( new BNetEndpoint( SOCK_STREAM))
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
void BmNetEndpoint::SetEncryptionType(const char* encType)
{
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

