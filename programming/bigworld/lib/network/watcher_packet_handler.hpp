#ifndef WATCHER_PACKET_HANDLER_HPP
#define WATCHER_PACKET_HANDLER_HPP

#include "cstdmf/config.hpp"
#if ENABLE_WATCHERS

#include "watcher_endpoint.hpp"
#include "watcher_nub.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/watcher_path_request.hpp"

BW_BEGIN_NAMESPACE

class WatcherPathRequest;
typedef BW::vector<WatcherPathRequest *> PathRequestList;


/**
 * This class is responsible for managing individual watcher path requests,
 * waiting for their responses, and then replying to the requesting process
 * with the collection of watcher path responses.
 */
class WatcherPacketHandler : public WatcherPathRequestNotification
{
public:
	/**
	 * Enumeration of different watcher protocol versions the packet
	 * handler can respond with.
	 */ 
	enum WatcherProtocolVersion
	{
		WP_VERSION_UNKNOWN = 0, //!< Unknown watcher protocol version.
		WP_VERSION_1,           //!< Protocol version 1 (string based).
		WP_VERSION_2            //!< Protocol version 2.
	};


	WatcherPacketHandler( const WatcherEndpoint & watcherEndpoint,
		int32 numPaths, WatcherProtocolVersion version, bool isSet = false );
	virtual ~WatcherPacketHandler();


	void run();
	void checkSatisfied();
	void sendReply();
	void doNotDelete( bool shouldNotDelete );


	/*
	 * Overridden from WatcherPathRequestNotification
	 */

	WatcherPathRequest * newRequest( BW::string & path );
	virtual void notifyComplete( WatcherPathRequest & pathRequest,
		int32 count );

private:

	static const BW::string v1ErrorIdentifier;	//!< Error prefix used in v1 watcher responses when responses will exceed UDP packet size.
	static const BW::string v1ErrorPacketLimit;	//!< Error message used in v1 watcher responses when responses will exceed UDP packet size.

	bool canDelete_; //!< Set to true when we aren't initiating path requests.

	WatcherEndpoint watcherEndpoint_; //!< The endpoint we should communicate through.

	WatcherProtocolVersion version_; //!< The protocol version we should talk.
	bool isSet_; //!< Is the current watcher request a set operation.

	int32 outgoingRequests_; //!< The number of separate watcher path requests we have initiated.
	int32 answeredRequests_; //!< The number of path requests that have replied to us.
	bool isExecuting_; //!< True if we are active and expecting responses.

	MemoryOStream packet_; //!< The raw packet data we will respond with.
	PathRequestList pathRequestList_; //!< List of all path requests belonging to the current watcher query.

	int maxPacketSize_; //!< The maximum size of packet we should fill with watcher response data.
	bool reachedPacketLimit_; //!< Have we already filled the outgoing packet.
};

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */


#endif /* ifndef WATCHER_PACKET_HANDLER_HPP */

