#include "anonymous_channel_client.hpp"

#include "network/channel_owner.hpp"
#include "network/interface_minder.hpp"
#include "network/interfaces.hpp"
#include "network/machined_utils.hpp"
#include "network/network_interface.hpp"

#include "cstdmf/debug_message_categories.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: AnonymousChannelClient
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
AnonymousChannelClient::AnonymousChannelClient() :
	pChannelOwner_( NULL )
{
}


/**
 *	Destructor.
 */
AnonymousChannelClient::~AnonymousChannelClient()
{
	delete pChannelOwner_;
}


/**
 *	This method initialises this object.
 *
 *	@return true on success, otherwise false.
 */
bool AnonymousChannelClient::init( Mercury::NetworkInterface & interface,
		Mercury::InterfaceMinder & interfaceMinder,
		const Mercury::InterfaceElement & birthMessage,
		const char * componentName,
		int numRetries )
{
	interfaceName_ = componentName;

	bool result = true;

	interfaceMinder.handler( birthMessage.id(), this );

	if (Mercury::MachineDaemon::registerBirthListener(
				interface.address(), birthMessage, componentName ) !=
			Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "AnonymousChannelClient::init: "
			"Failed to register birth listener for %s\n",
			componentName );

		result = false;
	}

	Mercury::Address serverAddr( Mercury::Address::NONE );

	if (Mercury::MachineDaemon::findInterface(
			componentName, 0, serverAddr, numRetries ) !=
		Mercury::REASON_SUCCESS)
	{
		result = false;
	}

	// Everyone talking to another process via this mechanism is doing it
	// irregularly at the moment.  Could make this optional.
	pChannelOwner_ = new Mercury::ChannelOwner( interface, serverAddr );
	pChannelOwner_->channel().isLocalRegular( false );
	pChannelOwner_->channel().isRemoteRegular( false );

	return result;
}


/**
 *	This method handles a birth message telling us that a new server has
 *	started.
 */
void AnonymousChannelClient::handleMessage(
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	Mercury::Address serverAddr;
	data >> serverAddr;

	MF_ASSERT( data.remainingLength() == 0 && !data.error() );

	pChannelOwner_->addr( serverAddr );

	PROC_IP_INFO_MSG( "AnonymousChannelClient::handleMessage: "
		"Got new %s at %s\n",
		interfaceName_.c_str(), pChannelOwner_->channel().c_str() );
}


/**
 *	This method adds a watcher for this instance.
 */
void AnonymousChannelClient::addWatchers( const char * name, Watcher & watcher )
{
	watcher.addChild( name, Mercury::ChannelOwner::pWatcher(), pChannelOwner_ );
}

BW_END_NAMESPACE

// anonymous_channel_client.cpp
