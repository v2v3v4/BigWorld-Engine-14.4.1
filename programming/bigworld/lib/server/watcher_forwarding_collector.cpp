#include "watcher_forwarding_collector.hpp"

#include "network/bundle.hpp"
#include "network/network_interface.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 *
 * @param pathRequest The WatcherPathRequest the collector should report
 *                    back to.
 * @param destWatcher The watcher path to be used on the components we are
 *                    forwarding to.
 */
ForwardingCollector::ForwardingCollector( WatcherPathRequestV2 & pathRequest,
	const BW::string & destWatcher ) :
	pathRequest_( pathRequest ),
	queryPath_( destWatcher ),
	callingComponents_( false ),
	pendingRequests_( 0 ),
	pAddressList_( NULL ),
	pInterface_( NULL ),
	tupleCount_( 0 )
{ }


/**
 * Destructor.
 */
ForwardingCollector::~ForwardingCollector()
{
	if (pAddressList_ != NULL)
	{
		delete pAddressList_;
	}

	// We don't own the interface, so don't destroy it
	pInterface_ = NULL;
}


/**
 * Initialise the Fowarding Collector with the component specific data.
 *
 * @param ie  The InterfaceElement that should be used to forward to.
 * @param interface The NetworkInterface to use to send messages from.
 * @param addrList A Pointer to a list of Mercury Addresses to forward
 *                 the messages to. NB: The ForwardingCollector claims
 *                 ownership of this AddressList and cleans it up on
 *                 destruction.
 *
 * returns true on successful initialisation, false on error.
 */
bool ForwardingCollector::init( const Mercury::InterfaceElement & ie,
	Mercury::NetworkInterface & interface, AddressList *addrList )
{
	if ((addrList == NULL) || (addrList->size() == 0))
	{
		ERROR_MSG( "ForwardingCollector: Failed to initialised due to "
					"empty target AddressList.\n" );
		return false;
	}

	pAddressList_ = addrList;
	pInterface_ = &interface;
	interfaceElement_ = ie;

	return true;
}


/**
 * Start the collector.
 *
 * Starting the collector will initiate all mercury requests to be sent
 * to the appropriate components.
 */
bool ForwardingCollector::start()
{
	if (callingComponents_)
	{
		ERROR_MSG( "ForwardingCollector: Failed to start, already calling "
					"components.\n" );
		return false;
	}

	callingComponents_ = true;
	AddressList::iterator iter = pAddressList_->begin();

	int forwardingDataSize =
		pathRequest_.getValueStream()->remainingLength();
	const void * forwardingData =
		pathRequest_.getValueStream()->retrieve( forwardingDataSize );

	if (!forwardingDataSize)
	{
		ERROR_MSG( "ForwardingCollector: No data provided to forward.\n" );
		return false;
	}

	while (iter != pAddressList_->end())
	{
		pendingRequests_++;

		Mercury::UDPBundle bundle;

		// Depending on whether we're the cellappmgr or the
		// baseappmgr, choose what to call? (maybe subclass this
		// stuff for the two types)
		bundle.startRequest( interfaceElement_, this );


		// We need to stream on the bundle:
		// - path to watcher function
		// - args of watcher function
		bundle << queryPath_;

		// Now add the data we are forwarding
		bundle.addBlob( forwardingData, forwardingDataSize );

		// Send the bundle to the component instance
		pInterface_->send( (*iter).first, bundle );

		++iter;
	}

	callingComponents_ = false;
	this->checkSatisfied();

	return true;
}


/**
 * Called upon receipt of each message to determine determine when
 * to notify the WatcherPathRequest.
 */
void ForwardingCollector::checkSatisfied()
{
	if ( callingComponents_ || pendingRequests_ )
		return;

	MemoryOStream & parentStream = pathRequest_.getResultStream();

	MemoryOStream tmpStream;
	tmpStream.writeStringLength( 2 ); // pair of output string / result
	//TODO: optimise this to ...
	//watcherValueToStream( tmpStream, .... );
	tmpStream << (uchar)WATCHER_TYPE_STRING;
	tmpStream << (uchar)Watcher::WT_READ_ONLY;
	tmpStream << outputStr_;	// The output string

	// Now the result data
	tmpStream << (uchar)WATCHER_TYPE_TUPLE;
	tmpStream << (uchar)Watcher::WT_READ_ONLY;
	tmpStream.writeStringLength( resultStream_.size() +
		BinaryOStream::calculatePackedIntSize( tupleCount_ ) );
	tmpStream.writeStringLength( tupleCount_ ); // Tuple entry count
	tmpStream.addBlob( resultStream_.data(), resultStream_.size() );


	// We now have the contained data packed so add it to the parent
	parentStream << (uchar)WATCHER_TYPE_TUPLE;
	parentStream << (uchar)Watcher::WT_READ_ONLY;
	parentStream.writeStringLength( tmpStream.size() );
	parentStream.addBlob( tmpStream.data(), tmpStream.size() );

	// Notify the path request that the result has been filled
	pathRequest_.setResult( "", Watcher::WT_READ_ONLY, NULL, NULL );

	// Nothing owns us and we have notified everything that was waiting for
	// us to complete, so go ahead and clean up.
	delete this;
}



// ===========================================================================
// Section: Mercury Interface Implementation
// ===========================================================================

