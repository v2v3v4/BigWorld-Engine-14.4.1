#include "pch.hpp"

#include "cstdmf/config.hpp"
#if ENABLE_WATCHERS

#include "watcher_packet_handler.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 );

BW_BEGIN_NAMESPACE

const BW::string WatcherPacketHandler::v1ErrorIdentifier = "<Err>";
const BW::string WatcherPacketHandler::v1ErrorPacketLimit =
	"Exceeded maximum packet size";


/**
 * Constructor
 *
 * @param socket    The watcher nub endpoint to use when replying to the
 *                  watcher request.
 * @param addr      The address of the requesting process we should reply
 *                  to.
 * @param numPaths  The number of watcher paths that have been requested,
 *                  and we should wait to hear responses from.
 * @param version   The watcher protocol version that is being requested.
 * @param isSet     Is this watcher request a set operation?
 */
WatcherPacketHandler::WatcherPacketHandler(
			const WatcherEndpoint & watcherEndpoint,
			int32 numPaths, WatcherProtocolVersion version, bool isSet ) :
		canDelete_ ( false ),
		watcherEndpoint_( watcherEndpoint ),
		version_( version ),
		isSet_( isSet ),
		outgoingRequests_( numPaths ),
		answeredRequests_( 0 ),
		isExecuting_( false ),
		maxPacketSize_( 0 ),
		reachedPacketLimit_( false )
{
	switch (version_)
	{
	case WP_VERSION_1:
		packet_ << (int32)WATCHER_MSG_TELL;
		maxPacketSize_ = WN_PACKET_SIZE -
			(static_cast<int>(v1ErrorIdentifier.size())  + 1) -
			(static_cast<int>(v1ErrorPacketLimit.size()) + 1);
		break;

	case WP_VERSION_2:
		packet_ << (int32)(isSet_ ? WATCHER_MSG_SET2_TELL2 : WATCHER_MSG_TELL2);
		if (watcherEndpoint_.isTCP())
		{
			maxPacketSize_ = WN_PACKET_SIZE_TCP;
		}
		else
		{
			maxPacketSize_ = WN_PACKET_SIZE;
		}
		break;

	default:
		ERROR_MSG( "WatcherPacketHandler: Unknown version %d\n", version_ );
		version_ = WP_VERSION_UNKNOWN;
		break;
	}

	// Reserve 4 bytes for the count of the number of replies
	// so we can modify it immediately before dispatching the response.
	packet_ << (int32)0;
}


/**
 * Destructor.
 */
WatcherPacketHandler::~WatcherPacketHandler()
{
	PathRequestList::iterator iter = pathRequestList_.begin();

	while (iter != pathRequestList_.end())
	{
		bw_safe_delete(*iter);
		iter++;
	}

	pathRequestList_.clear();
}


// Overridden from WatcherPathRequestNotification
WatcherPathRequest * WatcherPacketHandler::newRequest( BW::string & path )
{
	WatcherPathRequest *pathRequest = NULL;

	if (isExecuting_)
	{
		ERROR_MSG( "WatcherPacketHandler::newRequest: Attempt to create a new "
					"path request after packet handler told disallowed new "
					"requests." );
		return NULL;
	}

	switch (version_)
	{
	case WP_VERSION_1:
		pathRequest = new WatcherPathRequestV1( path );
		break;
	case WP_VERSION_2:
		pathRequest = new WatcherPathRequestV2( path );
		break;
	default:
		ERROR_MSG( "WatcherPacket::newRequest: "
				"Invalid watcher protocol version %d\n", version_ );
	}

	if (pathRequest)
	{
		pathRequest->setParent( this );
		pathRequestList_.push_back( pathRequest );
	}

	return pathRequest;
}


/**
 * Start all the watcher path request operations.
 */
void WatcherPacketHandler::run()
{
	isExecuting_ = true;

	PathRequestList::iterator iter = pathRequestList_.begin();

	this->doNotDelete( true );

	while (iter != pathRequestList_.end())
	{
		if (isSet_)
		{
			(*iter)->setWatcherValue();
		}
		else
		{
			(*iter)->fetchWatcherValue();
		}

		iter++;
	}

	this->doNotDelete( false );
}



// Overridden from WatcherPathRequestNotification
void WatcherPacketHandler::notifyComplete( WatcherPathRequest & pathRequest,
	int32 count )
{
	if (!reachedPacketLimit_)
	{
		// Check we haven't reached the packet size limit (UDP 64K)
		if (!watcherEndpoint_.isTCP() &&
				((packet_.size() + pathRequest.getDataSize()) > maxPacketSize_))
		{
			ERROR_MSG( "WatcherPacketHandler::notifyComplete: Can't add reply "
						"from WatcherPathRequest( '%s' ) due to packet size "
						"limit.\n", pathRequest.getPath().c_str() );

			reachedPacketLimit_ = true;

			// For version 1, add the old error messages,
			// version 2 currently just drop the response
			if (version_ == WP_VERSION_1)
			{
				packet_.addBlob( v1ErrorIdentifier.c_str(),
							static_cast<int>(v1ErrorIdentifier.size() + 1) );
				packet_.addBlob( v1ErrorPacketLimit.c_str(),
							static_cast<int>(v1ErrorPacketLimit.size() + 1) );
			}

		}
		else
		{

			// TODO: this will be split across packets...
			// separate into a diff method
			packet_.addBlob( pathRequest.getData(), pathRequest.getDataSize() );

			// Now go back to the reserved reply count location.
			int32 *pReplyCount = (int32 *)(packet_.data());
			pReplyCount[1] += count;
		}
	}

	answeredRequests_++;
	this->checkSatisfied();
}


/**
 * Send a reply packet to the requestor.
 */
void WatcherPacketHandler::sendReply()
{
	if (watcherEndpoint_.send( packet_.data(), packet_.size() ) == -1)
	{
		WARNING_MSG( "WatcherPacketHandler::sendReply: Failed to send reply\n" );

		PathRequestList::iterator iter = pathRequestList_.begin();

		while (iter != pathRequestList_.end())
		{
			WARNING_MSG( "\tPath: '%s'\n", (*iter)->getPath().c_str() );
			++iter;
		}
	}

	return;
}


/**
 * Check whether all outgoing path requests have responded.
 *
 * When all requests have responded we can finalise the last outgoing
 * packet and then notify our owner for deletion.
 */
void WatcherPacketHandler::checkSatisfied()
{
	// cleanup any completed requests
	if (outgoingRequests_ == answeredRequests_)
	{
		if (isExecuting_)
		{
			// we are done...

			// send packet out
			this->sendReply();
			isExecuting_ = false;
		}

		if (canDelete_)
		{
			delete this;
		}
	}

	return;
}


/**
 * Set a flag to prevent this object from being deleted until it's safe.
 *
 * @param shouldNotDelete  Should the packet handler be undeleteable?
 */
void WatcherPacketHandler::doNotDelete( bool shouldNotDelete )
{
	canDelete_ = !shouldNotDelete;

	if (canDelete_)
	{
		this->checkSatisfied();
	}
}

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

// watcher_packet_handler.cpp
