#ifndef MSG_RECEIVER_HPP
#define MSG_RECEIVER_HPP

#include "cstdmf/bw_namespace.hpp"

#include <sys/types.h>


BW_BEGIN_NAMESPACE

class Endpoint;	// Forward declaration

// -----------------------------------------------------------------------------
// Section: MsgReceiver
// -----------------------------------------------------------------------------
/**
 *	This class helps receives a "message" of a specific size.
 */
class MsgReceiver
{
public:
	MsgReceiver( size_t bufSize ) :
		pBuf_( new char[bufSize] ), bufCap_( bufSize ), bufUsed_( 0 ), 
		msgSize_( 0 )
	{}
	~MsgReceiver()
	{
		char* pExisting = reinterpret_cast< char * >(pBuf_);
		delete [] pExisting;
	}

	void setMsgSize( size_t size );
	bool recvMsg( Endpoint& endPoint );

	const void* msg() const	 		{ return pBuf_; }
	size_t msgLen() const 	{ return bufUsed_; }
	size_t capacity() const	{ return bufCap_; }
	bool isDone() const 			{ return bufUsed_ >= msgSize_; }
	bool isAlwaysDone() const		{ return msgSize_ == 0; }

private:
	void reallocBuf( size_t bufSize );

	void*	pBuf_;
	size_t	bufCap_;
	size_t	bufUsed_;
	size_t	msgSize_;
};

BW_END_NAMESPACE

#endif 	// MSG_RECEIVER_HPP
