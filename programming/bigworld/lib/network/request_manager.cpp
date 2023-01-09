#include "pch.hpp"

#include "request_manager.hpp"

#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "nub_exception.hpp"
#include "request.hpp"
#include "reply_order.hpp"
#include "unpacked_message_header.hpp"

#include "cstdmf/timestamp.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
RequestManager::RequestManager( bool isExternal,
		EventDispatcher & dispatcher ) :
	requestMap_(),
	nextReplyID_( (uint32(timestamp())%100000) + 10101 ),
	dispatcher_( dispatcher ),
	isExternal_( isExternal )
{
}


/**
 *	Destructor.
 */
RequestManager::~RequestManager()
{
	if (!requestMap_.empty())
	{
		INFO_MSG( "RequestManager::~RequestManager: "
					"Num pending reply handlers = %" PRIzu "\n",
				requestMap_.size() );

		RequestMap::iterator iter = requestMap_.begin();

		while (iter != requestMap_.end())
		{
			Request * pElement = iter->second;
			++iter;
			pElement->handleFailure( REASON_SHUTTING_DOWN );
		}

		requestMap_.clear();
	}
}


/**
 *	This method cancels the requests for the input channel.
 */
void RequestManager::cancelRequestsFor( Channel * pChannel )
{
	RequestMap::iterator iter = requestMap_.begin();

	while (iter != requestMap_.end())
	{
		Request & request = *iter->second;

		// Note: failRequest can modify the map so iter needs to be moved
		// early.
		++iter;

		if (request.matches( pChannel ))
		{
			this->failRequest( request, REASON_CHANNEL_LOST );
		}
	}
}


/**
 * Given a pointer to a reply handler, this method will remove any reference to
 * it in the reply handler elements so will not receive a message or time out
 * when it is not supposed to.
 */
void RequestManager::cancelRequestsFor( ReplyMessageHandler * pHandler,
	   	Reason reason )
{
	RequestMap::iterator iter = requestMap_.begin();

	while (iter != requestMap_.end())
	{
		Request & request = *iter->second;

		// Note: failRequest can modify the map so iter needs to be moved
		// early.
		++iter;

		if (request.matches( pHandler ))
		{
			this->failRequest( request, reason );
		}
	}
}


/**
 *	Cleans up when a request has failed, and informs the handler.
 */
void RequestManager::failRequest( Request & request, Reason reason )
{
	requestMap_.erase( request.replyID() );
	request.handleFailure( reason );
}


/**
 *	Allocates and fills out the reply IDs for pending requests for a bundle on
 *	a channel.
 */
void RequestManager::addReplyOrder( const ReplyOrder & replyOrder,
		   	Channel * pChannel )
{
	ReplyID replyID = this->getNextReplyID();

	Request * pRequest = new Request( replyID,
			replyOrder, pChannel, this, dispatcher_ );

	requestMap_[ replyID ] = pRequest;

	// Write the allocated replyID in the bundle data.
#if defined( BW_ENFORCE_ALIGNED_ACCESS )
	// The address may not be aligned, so we need to do it byte-by-byte:
	ReplyID replyIDNBO = BW_HTONL( replyID );
	memcpy( replyOrder.pReplyID, &replyIDNBO, sizeof(replyIDNBO) );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
	ReplyID * pReplyID = reinterpret_cast< ReplyID * >( 
		replyOrder.pReplyID );
	*pReplyID = BW_HTONL( replyID );
#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )
}


/**
 * 	This method handles a reply message.
 */
void RequestManager::handleMessage( const Address & source,
	UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	// first let's do some sanity checks
	if (header.identifier != REPLY_MESSAGE_IDENTIFIER)
	{
		ERROR_MSG( "RequestManager::handleMessage( %s ): "
			"received the wrong kind of message!\n",
			source.c_str() );

		return;
	}

	if (header.length < (int)(sizeof(int)))
	{
		ERROR_MSG( "RequestManager::handleMessage( %s ): "
			"received a corrupted reply message (length %d)!\n",
			source.c_str(), header.length );

		return;
	}

	int		inReplyTo;
	data >> inReplyTo;
	header.length -= sizeof(int);
	// note 'header' is not const for this reason!

	// see if we can look up this ID
	RequestMap::iterator rheIterator = requestMap_.find( inReplyTo );
	if (rheIterator == requestMap_.end())
	{
		WARNING_MSG( "RequestManager::handleMessage( %s ): "
			"Couldn't find handler for reply id 0x%08x (maybe it timed out?)\n",
			source.c_str(), inReplyTo );

		data.finish();
		return;
	}

	Request	* pRequest = rheIterator->second;

	// Check the message came from the right place.  We only enforce this check
	// on external nubs because replies can come from different addresses on
	// internal nubs if a channel has been offloaded.
	if (isExternal_ &&
			!pRequest->isValidSource( source ))
	{
		WARNING_MSG( "RequestManager::handleMessage: "
			"Got reply to request %d from unexpected source %s\n",
			inReplyTo, source.c_str() );

		return;
	}

	// get it out of the pending map
	requestMap_.erase( rheIterator );

	// finally we call the reply handler
	pRequest->handleMessage( source, header, data );
}

} // namespace Mercury

BW_END_NAMESPACE

// request_manager.cpp