// Overridden from Mercury::ReplyMessageHandler
void ForwardingCollector::handleMessage(
	const Mercury::Address & addr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data, void * arg)
{
	// Find the address / component ID of the responder so we can use it
	// when we decode the response.
	AddressList::iterator iter = pAddressList_->begin();
	while (iter != pAddressList_->end())
	{
		if ((*iter).first == addr)
			break;

		iter++;
	}

	if (iter == pAddressList_->end())
	{
		ERROR_MSG( "ForwardingCollector::handleMessage: "
						"Received a response from an unknown address (%s).\n",
					addr.c_str() );
		return;
	}

	pendingRequests_--;

	// push this off to the stream decoder
	ResponseDecoder decoder( (*iter).second, outputStr_, resultStream_ );
	if (decoder.decode( data ))
	{
		// We have another complete result tuple entry
		tupleCount_++;

		// The final element of the stream should be a boolean status.
		bool status;
		data >> status;
	}

	this->checkSatisfied();
	return;
}


// Overridden from Mercury::ReplyMessageHandler
void ForwardingCollector::handleException(
	const Mercury::NubException & ne, void *arg)
{
	Mercury::Address addr;
	if (ne.getAddress( addr ))
	{
		ERROR_MSG( "FowardingCollector: "
					"Received a mercury exception response from %s. '%s'\n",
				addr.c_str(),
				reasonToString( ne.reason() ) );
	}
	else
	{
		ERROR_MSG( "FowardingCollector: Received a mercury exception "
				"response. '%s'\n", reasonToString( ne.reason() ) );
	}

	// Stream error response into our result_ stream;
	pendingRequests_--;

	this->checkSatisfied();
}


/*
 * ForwardingCollector::ResponseDecoder
 */

/**
 * Custom decoding handler for our forwarding response.
 *
 * tupleHandler should be the first method called by decodeNext, followed
 * by stringHandler.
 *
 * @param stream The response sent back to us from a component.
 *
 * @returns true on success, false on error.
 */
bool ForwardingCollector::ResponseDecoder::decode( BinaryIStream & stream )
{
	if (!this->decodeNext( stream ) || (state_ != EXPECTING_OUTPUT))
	{
		ERROR_MSG( "First element of component response not a tuple as "
					"required. Dropping component response.\n" );
		return false;
	}

	if (!this->decodeNext( stream ) || (state_ != EXPECTING_ANY))
	{
		ERROR_MSG( "Second element of component response not a string as "
					"required. Dropping component response.\n" );
		return false;
	}

	return this->packResult( stream );
}


/**
 * Put the remaining contents of a stream (excluding the final
 * status byte) onto the result stream as a new tuple element.
 *
 * @param stream The response sent back to us from a component.
 *
 * @returns true on success, false on error.
 */
bool ForwardingCollector::ResponseDecoder::packResult(
	BinaryIStream & stream )
{
	if (state_ != EXPECTING_ANY)
		return false;

	MemoryOStream tempStream;
	tempStream.writeStringLength( 3 ); // pair of id / result / type

	// Add the component id
	//TODO: this is defaulting to a string type, would be nice to
	//      return as an actual int (possibly cast to int32).
	watcherValueToStream(
			tempStream, component_.second, Watcher::WT_READ_ONLY );


	// Size of the data should be the rest of the length with the 1 byte
	// status taken off.
	int size = stream.remainingLength() - 1;
	tempStream.addBlob( stream.retrieve( size ), size );

	// Add the component type
	watcherValueToStream( tempStream, component_.first, Watcher::WT_READ_ONLY );

	//result tuple, ro, size, [count, ... ]
	// Now push the component id / result onto the final stream
	resultStream_ << (uchar)WATCHER_TYPE_TUPLE;
	resultStream_ << (uchar)Watcher::WT_READ_ONLY;
	resultStream_.writeStringLength( tempStream.size() );
	resultStream_.addBlob( tempStream.data(), tempStream.size() );

	return true;
}


// Overriden from WatcherProtocolDecoder
bool ForwardingCollector::ResponseDecoder::stringHandler(
	BinaryIStream & stream, Watcher::Mode mode )
{
	// If we're not expecting output then we've encountered a bad stream
	if (state_ != EXPECTING_OUTPUT)
		return false;

	// Attempt to extract the string and place it on the output stream.
	BW::string str;
	stream >> str;

	if (stream.error())
	{
		ERROR_MSG( "ForwardingCollector::ResponseDecoder: Failed to decode "
					"string containing stdout/stderr.\n" );
		return false;
	}

	// Ensure the final element in the output string is a newline so that
	// any other component data that is appended is seperated.
	if (str[str.size() - 1] != '\n')
	{
		str.append( "\n" );
	}

	outputStr_.append( str );

	state_ = EXPECTING_ANY;

	return true;
}


// Overriden from WatcherProtocolDecoder
bool ForwardingCollector::ResponseDecoder::tupleHandler(
	BinaryIStream & stream, Watcher::Mode mode )
{
	// We should only receive a func tuple straight up
	if (state_ != EXPECTING_TUPLE)
		return false;

	int size = this->readSize( stream );
	if ((size == 0) || stream.error())
	{
		ERROR_MSG( "ForwardingCollector::ResponseDecoder: Stream error "
					"decoding tuple size.\n" );
		return false;
	}

	int count = this->readSize( stream );
	if ((count != 2) || stream.error())
	{
		ERROR_MSG( "ForwardingCollector::ResponseDecoder: Incorrect number "
					"of tuple elements when decoding response.\n" );
		return false;
	}

	// Change our working state
	state_ = EXPECTING_OUTPUT;

	return true;
}

BW_END_NAMESPACE

// watcher_forwarding_collector.cpp

