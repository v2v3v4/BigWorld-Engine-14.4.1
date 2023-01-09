#ifndef WATCHER_FORWARDING_COLLECTOR_HPP
#define WATCHER_FORWARDING_COLLECTOR_HPP

#include "network/interfaces.hpp"
#include "network/interface_element.hpp"

#include "watcher_protocol.hpp"
#include "watcher_forwarding_types.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class NetworkInterface;
}

class WatcherPathRequestV2;


/**
 * This class is responsible for managing watcher requests to
 * components via Mercury as well as collecting responses from
 * all components the request was sent to.
 */
class ForwardingCollector : public Mercury::ShutdownSafeReplyMessageHandler
{
public:

	ForwardingCollector( WatcherPathRequestV2 & pathRequest,
		const BW::string & destWatcher );
	virtual ~ForwardingCollector();

	bool init( const Mercury::InterfaceElement & ie,
			Mercury::NetworkInterface & interface, AddressList *addrList );

	bool start();
	void checkSatisfied();


	/*
	 * Overridden from Mercury::ReplyMessageHandler
	 */
	void handleMessage(const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg);

	void handleException(const Mercury::NubException & ne, void *arg);


private:
	WatcherPathRequestV2 & pathRequest_;	//!< Parent path request.
	BW::string queryPath_;	//!< The watcher path to call on each component.

	bool callingComponents_;	//!< Are we currently sending out messages.
	uint32 pendingRequests_;	//!< Number of outstanding replies we're expecting.

	Mercury::InterfaceElement interfaceElement_;	//!< The Mercury Interface to talk to.
	AddressList *pAddressList_;	//!< List of component addresses to forward to.
	Mercury::NetworkInterface * pInterface_;	//!< The interface messages should be sent through.

	BW::string outputStr_;	//!< All the data from stdout/stderr.
	MemoryOStream resultStream_;	//!< Packed result tuple data.
	uint32 tupleCount_;	//!< Number of tuple entries.

	/**
	 * @internal
	 * Fowarding Collector protocol decoding handler for responses.
	 */
	class ResponseDecoder : public WatcherProtocolDecoder
	{
		/**
		 * @internal
		 */
		enum WorkState
		{
			EXPECTING_TUPLE,	//!< Should process FUNC_TUPLE next
			EXPECTING_OUTPUT,	//!< Expecting output stream now
			EXPECTING_ANY		//!< Any result could come on the stream now
		};

	public:
		ResponseDecoder( Component component,
			BW::string & output, MemoryOStream & result) :
			component_( component ),
			state_( EXPECTING_TUPLE ),
			outputStr_( output ),
			resultStream_( result )
		{ }


		bool decode( BinaryIStream & stream );
		bool stringHandler( BinaryIStream & stream, Watcher::Mode mode );
		bool tupleHandler( BinaryIStream & stream, Watcher::Mode mode );

	private:
		Component component_;	//!< The component of this decode.
		WorkState state_; //!< Current state processing indicator.

		BW::string &outputStr_;	//!< All the data from stdout/stderr.
		MemoryOStream &resultStream_;	//!< Packed result tuple data.

		bool packResult( BinaryIStream & stream );
	};

};

BW_END_NAMESPACE

#endif // WATCHER_FORWARDING_COLLECTOR_HPP
