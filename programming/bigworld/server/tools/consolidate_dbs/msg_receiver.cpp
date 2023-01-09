#include "msg_receiver.hpp"

#include "network/endpoint.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Sets up this MsgReceiver to receive a message of the specified size.
 * 	When size is 0, we just receive whatever we can from the socket
 * 	(limited by our buffer size) and isDone() is true no matter what.
 */
void MsgReceiver::setMsgSize( size_t size )
{
	// Prevent people from starting a new message when the current one hasn't
	// finished.
	MF_ASSERT( this->isDone() );

	if (size > bufCap_)
	{
		this->reallocBuf( size );
	}
	bufUsed_ = 0;
	msgSize_ = size;
}

/**
 * 	Tries to read from the socket. Returns false if there was an error. When
 * 	we finishes receiving our current message, isDone() will be true.
 */
bool MsgReceiver::recvMsg( Endpoint& endPoint )
{
	bool isOK = true;
	// If msgSize_ is 0, then we just read all that we can from the socket.
	size_t numToRead = (msgSize_ > 0) ? msgSize_ - bufUsed_ : bufCap_;
	int numRead = endPoint.recv( pBuf_, numToRead );
	if (numRead > 0)
	{
		bufUsed_ += numRead;
	}
	else if (numRead == 0)	// Other end has shutdown
	{
		isOK = false;
	}
	else if (numRead == -1)
	{
		if (errno != EAGAIN)
		{
			isOK = false;
		}
	}

	return isOK;
}

/**
 * 	Reallocates our buffer to be the specified size.
 */
void MsgReceiver::reallocBuf( size_t bufSize )
{
	char* pExisting = reinterpret_cast< char * >(pBuf_);
	delete [] pExisting;
	pBuf_ = new char[ bufSize ];
	bufCap_ = bufSize;
}

BW_END_NAMESPACE

// msg_receiver.cpp
