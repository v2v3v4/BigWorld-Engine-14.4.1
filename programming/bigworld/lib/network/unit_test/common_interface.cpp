#include "pch.hpp"

#include "common_interface.hpp"

#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class CommonStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (CommonHandler::*Handler)(
			const Mercury::Address & srcAddr,
			const ARGS & args );

	CommonStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		void * pData = header.pInterface->pExtensionData();

		// Make sure the pExtensionData has been set on the NetworkInterface
		// to the local handler.
		MF_ASSERT( pData );

		CommonHandler * pHandler = static_cast<CommonHandler *>( pData );
		(pHandler->*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};

/**
 *	Class for stream-style Mercury message handler objects.
 */
class CommonStreamMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (CommonHandler::*Handler)(
			const Mercury::Address & srcAddr,
			BinaryIStream & data );

	CommonStreamMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		void * pData = header.pInterface->pExtensionData();

		// Make sure the pExtensionData has been set on the NetworkInterface
		// to the local handler.
		MF_ASSERT( pData );

		CommonHandler * pHandler = static_cast<CommonHandler *>( pData );
		(pHandler->*handler_)( srcAddr, data );
	}

	Handler handler_;
};

BW_END_NAMESPACE


#define DEFINE_SERVER_HERE
#include "common_interface.hpp"

// common_interface.cpp
