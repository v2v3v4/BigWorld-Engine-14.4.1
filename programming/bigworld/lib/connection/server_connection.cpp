#include "pch.hpp"

#include "server_connection.hpp"

#include "baseapp_ext_interface.hpp"
#include "client_interface.hpp"
#include "condemned_interfaces.hpp"
#include "data_download.hpp"
#include "download_segment.hpp"
#include "entity_def_constants.hpp"
#include "login_challenge_factory.hpp"
#include "login_handler.hpp"
#include "login_interface.hpp"
#include "message_handlers.hpp"
#include "server_message_handler.hpp"


#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "math/vector3.hpp"

#include "network/bundle.hpp"
#include "network/channel.hpp"
#include "network/compression_stream.hpp"
#include "network/interface_table.hpp"
#include "network/network_interface.hpp"
#include "network/nub_exception.hpp"
#include "network/once_off_packet.hpp"
#include "network/packet_receiver_stats.hpp"
#include "network/portmap.hpp"
#include "network/public_key_cipher.hpp"
#include "network/symmetric_block_cipher.hpp"
#include "network/interface_element.hpp"
#include "cstdmf/profiler.hpp"


DECLARE_DEBUG_COMPONENT2( "Connection", 0 )

BW_BEGIN_NAMESPACE


#ifndef CODE_INLINE
#include "server_connection.ipp"
#endif

namespace // anonymous
{

const uint8 IS_ON_GROUND_UNKNOWN = 2;


/// The number of seconds of inactivity before a connection is closed.
const float DEFAULT_INACTIVITY_TIMEOUT = 60.f;			// 60 seconds

// How often the network statistics should be updated.
const float UPDATE_STATS_PERIOD = 1.f;	// 1 second

// Output a warning message if we receive packets further apart than this
// (in milliseconds)
const int PACKET_DELTA_WARNING_THRESHOLD = 3000;

} // namespace (anonymous)


/**
 *	This static member stores the value used in position coordinates to
 *	represent no value.
 */
const float ServerConnection::NO_POSITION = -13000.f;


/**
 *	This static member stores the value used in directin angles to represent
 *	no value.
 */
const float ServerConnection::NO_DIRECTION = FLT_MAX;


/**
 *	This method returns a human-readable string representation of the transport
 *	code.
 */
const char * ServerConnection::transportToCString( 
		ConnectionTransport transport )
{
	switch (transport)
	{
	case CONNECTION_TRANSPORT_UDP:
		return "UDP";
	case CONNECTION_TRANSPORT_TCP:
#if !defined( EMSCRIPTEN )
		return "TCP";
#endif
	case CONNECTION_TRANSPORT_WEBSOCKETS:
		return "WebSockets";
	default:
		return "(unknown)";
	}
}


/**
 *	Constructor.
 *
 *	@param entityDefConstants 	The constants from entity definitions.
 */
ServerConnection::ServerConnection(
		LoginChallengeFactories & challengeFactories,
		CondemnedInterfaces & condemnedInterfaces,
		const EntityDefConstants & entityDefConstants ) :
	Mercury::BundlePrimer(),
	Mercury::ChannelListener(),
	TimerHandler(),
	sessionKey_( 0 ),
	username_(),
	numMovementBytes_(),
	numNonMovementBytes_(),
	numOverheadBytes_(),
	pHandler_( NULL ),
	id_( EntityID( -1 ) ),
	selectedEntityID_( EntityID( -1 ) ),
	spaceID_( 0 ),
	bandwidthFromServer_( 0 ),
	pTime_( NULL ),
	lastReceiveTime_( 0 ),
	lastSendTime_( 0.0 ),
	minSendInterval_( 1.01/20.0 ),
	sendTimeReportThreshold_( 10.0 ),
	updateFrequency_( 10.f ),
	dispatcher_(),
	pInterface_( NULL ),
	pChannel_( NULL ),
	transport_( CONNECTION_TRANSPORT_UDP ),
	packetLossParameters_(),
	shouldResetEntitiesOnChannel_(),
	tryToReconfigurePorts_( false ),
	entitiesEnabled_( false ),
	isOnGround_( IS_ON_GROUND_UNKNOWN ),
	inactivityTimeout_( DEFAULT_INACTIVITY_TIMEOUT ),
	digest_( entityDefConstants.digest() ),
	serverTimeHandler_(),
	serverMsg_(),
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	sendingSequenceNumber_( 0 ),
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
	packedXZScale_( 1.f ),
	// (EntityID idAlias_[256]),
	passengerToVehicle_(),
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	// (Position3D sentPositions_[256]),
	referencePosition_(),
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
	controlledEntities_(),
	dataDownloads_(),
	pBlockCipher_( NULL ),
	pLogOnParamsEncoder_( NULL ),
	timerHandle_(),
	challengeFactories_( challengeFactories ),
	condemnedInterfaces_( condemnedInterfaces ),
	maxClientMethodCount_( entityDefConstants.maxExposedClientMethodCount() ),
	maxBaseMethodCount_( entityDefConstants.maxExposedBaseMethodCount() ),
	maxCellMethodCount_( entityDefConstants.maxExposedCellMethodCount() ),
	pTaskManager_( NULL ),
	// see also initialiseConnectionState
	FIRST_AVATAR_UPDATE_MESSAGE(
		ClientInterface::avatarUpdateNoAliasDetailed.id() ),
	LAST_AVATAR_UPDATE_MESSAGE(
		ClientInterface::avatarUpdateAliasNoPosNoDir.id() ),
	isEarly_( false )
{
	this->pInterface( new Mercury::NetworkInterface( &dispatcher_,
					  Mercury::NETWORK_INTERFACE_EXTERNAL ) ),
	// Timer for updating statistics
	timerHandle_ = this->dispatcher().addTimer(
					static_cast< int >( UPDATE_STATS_PERIOD * 1000000 ), this,
					NULL, "ServerConnection" );

	// Ten samples at one second each
	numMovementBytes_.monitorRateOfChange( 10 );
	numNonMovementBytes_.monitorRateOfChange( 10 );
	numOverheadBytes_.monitorRateOfChange( 10 );

	tryToReconfigurePorts_ = false;
	this->initialiseConnectionState();
}


/**
 *	Destructor
 */
ServerConnection::~ServerConnection()
{
	timerHandle_.cancel();

	// disconnect if we didn't already do so
	this->disconnect( /* informServer */ true, 
		/* shouldCondemnChannel */ true );

	bw_safe_delete( pInterface_ );
}


/**
 *	This method sets entity definition constants. Generally, this method
 *	should not be used and the constants should be passed in the constructor.
 *	This is here only if the constants are not available at that stage.
 */
void ServerConnection::setConstants( const EntityDefConstants & constants )
{
	digest_ = constants.digest();
	maxClientMethodCount_ = constants.maxExposedClientMethodCount();
	maxBaseMethodCount_ = constants.maxExposedBaseMethodCount();
	maxCellMethodCount_ = constants.maxExposedCellMethodCount();
}


/**
 *	This private method initialises or reinitialises our state related to a
 *	connection. It should be called before a new connection is made.
 */
void ServerConnection::initialiseConnectionState()
{
	id_ = EntityID( -1 );
	spaceID_ = SpaceID( -1 );
	bandwidthFromServer_ = 0;

	lastReceiveTime_ = 0;
	lastSendTime_ = 0.0;

	entitiesEnabled_ = false;
	isOnGround_ = IS_ON_GROUND_UNKNOWN;

	serverTimeHandler_ = ServerTimeHandler();
	serverTimeHandler_.pServerConnection( this );

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	sendingSequenceNumber_ = 0;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	memset( idAlias_, 0, sizeof( idAlias_ ) );

	shouldResetEntitiesOnChannel_ = false;

	controlledEntities_.clear();

	pBlockCipher_ = Mercury::SymmetricBlockCipher::create();
}


/**
 * This method returns the network interface for this connection. A network
 * interface is created if not existing.
 */
Mercury::NetworkInterface & ServerConnection::networkInterface()
{
	if (!pInterface_)
	{
		Mercury::NetworkInterface * pInterface = new Mercury::NetworkInterface(
			&this->dispatcher(), Mercury::NETWORK_INTERFACE_EXTERNAL );

		this->registerInterfaces( *pInterface );
		this->pInterface( pInterface );
	}

	return *pInterface_;
}


/**
 *	This private helper method registers the mercury interfaces with the
 *	provided network interface, so that it will serve them.  This should be
 *	done for every network interface that might receive messages from the
 *	server, which includes the temporary network interfaces used in the BaseApp
 *	login process.
 */
void ServerConnection::registerInterfaces(
		Mercury::NetworkInterface & networkInterface )
{
	ClientInterface::registerWithInterface( networkInterface );

	// Message handlers have access to the network interface that the message
	// arrived on. Set the ServerConnection here so that the message handler
	// knows which one to deliver the message to. (This is used by bots).
	networkInterface.pExtensionData( this );
}


/**
 *	This method logs in to the named server with the input username and
 *	password.
 *
 *	NOTE: This method has been deprecated as it blocks on network communication.
 *	Consider using either ServerConnection::logOnBegin(),
 *	SmartServerConnection::logOnTo() or BWConnection::logOnTo().
 *
 *	@param pHandler		The handler to receive messages from this connection.
 *	@param serverName	The name of the server to connect to.
 *	@param username		The username to log on with.
 *	@param password		The password to log on with.
 *	@param port			The port that the server should talk to us on.
 */
LogOnStatus ServerConnection::logOn( ServerMessageHandler * pHandler,
	const char * serverName,
	const char * username,
	const char * password,
	uint16 port )
{
	NOTICE_MSG( "ServerConnection::logOn: This method has been deprecated as "
			"it blocks on network communication. Consider using either "
			"ServerConnection::logOnBegin(), SmartServerConnection::logOnTo() "
			"or BWConnection::logOnTo().\n" );

	LoginHandlerPtr pLoginHandler = this->logOnBegin(
		serverName, username, password, port );

	// Note: we could well get other messages here and not just the
	// reply (if the proxy sends before we get the reply), but we
	// don't add ourselves to the master/slave system before then
	// so we won't be found ... and in the single servconn case the
	// channel (data) won't be created until then so the the intermediate
	// messages will be resent.

	while (!pLoginHandler->isDone())
	{
		dispatcher_.processUntilBreak();
	}

	LogOnStatus status = this->logOnComplete( pLoginHandler, pHandler );

	if (status == LogOnStatus::LOGGED_ON)
	{
		this->enableEntities();
	}

	return status;
}


/**
 *	This method begins an asynchronous login.
 *
 *	@param serverName 	The server in dotted-quad or hostname form.
 *	@param username		The name of the user account.
 *	@param password 	The account password.
 *	@param port 		The port to connect to.
 *	@param transport 	The transport to use, one of
 *						ConnectionTransport.
 */
LoginHandlerPtr ServerConnection::logOnBegin(
	const char * serverName,
	const char * username,
	const char * password,
	uint16 port,
	ConnectionTransport transport )
{
	this->initialiseConnectionState();

	LogOnParamsPtr pParams = new LogOnParams( username, password,
		pBlockCipher_->key() );

	pParams->digest( this->digest() );

	// make sure we are not already logged on
	if (this->isOnline())
	{
		return new LoginHandler( this, LogOnStatus::ALREADY_ONLINE_LOCALLY );
	}

	TRACE_MSG( "ServerConnection::logOnBegin: "
			"Using server: %s, transport: %s, username: %s \n",
		serverName, this->transportToCString( transport ),
		pParams->username().c_str() );

	username_ = pParams->username();

	// Register the interfaces if they have not already been registered.
	this->registerInterfaces( this->networkInterface() );

	// Find out where we want to log in to
	uint16 loginPort = port ? port : PORT_LOGIN;

	const char * serverPort = strchr( serverName, ':' );
	BW::string serverNameStr;
	if (serverPort != NULL)
	{
		loginPort = atoi( serverPort+1 );
		serverNameStr.assign( serverName, serverPort - serverName );
		serverName = serverNameStr.c_str();
	}

	Mercury::Address loginAddr( 0, htons( loginPort ) );

	// Save this away so we can recall it when switching BaseApps.
	transport_ = transport;

	// TODO: Make DNS lookups non-blocking.
	if (Endpoint::convertAddress( serverName, (u_int32_t&)loginAddr.ip ) != 0 ||
		loginAddr.ip == 0)
	{
		ERROR_MSG( "ServerConnection::logOnBegin: "
				"Could not find server '%s'\n", serverName );
		return new LoginHandler( this, LogOnStatus::DNS_LOOKUP_FAILED );
	}

	// Create a LoginHandler and start the handshake

	LoginHandlerPtr pLoginHandler = new LoginHandler( this );
	pLoginHandler->start( loginAddr, transport, pParams );
	return pLoginHandler;
}


/**
 *	This method completes an asynchronous login.
 *
 *	Note: Don't call this from within processing the bundle that contained
 *	the reply if you have multiple ServConns, as it could stuff up processing
 *	for another of the ServConns.
 */
LogOnStatus ServerConnection::logOnComplete(
	LoginHandlerPtr pLoginHandler,
	ServerMessageHandler * pHandler )
{
	MF_ASSERT_DEV( pLoginHandler != NULL );

	LogOnStatus status = pLoginHandler->status();

	if ((status == LogOnStatus::LOGGED_ON) &&
			!this->isOnline())
	{
		WARNING_MSG( "ServerConnection::logOnComplete: "
				"Already logged off\n" );

		status = LogOnStatus::CANCELLED;
		serverMsg_ = "Already logged off";
	}
	else if (status == LogOnStatus::LOGGED_ON)
	{
		DEBUG_MSG( "ServerConnection::logOn: status==LOGGED_ON\n" );

		serverMsg_ = pLoginHandler->serverMsg();

		const LoginReplyRecord & result = pLoginHandler->replyRecord();

		DEBUG_MSG( "ServerConnection::logOn: from: %s\n",
				   this->networkInterface().address().c_str() );
		DEBUG_MSG( "ServerConnection::logOn: to:   %s\n",
				result.serverAddr.c_str() );

		// We establish our channel to the BaseApp in
		// BaseAppLoginHandler::handleMessage - this is just a sanity check.
		if (result.serverAddr != this->addr())
		{
			char winningAddr[ 256 ];
			strncpy( winningAddr, this->addr().c_str(), sizeof( winningAddr ) );

			WARNING_MSG( "ServerConnection::logOnComplete: "
				"BaseApp address on login reply (%s) differs from winning "
				"BaseApp reply (%s)\n",
				result.serverAddr.c_str(), winningAddr );
		}
	}
	else if (status == LogOnStatus::CONNECTION_FAILED)
	{
		ERROR_MSG( "ServerConnection::logOnComplete: Logon failed (%s)\n",
				pLoginHandler->errorMsg().c_str() );
		status = LogOnStatus::CONNECTION_FAILED;
		serverMsg_ = pLoginHandler->serverMsg();
	}
	else if (status == LogOnStatus::DNS_LOOKUP_FAILED)
	{
		serverMsg_ = "DNS lookup failed";
		ERROR_MSG( "ServerConnection::logOnComplete: Logon failed: %s\n",
				serverMsg_.c_str() );
	}
	else
	{
		serverMsg_ = pLoginHandler->errorMsg();
		INFO_MSG( "ServerConnection::logOnComplete: Logon failed: %s\n",
				serverMsg_.c_str() );
	}

	// Release the reply handler
	pLoginHandler = NULL;

	// Get out if we didn't log on
	if (status != LogOnStatus::LOGGED_ON)
	{
		return status;
	}

	// Yay we logged on!

	id_ = NULL_ENTITY_ID;

	// Send an initial packet to the proxy to open up a hole in any
	// firewalls on our side of the connection.
	this->send();

	// DEBUG_MSG( "ServerConnection::logOn: sent initial message to server\n" );

	// Install the user's server message handler until we disconnect
	// (serendipitous leftover argument from when it used to be called
	// here to create the player entity - glad I didn't clean that up :)
	pHandler_ = pHandler;

	return status;
}


/**
 *	This method enables the receipt of entity and related messages from the
 *	server, i.e. the bulk of game traffic. The server does not start sending
 *	to us until we are ready for it. This should be called shortly after login.
 */
void ServerConnection::enableEntities()
{
	// Ok cell, we are ready for entity updates now.
	this->bundle().startMessage( BaseAppExtInterface::enableEntities );

	DEBUG_MSG( "ServerConnection::enableEntities: Enabling entities\n" );

	this->send();

	entitiesEnabled_ = true;
}


/**
 * 	This method returns whether or not we are online with the server.
 *	If a login is in progress, it will still return false.
 */
bool ServerConnection::isOnline() const
{
	return (pChannel_ != NULL) && pChannel_->isConnected();
}


/**
 *	This method removes the channel. This should be called when an exception
 *	occurs during processing input, or when sending data fails. The layer above
 *	can detect this by checking the online method once per frame. This is safer
 *	than using a callback, since a disconnect may happen at inconvenient times,
 *	e.g. when sending.
 */
void ServerConnection::disconnect( bool informServer /* = true */,
	bool shouldCondemnChannel /* = true */)
{
	// Destroy our channel
	// delete pChannel_;
	if (pChannel_ != NULL)
	{
		if (informServer)
		{
			BaseAppExtInterface::disconnectClientArgs args;
			args.reason = 0; // reason not yet used.
			this->bundle().sendMessage( args, Mercury::RELIABLE_NO );

			this->channel().send();
		}

		pChannel_->pChannelListener( NULL );

		if (pChannel_->isConnected())
		{
			if (shouldCondemnChannel)
			{
				pChannel_->shutDown();
			}
			else
			{
				pChannel_->destroy();
			}
		}

		pChannel_->bundlePrimer( NULL );
		pChannel_ = NULL;
	}

	// clear in-progress proxy data downloads
	for (uint i = 0; i < dataDownloads_.size(); ++i)
	{
		bw_safe_delete( dataDownloads_[i] );
	}
	dataDownloads_.clear();

	// forget the handler and the session key
	pHandler_ = NULL;
	sessionKey_ = 0;

        // Discard the network interface so we can have a new port, and
        // no other state. The new network interface is lazily recreated.
	if (pInterface_)
	{
		pInterface_->detach();
		condemnedInterfaces_.removeWhenAcked( pInterface_, inactivityTimeout_ );

		pInterface_ = NULL;
	}
}


/**
 *	This method sets the NetworkInterface associated with this ServerConnection.
 */
void ServerConnection::pInterface( Mercury::NetworkInterface * pInterface )
{
	pInterface_ = pInterface;

	if (pInterface_ != NULL)
	{
		packetLossParameters_.apply( *pInterface_ );
	}
}


/**
 *  This method returns the ServerConnection's channel.
 */
Mercury::Channel & ServerConnection::channel()
{
	// Don't call this before this->isOnline() is true.
	MF_ASSERT_DEV( pChannel_ );
	return *pChannel_;
}


/**
 *	This method sets the channel to the server.
 */
void ServerConnection::channel( Mercury::Channel & channel )
{
	if (pChannel_.get() != NULL)
	{
		pChannel_->pChannelListener( NULL );
	}

	pChannel_ = &channel;

	pChannel_->bundlePrimer( this );

	// Disconnect if we do not receive anything for this number of seconds.
	pChannel_->startInactivityDetection( inactivityTimeout_ );

	// If we are switching to another proxy entity on another BaseApp, we need
	// to reset our entities and re-enable entities once we have the new
	// channel.
	if (shouldResetEntitiesOnChannel_)
	{
		this->resetEntities();
		shouldResetEntitiesOnChannel_ = false;
	}

	pChannel_->pChannelListener( this );
}


/**
 *	This method returns the address of the server process we are currently
 *	communicating with,  or NONE if we are not connected or are logging in.
 */
const Mercury::Address & ServerConnection::addr() const
{
	if (this->isOffline())
	{
		return Mercury::Address::NONE;
	}

	// TODO: It'd be good to be able to return the LoginApp address during
	// login, but currently ServerConnection passes it to LoginHandler without
	// remembering it for itself.

	return pChannel_->addr();
}


/**
 * 	This method adds a message to the server to inform it of the
 * 	new position and direction (and the rest) of an entity under our control.
 *	The server must have explicitly given us control of that entity first.
 *
 *	@param id			ID of entity or NULL_ENTITY_ID to use connected entity
 *	@param spaceID		ID of the space the entity is in or NULL_SPACE_ID
 *						to use the space of the connected Entity.
 *	@param vehicleID	ID of the innermost vehicle the entity is on.
 * 	@param pos			Local position of the entity.
 * 	@param yaw			Local direction of the entity.
 * 	@param pitch		Local direction of the entity.
 * 	@param roll			Local direction of the entity.
 * 	@param onGround		Whether or not the entity is on terrain (if present).
 * 	@param globalPos	Approximate global position of the entity
 */
void ServerConnection::addMove( EntityID id, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos,
	float yaw, float pitch, float roll,
	bool onGround, const Position3D & globalPos )
{
	if (this->isOffline())
		return;

	if (spaceID == NULL_SPACE_ID)
	{
		spaceID = spaceID_;
	}

	if (id == NULL_ENTITY_ID)
	{
		id = id_;
	}

	if (spaceID != spaceID_)
	{
		ERROR_MSG( "ServerConnection::addMove: "
					"Attempted to move %d from space %u to space %u\n",
				id, spaceID_, spaceID );
		return;
	}

	if (!this->isControlledLocally( id ))
	{
		ERROR_MSG( "ServerConnection::addMove: "
				"Tried to add a move for entity id %d that we do not control\n",
			id );
		// be assured that even if we did not return here the server
		// would not accept the position update regardless!
		return;
	}

	EntityID currVehicleID = this->getVehicleID( id );

	bool shouldSendExplicit = (vehicleID != currVehicleID);

	Position3D coordPos( BW_HTONF( pos.x ), BW_HTONF( pos.y ), BW_HTONF( pos.z ) );
	YawPitchRoll dir( yaw, pitch, roll );

	Mercury::Bundle & bundle = this->bundle();

	if (id == id_)
	{
		bool hasChangedOnGround = (uint8(onGround) != isOnGround_);
		shouldSendExplicit |= hasChangedOnGround;
		isOnGround_ = onGround;

		// Send reliably when the isOnGround state changes.
		// TODO: It is probably better if this is not sent reliably and works
		// the same as vehicle changes.
		Mercury::ReliableType reliableType =
			hasChangedOnGround ? Mercury::RELIABLE_DRIVER : Mercury::RELIABLE_NO;

		// TODO: When on a vehicle, the reference number is not used and so does not
		// need to be sent (and remembered).

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
		uint8 refNum = sendingSequenceNumber_;
		sentPositions_[ sendingSequenceNumber_ ] = globalPos;
		++sendingSequenceNumber_;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

		if (!shouldSendExplicit)
		{
			BaseAppExtInterface::avatarUpdateImplicitArgs upArgs;

			upArgs.pos = coordPos;
			upArgs.dir = dir;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
			upArgs.refNum = refNum;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

			bundle.sendMessage( upArgs, reliableType );
		}
		else
		{
			BaseAppExtInterface::avatarUpdateExplicitArgs upArgs;

			upArgs.vehicleID = BW_HTONL( vehicleID );
			upArgs.flags = onGround ? AVATAR_UPDATE_EXPLICT_FLAG_ONGROUND : 0;

			upArgs.pos = coordPos;
			upArgs.dir = dir;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
			upArgs.refNum = refNum;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
			
			bundle.sendMessage( upArgs, Mercury::RELIABLE_NO );
		}
	}
	else
	{
		if (!shouldSendExplicit)
		{
			BaseAppExtInterface::avatarUpdateWardImplicitArgs upArgs;

			upArgs.ward = BW_HTONL( id );

			upArgs.pos = coordPos;
			upArgs.dir = dir;

			bundle.sendMessage( upArgs, Mercury::RELIABLE_NO );
		}
		else
		{
			BaseAppExtInterface::avatarUpdateWardExplicitArgs upArgs;

			upArgs.ward = BW_HTONL( id );
			upArgs.vehicleID = BW_HTONL( vehicleID );
			// Currently not handling onGround for wards.
			upArgs.flags = 0;

			upArgs.pos = coordPos;
			upArgs.dir = dir;

			bundle.sendMessage( upArgs, Mercury::RELIABLE_NO );
		}
	}

	// Currently even when we control an entity we keep getting updates
	// for it but just ignore them. This is so we can get the various
	// prefixes. We could set the vehicle info ourself but changing
	// the space ID is not so straightforward. However the main
	// advantage of this approach is not having to change the server to
	// know about the entities that we control. Unfortunately it is
	// quite inefficient - both for sending unnecessary explicit
	// updates (sends for about twice as long) and for getting tons
	// of unwanted position updates, mad worse by the high likelihood
	// of controlled entities being near to the client. Oh well,
	// it'll do for now.
}


/**
 * 	This method is called to start a new message to the proxy.
 * 	Note that proxy messages cannot be sent on a bundle after
 * 	entity messages.
 *
 * 	@param methodID	The method to send.
 *
 * 	@return	A stream to write the message on.
 */
BinaryOStream & ServerConnection::startBasePlayerMessage( int methodID )
{
	if (this->isOffline())
	{
		CRITICAL_MSG( "ServerConnection::startBasePlayerMessage: "
				"Called when not connected to server!\n" );
	}

	const ExposedMethodMessageRange & range =
		BaseAppExtInterface::Range::baseEntityMethodRange;

	int16 msgID;
	int16 subMsgID;

	range.msgIDFromExposedID( methodID, maxBaseMethodCount_, msgID, subMsgID );

	Mercury::InterfaceElement ie = BaseAppExtInterface::baseEntityMethod;

	ie.id( Mercury::MessageID( msgID ) );
	this->bundle().startMessage( ie );

	if (subMsgID != -1)
	{
		this->bundle() << uint8( subMsgID );
	}

	return this->bundle();
}


/**
 * 	This message sends an entity message for the player's avatar.
 *
 * 	@param methodID	The message to send.
 *
 * 	@return A stream to write the message on.
 */
BinaryOStream & ServerConnection::startCellPlayerMessage( int methodID )
{
	return this->startCellEntityMessage( methodID, 0 );
}


/**
 * 	This message sends an entity message to a given entity.
 *
 * 	@param methodID		The id of the method to call.
 * 	@param entityID		The id of the entity to receive the message.
 *
 * 	@return A stream to write the message on.
 */
BinaryOStream & ServerConnection::startCellEntityMessage( int methodID,
		EntityID entityID )
{
	if (this->isOffline())
	{
		CRITICAL_MSG( "ServerConnection::startEntityMessage: "
				"Called when not connected to server!\n" );
	}

	const ExposedMethodMessageRange & range =
		BaseAppExtInterface::Range::cellEntityMethodRange;

	int16 msgID;
	int16 subMsgID;

	range.msgIDFromExposedID( methodID, maxCellMethodCount_, msgID, subMsgID );

	Mercury::InterfaceElement ie = BaseAppExtInterface::cellEntityMethod;

	ie.id( Mercury::MessageID( msgID ) );
	this->bundle().startMessage( ie );

	this->bundle() << entityID;

	if (subMsgID != -1)
	{
		this->bundle() << uint8( subMsgID );
	}

	return this->bundle();
}


/**
 *	This method starts a message that will be sent to a server entity.
 */
BinaryOStream * ServerConnection::startServerEntityMessage( int methodID,
		EntityID entityID, bool isForBaseEntity )
{
	if (entityID == id_)
	{
		if (isForBaseEntity)
		{
			return &this->startBasePlayerMessage( methodID );
		}
		else
		{
			return &this->startCellPlayerMessage( methodID );
		}
	}
	else if (!isForBaseEntity)
	{
		return &this->startCellEntityMessage( methodID, entityID );
	}
	else
	{
		ERROR_MSG( "Can only call base methods on the connected entity\n" );
		return NULL;
	}
}


/*
 *	Override from Mercury::ChannelListener.
 */
void ServerConnection::onChannelGone( Mercury::Channel & channel )
{
	ERROR_MSG( "ServerConnection::onChannelGone(%d): "
			"Disconnecting due to dead channel( %s ).\n",
		id_, channel.c_str() );

	// Channel has been destroyed, so there is no need to inform
	// the server and condemn the channel.
	this->disconnect( /* informServer */ false,
		/* shouldCondemnChannel */ false );
}


/**
 *	This method processes all pending network messages. They are passed to the
 *	input handler that was specified in logOnComplete.
 *
 *	@return	Returns true if any packets were processed.
 */
bool ServerConnection::processInput()
{
	PROFILER_SCOPED( processServerInput );
	// process any pending packets
	// (they may not be for us in a multi servconn environment)
	bool gotAnyPackets = false;
	bool gotAPacket = false;

	do
	{
		// Check packetsIn separately as it changes in the network dispatcher.
		uint numPackets = this->packetsIn();

		gotAPacket = (dispatcher_.processOnce( false ) != 0);
		gotAPacket |= (this->packetsIn() != numPackets);

		gotAnyPackets |= gotAPacket;
	}
	while (gotAPacket);

	// Don't bother collecting statistics if we're not online.
	if (!this->isOnline())
	{
		return gotAnyPackets;
	}

	// see how long that processing took
	if (gotAnyPackets)
	{
		uint64 currTimeStamp = timestamp();

		if (lastReceiveTime_ != 0)
		{
			uint64 delta = (currTimeStamp - lastReceiveTime_)
							* uint64( 1000 ) / stampsPerSecond();
			int deltaInMS = int( delta );

			if (deltaInMS > PACKET_DELTA_WARNING_THRESHOLD)
			{
				WARNING_MSG( "ServerConnection::processInput(%d): "
						"There were %d ms between packets\n",
					id_, deltaInMS );
			}
		}

		lastReceiveTime_ = currTimeStamp;
	}

	return gotAnyPackets;
}


/**
 *	This method handles an entity method call from the server.
 */
void ServerConnection::entityMethod( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & stream )
{
	int exposedMethodID =
		ClientInterface::Range::entityMethodRange.exposedIDFromMsgID(
				header.identifier, stream, maxClientMethodCount_ );

	pHandler_->onEntityMethod( selectedEntityID_, exposedMethodID, stream );
}


/**
 *	This method handles an entity property update from the server.
 */
void ServerConnection::entityProperty( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & stream )
{
	int exposedPropertyID =
		ClientInterface::Range::entityPropertyRange.exposedIDFromMsgID(
				header.identifier );
	pHandler_->onEntityProperty( selectedEntityID_, exposedPropertyID, stream );
}


/**
 *	This method handles a nested entity property update from the server.
 */
void ServerConnection::nestedEntityProperty( BinaryIStream & stream )
{
	pHandler_->onNestedEntityProperty( selectedEntityID_, stream, false );
}


/**
 *	This method handles an update to a slice sent from the server.
 */
void ServerConnection::sliceEntityProperty( BinaryIStream & stream )
{
	pHandler_->onNestedEntityProperty( selectedEntityID_, stream, true );
}


/**
 *	This method calculates the streamSize of an entity method message.
 *
 *	@param msgID		Message Id.
 */
int ServerConnection::entityMethodGetStreamSize(
		Mercury::MessageID msgID ) const
{
	MF_ASSERT( pHandler_ );

	const ExposedMethodMessageRange & range =
		ClientInterface::Range::entityMethodRange;

	int exposedMethodID = range.simpleExposedIDFromMsgID( msgID );

	if (range.needsSubID( exposedMethodID, maxClientMethodCount_))
	{
		return -static_cast<int>( Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE );
	}

	return pHandler_->getEntityMethodStreamSize( selectedEntityID_,
			exposedMethodID, isEarly_ );
}


/**
 *	This method calculates the streamSize of an entity property message.
 *
 *	@param msgID		Message Id.
 */
int ServerConnection::entityPropertyGetStreamSize(
		Mercury::MessageID msgID ) const
{
	int exposedPropertyID =
		ClientInterface::Range::entityPropertyRange.exposedIDFromMsgID( msgID );
	MF_ASSERT( pHandler_ );

	return pHandler_->getEntityPropertyStreamSize( selectedEntityID_,
			exposedPropertyID, isEarly_ );
}


// -----------------------------------------------------------------------------
// Section: avatarUpdate and related message handlers
// -----------------------------------------------------------------------------


#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
/**
 *	This method handles the relativePositionReference message. It is used to
 *	indicate the position that should be used as the base for future relative
 *	positions.
 */
void ServerConnection::relativePositionReference(
	const ClientInterface::relativePositionReferenceArgs & args )
{
	referencePosition_ =
		::BW_NAMESPACE calculateReferencePosition( sentPositions_[ args.sequenceNumber ] );
}


/**
 *	This method handles the relativePosition message. It is used to indicate the
 *	position that should be used as the base for future relative positions.
 */
void ServerConnection::relativePosition(
		const ClientInterface::relativePositionArgs & args )
{
	referencePosition_ = args.position;
}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */


/**
 *	This method indicates that the vehicle an entity is on has changed.
 */
void ServerConnection::setVehicle(
	const ClientInterface::setVehicleArgs & args )
{
	this->setVehicle( args.passengerID, args.vehicleID );
}


/**
 *	This method changes the vehicle an entity is on has changed.
 */
void ServerConnection::setVehicle( EntityID passengerID, EntityID vehicleID )
{
	if (vehicleID)
	{
		passengerToVehicle_[ passengerID ] = vehicleID;
	}
	else
	{
		passengerToVehicle_.erase( passengerID );
	}
}


/**
 *	This method handles a message from the server setting the "selected"
 *	entity that subsequent messages will be delivered to.
 */
void ServerConnection::selectAliasedEntity(
		const ClientInterface::selectAliasedEntityArgs & args )
{
	selectedEntityID_ = idAlias_[ args.idAlias ];
}


/**
 *	This method handles a message from the server setting the "selected"
 *	entity that subsequent messages will be delivered to.
 */
void ServerConnection::selectEntity(
		const ClientInterface::selectEntityArgs & args )
{
	selectedEntityID_ = args.id;
}


/**
 *	This method handles a message from the server setting the "selected"
 *	entity that subsequent messages will be delivered to.
 */
void ServerConnection::selectPlayerEntity()
{
	selectedEntityID_ = id_;
}


/**
 *	This method handles a message from the server with a volatile entity
 *	position, and also sets the "selected" entity to be that entity. The
 *	position is detailed enough to be used as a reference position if possible.
 */
void ServerConnection::avatarUpdateNoAliasDetailed(
	const ClientInterface::avatarUpdateNoAliasDetailedArgs & args )
{
	EntityID id = args.id;

	selectedEntityID_ = id;

	/* Ignore updates from controlled entities */
	if (this->isControlledLocally( id ))	
	{
		return;
	}

	EntityID vehicleID = this->getVehicleID( id );

	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMoveWithError( id, spaceID_, vehicleID,
			args.position, Vector3::zero(), args.dir.yaw,
			args.dir.pitch, args.dir.roll, /* isVolatile */ true );
	}

	this->detailedPositionReceived( id, spaceID_, vehicleID, args.position );
}


/**
 *	This method handles a message from the server with a volatile player
 *	position, and also sets the "selected" entity to be the player. The
 *	position is detailed enough to be used as a reference position if possible.
 */
void ServerConnection::avatarUpdateAliasDetailed(
	const ClientInterface::avatarUpdateAliasDetailedArgs & args )
{
	EntityID id = idAlias_[ args.idAlias ];

	selectedEntityID_ = id;

	/* Ignore updates from controlled entities */
	if (this->isControlledLocally( id ))	
	{
		return;
	}

	EntityID vehicleID = this->getVehicleID( id );

	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMoveWithError( id, spaceID_, vehicleID,
			args.position, Vector3::zero(), args.dir.yaw,
			args.dir.pitch, args.dir.roll, /* isVolatile */ true );
	}

	this->detailedPositionReceived( id, spaceID_, vehicleID, args.position );
}


/**
 *	This method handles a message from the server with a volatile player
 *	position, and also sets the "selected" entity to be the player. The
 *	position is detailed enough to be used as a reference position if possible.
 */
void ServerConnection::avatarUpdatePlayerDetailed(
	const ClientInterface::avatarUpdatePlayerDetailedArgs & args )
{
	EntityID id = id_;

	selectedEntityID_ = id;

	/* Ignore updates from controlled entities */
	if (this->isControlledLocally( id ))	
	{
		return;
	}

	EntityID vehicleID = this->getVehicleID( id );

	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMoveWithError( id, spaceID_, vehicleID,
			args.position, Vector3::zero(), args.dir.yaw,
			args.dir.pitch, args.dir.roll, /* isVolatile */ true );
	}

	this->detailedPositionReceived( id, spaceID_, vehicleID, args.position );
}


#if VOLATILE_POSITIONS_ARE_ABSOLUTE
#define AVATAR_UPDATE_GET_POS_FullPos										\
		args.position.unpackXYZ( pos.x, pos.y, pos.z, packedXZScale_ );		\
		args.position.getXYZError( posError.x, posError.y, posError.z,		\
				packedXZScale_ );											\

#define AVATAR_UPDATE_GET_POS_OnGround										\
		pos.y = NO_POSITION;												\
		args.position.unpackXZ( pos.x, pos.z, packedXZScale_ );				\
		args.position.getXZError( posError.x, posError.z, packedXZScale_ );	\

#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
#define AVATAR_UPDATE_GET_POS_ORIGIN										\
		const Position3D & originPos =										\
			(vehicleID == NULL_ENTITY_ID) ? referencePosition_ :			\
											Position3D::zero();				\

#define AVATAR_UPDATE_GET_POS_FullPos										\
		AVATAR_UPDATE_GET_POS_ORIGIN										\
		args.position.unpackXYZ( pos.x, pos.y, pos.z, packedXZScale_ );		\
		args.position.getXYZError( posError.x, posError.y, posError.z,		\
				packedXZScale_ );											\
		pos += originPos;													\

#define AVATAR_UPDATE_GET_POS_OnGround										\
		AVATAR_UPDATE_GET_POS_ORIGIN										\
		pos.y = NO_POSITION;												\
		args.position.unpackXZ( pos.x, pos.z, packedXZScale_ );				\
		args.position.getXZError( posError.x, posError.z, packedXZScale_ );	\
		pos.x += originPos.x;												\
		pos.z += originPos.z;												\

#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

#define AVATAR_UPDATE_GET_POS_NoPos											\
		pos.set( NO_POSITION, NO_POSITION, NO_POSITION );					\

#define AVATAR_UPDATE_GET_DIR_YawPitchRoll									\
		float yaw = NO_DIRECTION, pitch = NO_DIRECTION, roll = NO_DIRECTION;\
		args.dir.get( yaw, pitch, roll );									\

#define AVATAR_UPDATE_GET_DIR_YawPitch										\
		float yaw = NO_DIRECTION, pitch = NO_DIRECTION, roll = NO_DIRECTION;\
		args.dir.get( yaw, pitch );											\

#define AVATAR_UPDATE_GET_DIR_Yaw											\
		float yaw = NO_DIRECTION, pitch = NO_DIRECTION, roll = NO_DIRECTION;\
		args.dir.get( yaw );												\

#define AVATAR_UPDATE_GET_DIR_NoDir											\
		float yaw = NO_DIRECTION, pitch = NO_DIRECTION, roll = NO_DIRECTION;\

#define AVATAR_UPDATE_GET_ID_NoAlias	args.id;
#define AVATAR_UPDATE_GET_ID_Alias		idAlias_[ args.idAlias ];

#define IMPLEMENT_AVUPMSG( ID, POS, DIR )									\
void ServerConnection::avatarUpdate##ID##POS##DIR(							\
		const ClientInterface::avatarUpdate##ID##POS##DIR##Args & args )	\
{																			\
	EntityID id = AVATAR_UPDATE_GET_ID_##ID									\
																			\
	selectedEntityID_ = id;													\
																			\
	/* Ignore updates from controlled entities */							\
	if (this->isControlledLocally( id ))									\
	{																		\
		return;																\
	}																		\
																			\
	if (pHandler_ != NULL)													\
	{																		\
		Position3D pos;														\
		Vector3 posError( 0.f, 0.f, 0.f );									\
																			\
		EntityID vehicleID = this->getVehicleID( id );						\
																			\
		AVATAR_UPDATE_GET_POS_##POS											\
																			\
		AVATAR_UPDATE_GET_DIR_##DIR											\
																			\
		pHandler_->onEntityMoveWithError( id, spaceID_, vehicleID,			\
			pos, posError, yaw, pitch, roll, /* isVolatile */ true );		\
	}																		\
}


IMPLEMENT_AVUPMSG( NoAlias, FullPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, NoDir )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, NoDir )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, NoDir )
IMPLEMENT_AVUPMSG( Alias, FullPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, FullPos, YawPitch )
IMPLEMENT_AVUPMSG( Alias, FullPos, Yaw )
IMPLEMENT_AVUPMSG( Alias, FullPos, NoDir )
IMPLEMENT_AVUPMSG( Alias, OnGround, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, OnGround, YawPitch )
IMPLEMENT_AVUPMSG( Alias, OnGround, Yaw )
IMPLEMENT_AVUPMSG( Alias, OnGround, NoDir )
IMPLEMENT_AVUPMSG( Alias, NoPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, NoPos, YawPitch )
IMPLEMENT_AVUPMSG( Alias, NoPos, Yaw )
IMPLEMENT_AVUPMSG( Alias, NoPos, NoDir )


/**
 *	This method handles a detailed position and direction update.
 */
void ServerConnection::detailedPosition(
	const ClientInterface::detailedPositionArgs & args )
{
	/* Ignore updates from controlled entities */
	if (this->isControlledLocally( selectedEntityID_ ))
	{
		return;
	}

	EntityID vehicleID = this->getVehicleID( selectedEntityID_ );

	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMoveWithError( selectedEntityID_, spaceID_,
			vehicleID, args.position, Vector3::zero(), args.direction.yaw,
			args.direction.pitch, args.direction.roll, /* isVolatile */ false );
	}

	this->detailedPositionReceived(
		selectedEntityID_, spaceID_, vehicleID, args.position );
}


/**
 *	This method handles a forced position and direction update.
 *	This is when an update is being forced back for an (ordinarily)
 *	client controlled entity, including for the player. Usually this is
 *	due to a physics correction from the server, but it could be for any
 *	reason decided by the server (e.g. server-initiated teleport).
 */
void ServerConnection::forcedPosition(
	const ClientInterface::forcedPositionArgs & args )
{
	this->setVehicle( args.id, args.vehicleID );

	if (args.id == id_)
	{
		if ((spaceID_ != NULL_SPACE_ID) &&
				(spaceID_ != args.spaceID) &&
				(pHandler_ != NULL))
		{
			pHandler_->spaceGone( spaceID_ );
		}

		spaceID_ = args.spaceID;

		if (this->isControlledLocally( args.id ))
		{
			this->bundle().startMessage(
				BaseAppExtInterface::ackPhysicsCorrection );
		}
	}
	else if (this->isControlledLocally( args.id ))
	{
		BaseAppExtInterface::ackWardPhysicsCorrectionArgs ackArgs;

		ackArgs.ward = BW_HTONL( args.id );

		this->bundle().sendMessage( ackArgs );
	}

	// finally tell the handler about it
	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMoveWithError( args.id, args.spaceID, args.vehicleID,
			args.position, Vector3::zero(), args.direction.yaw,
			args.direction.pitch, args.direction.roll, /* isVolatile */ false );
	}
}


/**
 *	The server is telling us whether or not we are controlling this entity
 */
void ServerConnection::controlEntity(
	const ClientInterface::controlEntityArgs & args )
{
	if (args.on)
	{
		controlledEntities_.insert( args.id );
	}
	else
	{
		controlledEntities_.erase( args.id );
	}

	// tell the message handler about it
	if (pHandler_ != NULL)
	{
		pHandler_->onEntityControl( args.id, args.on );
	}
}


/**
 *	This method is called when a detailed position for an entity has been
 *	received.
 */
void ServerConnection::detailedPositionReceived( EntityID id,
	SpaceID spaceID, EntityID vehicleID, const Position3D & position )
{
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	if ((id == id_) && (vehicleID == NULL_ENTITY_ID))
	{
		referencePosition_ = ::BW_NAMESPACE calculateReferencePosition( position );
	}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


// -----------------------------------------------------------------------------
// Section: Statistics methods
// -----------------------------------------------------------------------------

namespace // (anonymous)
{
	void (*s_bandwidthFromServerMutator)( int bandwidth ) = NULL;
} // end namespace (anonymous)


void setBandwidthFromServerMutator( void (*mutatorFn)( int bandwidth ) )
{
	s_bandwidthFromServerMutator = mutatorFn;
}


/**
 *	This method gets the bandwidth that this server connection should receive
 *	from the server.
 *
 *	@return		The current downstream bandwidth in bits per second.
 */
int ServerConnection::bandwidthFromServer() const
{
	return bandwidthFromServer_;
}


/**
 *	This method sets the bandwidth that this server connection should receive
 *	from the server.
 *
 *	@param bandwidth	The bandwidth in bits per second.
 */
void ServerConnection::bandwidthFromServer( int bandwidth )
{
	if (s_bandwidthFromServerMutator == NULL)
	{
		ERROR_MSG( "ServerConnection::bandwidthFromServer: Cannot comply "
			"since no mutator set with 'setBandwidthFromServerMutator'\n" );
		return;
	}

	const int MIN_BANDWIDTH = 0;
	const int MAX_BANDWIDTH = PACKET_MAX_SIZE * NETWORK_BITS_PER_BYTE * 10 / 2;

	bandwidth = Math::clamp( MIN_BANDWIDTH, bandwidth, MAX_BANDWIDTH );

	(*s_bandwidthFromServerMutator)( bandwidth );

	// don't set it now - wait to hear back from the server
	//bandwidthFromServer_ = bandwidth;
}


/**
 *	This method returns the number of bits received per second.
 */
double ServerConnection::bpsIn() const
{
	return pInterface_ ? 
		pInterface_->receivingStats().bitsPerSecond() :
		0.0;
}


/**
 *	This method returns the number of bits sent per second.
 */
double ServerConnection::bpsOut() const
{
	return pInterface_ ? 
		pInterface_->sendingStats().bitsPerSecond() :
		0.0;
}


/**
 *	This method returns the number of packets received per second.
 */
double ServerConnection::packetsPerSecondIn() const
{
	return pInterface_ ? 
		pInterface_->receivingStats().packetsPerSecond() :
		0.0;
}


/**
 *	This method returns the number of packets sent per second.
 */
double ServerConnection::packetsPerSecondOut() const
{
	return pInterface_ ? 
		pInterface_->sendingStats().packetsPerSecond() :
		0.0;
}


/**
 *	This method returns the number of messages received per second.
 */
double ServerConnection::messagesPerSecondIn() const
{
	return pInterface_ ? 
		pInterface_->receivingStats().messagesPerSecond() :
		0.0;
}


/**
 *	This method returns the number of messages sent per second.
 */
double ServerConnection::messagesPerSecondOut() const
{
	return pInterface_ ? 
		pInterface_->sendingStats().messagesPerSecond() :
		0.0;
}


/**
 *	This method returns the percentage of movement bytes received.
 */
double ServerConnection::movementBytesPercent() const
{
	return this->getRatePercent( numMovementBytes_ );
}

/**
 *	This method returns the percentage of non-movement bytes received
 */
double ServerConnection::nonMovementBytesPercent() const
{
	return this->getRatePercent( numNonMovementBytes_ );
}


/**
 *	This method returns the percentage of overhead bytes received
 */
double ServerConnection::overheadBytesPercent() const
{
	return this->getRatePercent( numOverheadBytes_ );
}


/**
 *	This method returns the percent that the input stat makes up of
 *	total bandwidth.
 */
double ServerConnection::getRatePercent( const Stat & stat ) const
{
	double total =
		numMovementBytes_.getRateOfChange() +
		numNonMovementBytes_.getRateOfChange() +
		numOverheadBytes_.getRateOfChange();

	if (total > 0.0)
	{
		return stat.getRateOfChange()/total * 100.0;
	}
	else
	{
		return 0.0;
	}
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with movement messages.
 */
int ServerConnection::movementBytesTotal() const
{
	return numMovementBytes_.total();
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with non-movement messages.
 */
int ServerConnection::nonMovementBytesTotal() const
{
	return numNonMovementBytes_.total();
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with packet overhead.
 */
int ServerConnection::overheadBytesTotal() const
{
	return numOverheadBytes_.total();
}


/**
 *	This method returns the number of packets received by the current network
 *	interface.
 */
uint32 ServerConnection::packetsIn() const
{
	return pInterface_ ? pInterface_->receivingStats().numPacketsReceived() : 0;
}


/**
 *	This method is an override from TimerHandler.
 */
void ServerConnection::handleTimeout( TimerHandle handle, void * arg )
{
	this->updateStats();
}


/**
 *	This method updates the timing statistics of the server connection.
 */
void ServerConnection::updateStats()
{
	if (pInterface_ == NULL)
	{
		return;
	}

	const Mercury::NetworkInterface & iface = this->networkInterface();
	const Mercury::PacketReceiverStats & stats = iface.receivingStats();
	const Mercury::InterfaceTable & ifTable = iface.interfaceTable();

	uint32 newMovementBytes = 0;

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	newMovementBytes += ifTable[
		ClientInterface::relativePositionReference.id() ].numBytesReceived();
	newMovementBytes += ifTable[
		ClientInterface::relativePosition.id() ].numBytesReceived();
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	newMovementBytes += ifTable[
		ClientInterface::setVehicle.id() ].numBytesReceived();
	newMovementBytes += ifTable[
		ClientInterface::forcedPosition.id() ].numBytesReceived();

	for (int i = FIRST_AVATAR_UPDATE_MESSAGE;
			i <= LAST_AVATAR_UPDATE_MESSAGE; i++)
	{
		newMovementBytes += ifTable[ i ].numBytesReceived();
	}

	numMovementBytes_.setTotal( newMovementBytes );
	numOverheadBytes_.setTotal( stats.numOverheadBytesReceived() );
	numNonMovementBytes_.setTotal( stats.numBytesReceived() -
		numOverheadBytes_.total() - newMovementBytes );

	numMovementBytes_.tick( UPDATE_STATS_PERIOD );
	numNonMovementBytes_.tick( UPDATE_STATS_PERIOD );
	numOverheadBytes_.tick( UPDATE_STATS_PERIOD );
}


/**
 *	This method sends the current bundle to the server if sufficient time has
 *	passed since the last send, according to the minimum send interval.
 */
void ServerConnection::sendIfNecessary()
{
	if (this->isOffline())
	{
		return;
	}

	if ((*pTime_ - lastSendTime_) > minSendInterval_)
	{
		this->send();
	}
}


/**
 *	This method sends the current bundle to the server immediately.
 */
void ServerConnection::send()
{
	// get out now if we are not connected
	if (this->isOffline())
	{
		return;
	}

	// record the time we last did a send
	if (pTime_)
	{
		double sinceLastSendTime = *pTime_ - lastSendTime_;
		if (isZero( lastSendTime_ ) &&
				(sinceLastSendTime > sendTimeReportThreshold_))
		{
			WARNING_MSG( "ServerConnection::send(%d): "
					"Time since last send to server: %.0fms\n",
				id_,
				sinceLastSendTime * 1e3 );
		}
		lastSendTime_ = *pTime_;
	}

	// get the channel to send the bundle
	this->channel().send();

}


/*
 *	Override from Mercury::ChannelListener.
 */
void ServerConnection::onChannelSend( Mercury::Channel & channel )
{
	static const int OVERFLOW_LIMIT = 1024;

	if (&channel != &(this->channel()))
	{
		return;
	}

	// TODO: #### Make a better check that is dependent on both the window
	// size and time since last heard from the server.
	if (!channel.isTCP() &&
			(static_cast< Mercury::UDPChannel & >( channel ).sendWindowUsage() >
				OVERFLOW_LIMIT))
	{
		WARNING_MSG( "ServerConnection::onChannelSend(%d): "
				"Disconnecting since channel %s has overflowed.\n",
			id_, channel.c_str() );

		channel.destroy();
	}
}


/**
 * 	This method primes outgoing bundles with the authenticate message once it
 * 	has been received.
 */
void ServerConnection::primeBundle( Mercury::Bundle & bundle )
{
	if (sessionKey_)
	{
		BaseAppExtInterface::authenticateArgs authenticateArgs;

		authenticateArgs.key = sessionKey_;

		bundle.sendMessage( authenticateArgs, Mercury::RELIABLE_PASSENGER );
	}
}


/**
 * 	This method returns the number of unreliable messages that are streamed on
 *  by primeBundle().
 */
int ServerConnection::numUnreliableMessages() const
{
	return sessionKey_ ? 1 : 0;
}


/**
 *	This method requests the server to send update information for the entity
 *	with the input id.
 *
 *  @param id		ID of the entity whose update is requested.
 *	@param stamps	A vector containing the known cache event stamps. If none
 *					are known, stamps is empty.
 */
void ServerConnection::requestEntityUpdate( EntityID id,
	const CacheStamps & stamps )
{
	if (this->isOffline())
		return;

	this->bundle().startMessage( BaseAppExtInterface::requestEntityUpdate );
	this->bundle() << id;

	CacheStamps::const_iterator iter = stamps.begin();

	while (iter != stamps.end())
	{
		this->bundle() << (*iter);

		iter++;
	}
}


/**
 *	This method returns the approximate round-trip time to the server.
 */
float ServerConnection::latency() const
{
	return pChannel_ ? float( pChannel_->roundTripTimeInSeconds() ) : 0.f;
}


/**
 *	This method sets the artificial loss parameters for network interfaces
 *	managed by this ServerConnection.
 *
 *	@param lossRatio	The loss ratio to set.
 *	@param minLatency	The minimum latency.
 *	@param maxLatency	The maximum latency.
 */
void ServerConnection::setArtificialLoss( float lossRatio, float minLatency,
	float maxLatency )
{
	packetLossParameters_.set( lossRatio, minLatency, maxLatency );

	if (pInterface_ != NULL)
	{
		packetLossParameters_.apply( *pInterface_ );
	}
}


/**
 *	This method copies the artificial loss parameters from another 
 *	PacketLossParameters.
 *
 *	@param parameters	A PacketLossParameters instance 
 */
void ServerConnection::setArtificialLoss( 
	const Mercury::PacketLossParameters & parameters )
{
	packetLossParameters_ = parameters;

	if (pInterface_ != NULL)
	{
		packetLossParameters_.apply( *pInterface_ );
	}
}


// -----------------------------------------------------------------------------
// Section: Server Timing code
// -----------------------------------------------------------------------------
// TODO:PM This section is really just here to give a better time to the
// filtering. This should be reviewed.


/**
 *	This method returns the server time estimate based on the input client time.
 */
double ServerConnection::serverTime( double clientTime ) const
{
	return serverTimeHandler_.serverTime( clientTime );
}


/**
 *	This method returns the server time associated with the last packet that was
 *	received from the server.
 */
double ServerConnection::clientTimeOfLastMessage() const
{
	return serverTimeHandler_.clientTimeOfLastMessage();
}


/**
 * 	This method returns the game time associated with the last packet that was
 * 	received from the server.
 */
GameTime ServerConnection::gameTimeOfLastMessage() const
{
	return serverTimeHandler_.gameTimeOfLastMessage();
}


/**
 *	The constructor for ServerTimeHandler.
 */
ServerConnection::ServerTimeHandler::ServerTimeHandler() :
	tickByte_( 0 ),
	hasReceivedGameTime_( false ),
	timeAtSequenceStart_( 0.0 ),
	gameTimeAtSequenceStart_( 0 ),
	lastReturnedServerTime_( 0.0 ),
	lastPacketWasOrdered_( true ),
	lastOrderedTickByte_( tickByte_ )
{
}


/**
 * 	This method is called when the server sends a new gametime.
 * 	This should be after the sequence number for the current packet
 * 	has been set.
 *
 *	@param newGameTime	The current game time in ticks.
 */
void ServerConnection::ServerTimeHandler::gameTime( GameTime newGameTime,
		double currentTime )
{
	hasReceivedGameTime_ = true;
	tickByte_ = uint8( newGameTime );
	gameTimeAtSequenceStart_ = newGameTime - tickByte_;
	timeAtSequenceStart_ = currentTime -
		double(tickByte_) / pConnection_->updateFrequency();
}


/**
 *	This method is called when a new tick sync message is received from the
 *	server. It is used to synchronise between client and server time.
 *
 *	@param newSeqNum	The sequence number just received. This increases by one
 *						for each packets and packets should be received at 10Hz.
 *
 *	@param currentTime	This is the time that the client currently thinks it is.
 *						We need to pass it in since this file does not have
 *						access to the app file.
 *
 *	@param isEarly		Specifies whether current message is being processed
 *						early (out of order).
 */
void ServerConnection::ServerTimeHandler::tickSync( uint8 newSeqNum,
		double currentTime, bool isEarly )
{
	const double SEQUENCE_PERIOD = 256.0 / pConnection_->updateFrequency();
	const int SEQUENCE_PERIOD_INT = 256;

	// Have we started yet?

	if (!hasReceivedGameTime_)
	{
		// The first one is always like this.
		// INFO_MSG( "ServerTimeHandler::sequenceNumber: "
		//	"Have not received gameTime message yet.\n" );
		return;
	}


	int seqDiff = newSeqNum - tickByte_;

/*
A packet is "ordered" when it is the next anticipated one (i.e. it comes
right after the previous one (and thus it is "not early")).
"Unordered" (or "early") packet is the one which sequence number is greater
than the anticipated.

If a server sends packets A B C D E F G (in that order) and they come
to client reordered ("-" means tick, time increases down,
O - ordered, U - unordered):
- A
- B (O)
-
-
- E (U)
- D (U)
- C (O) D (O) E (O)
(When the ordered C comes, all the (already received) sequential packets are
processed at once.)
- F (O)

Possible transitions Ordered vs Unordered packets:
1) Ordered to Unordered: 
	The following condition must be true:
		tickByte_ < newSeqNum,
	otherwise we are wrapped around,
	and add SEQUENCE_PERIOD/SEQUENCE_PERIOD_INT 

2) Ordered to Ordered:
	The following condition must be true:
		tickByte_ < newSeqNum,
	otherwise we are wrapped around,
	and add SEQUENCE_PERIOD/SEQUENCE_PERIOD_INT 

3) Unordered to Ordered:
	The following condition must be true:
		tickByte_ >= newSeqNum,
	otherwise we are wrapped around going backward,
	and sub SEQUENCE_PERIOD/SEQUENCE_PERIOD_INT 

4) Unordered to Unordered:
	Both the following conditions must be true:
		LastOrderedPacket < tickByte_
		LastOrderedPacket < newSeqNum,
	otherwise we are wrapped around going backward,
	or forward of tickbyte_/newSeqNum,
	and sub/add SEQUENCE_PERIOD_INT
	to tickbyte (having extended to 16 bit value) and/or
	to newSeqNum (having extended to 16 bit value)

	a) if normalised tickbyte >= 256 and normalized newSeqNum < 256,
		we wrapped around going backward
		and need to sub SEQUENCE_PERIOD/SEQUENCE_PERIOD_INT 

	b) if normalised tickbyte < 256 and normalized newSeqNum >= 256,
		we wrapped around going forward
		and need to add SEQUENCE_PERIOD/SEQUENCE_PERIOD_INT 
*/
	bool thisPacketIsOrdered = !isEarly;

	// Transition 1, saving the
	// tickSync only:
	if (lastPacketWasOrdered_ && !thisPacketIsOrdered)
	{
		//We got last ordered packet, next is unordered,
		//let's save it
		lastOrderedTickByte_ = tickByte_;
	}
	// Transition 1/2 will be handled further below
	// Transition 3:
	else if (!lastPacketWasOrdered_ && thisPacketIsOrdered)
	{
		if (seqDiff > 0)
		{
			//wrapped around backward
			timeAtSequenceStart_ -= SEQUENCE_PERIOD;
			gameTimeAtSequenceStart_ -= SEQUENCE_PERIOD_INT;
		}
	}
	// Transition 4:
	else if (!lastPacketWasOrdered_ && !thisPacketIsOrdered)
	{
		uint16 unorderedTickByte = tickByte_;
		uint16 unorderedInput = newSeqNum;

		//normalise newSeqNum
		if (unorderedInput < lastOrderedTickByte_)
		{
			unorderedInput += SEQUENCE_PERIOD_INT;
		}

		//normalise TickByte
		if (unorderedTickByte < lastOrderedTickByte_)
		{
			unorderedTickByte += SEQUENCE_PERIOD_INT;
		}

		// Transition 4-a:
		if ((unorderedInput >= SEQUENCE_PERIOD_INT) &&
			(unorderedTickByte < SEQUENCE_PERIOD_INT))
		{
			timeAtSequenceStart_ += SEQUENCE_PERIOD;
			gameTimeAtSequenceStart_ += SEQUENCE_PERIOD_INT;
		}
		// Transition 4-b:
		else if ((unorderedInput < SEQUENCE_PERIOD_INT) &&
				 (unorderedTickByte >= SEQUENCE_PERIOD_INT))
		{
			timeAtSequenceStart_ -= SEQUENCE_PERIOD;
			gameTimeAtSequenceStart_ -= SEQUENCE_PERIOD_INT;
		}
	}

	if (seqDiff < 0)
	{
		// If the last packet was ordered, we don't expect the sequence to go
		// backwards, so we must have wrapped the tickByte.
		if (lastPacketWasOrdered_) 
		{
			// we've wrapped around going forward
			timeAtSequenceStart_ += SEQUENCE_PERIOD;
			gameTimeAtSequenceStart_ += SEQUENCE_PERIOD_INT;
		}
	}

	lastPacketWasOrdered_ = thisPacketIsOrdered;
	tickByte_ = newSeqNum;

	if (seqDiff == 0)
	{
		//Ordered packet comes just after the same tick unordered 
		//Probably some packets were lost
		return;
	}

	// Want to adjust the time so that the client does not get too out of sync.
	double timeError = currentTime - this->clientTimeOfLastMessage();

	const double MAX_TIME_ERROR = 0.05;
	const double MAX_TIME_ADJUST = 0.005;

	if (timeError > MAX_TIME_ERROR)
	{
		timeAtSequenceStart_ += MF_MIN( timeError, MAX_TIME_ADJUST );
	}
	else if (-timeError > MAX_TIME_ERROR)
	{
		timeAtSequenceStart_ += MF_MAX( timeError, -MAX_TIME_ADJUST );
	}
}


/**
 *	This method returns the time that this client thinks the server is at.
 */
double
ServerConnection::ServerTimeHandler::serverTime( double clientTime ) const
{
	if (!hasReceivedGameTime_)
	{
		return 0.0;
	}

	double srvTime =(gameTimeAtSequenceStart_ / pConnection_->updateFrequency()) +
		(clientTime - timeAtSequenceStart_);
	if (srvTime < lastReturnedServerTime_)
	{
		return lastReturnedServerTime_;
	}
	lastReturnedServerTime_ = srvTime;
	return srvTime;
}


/**
 *	This method returns the server time associated with the last packet that was
 *	received from the server.
 */
double ServerConnection::ServerTimeHandler::clientTimeOfLastMessage() const
{
	if (!hasReceivedGameTime_)
	{
		return 0.0;
	}

	return timeAtSequenceStart_ +
		double(tickByte_) / pConnection_->updateFrequency();
}


/**
 * 	This method returns the game time of the current message.
 */
GameTime ServerConnection::ServerTimeHandler::gameTimeOfLastMessage() const
{
	return gameTimeAtSequenceStart_ + tickByte_;
}


#if ENABLE_WATCHERS
/**
 *	This method initialises the watcher information for this object.
 */
WatcherPtr ServerConnection::pWatcher()
{
	DirectoryWatcherPtr pWatcher = NULL;
	if (!pWatcher.hasObject())
	{
		pWatcher = new DirectoryWatcher();
		
		pWatcher->addChild( "Desired bps in", 
			makeWatcher( &ServerConnection::bandwidthFromServer ) );
		pWatcher->addChild( "bps in",
			makeWatcher( &ServerConnection::bpsIn ) );
		pWatcher->addChild( "bps out",
			makeWatcher( &ServerConnection::bpsOut ) );
		pWatcher->addChild( "PacketsSec in",
			makeWatcher( &ServerConnection::packetsPerSecondIn ) );
		pWatcher->addChild( "PacketsSec out",
			makeWatcher( &ServerConnection::packetsPerSecondIn ) );
		
		pWatcher->addChild( "Messages in",
			makeWatcher( &ServerConnection::messagesPerSecondIn ) );
		pWatcher->addChild( "Messages out",
			makeWatcher( &ServerConnection::messagesPerSecondOut ) );

		pWatcher->addChild( "Expected Freq", 
			makeWatcher( &ServerConnection::updateFrequency ) );
		
		pWatcher->addChild( "Game Time",
			makeWatcher( &ServerConnection::gameTimeOfLastMessage ) );

		pWatcher->addChild( "Movement pct",
			makeWatcher( &ServerConnection::movementBytesPercent ) );
		pWatcher->addChild( "Non-move pct",
			makeWatcher( &ServerConnection::nonMovementBytesPercent ) );
		pWatcher->addChild( "Overhead pct",
			makeWatcher( &ServerConnection::overheadBytesPercent ) );

		pWatcher->addChild( "Movement total",
			makeWatcher( &ServerConnection::movementBytesTotal ) );
		pWatcher->addChild( "Non-move total",
			makeWatcher( &ServerConnection::nonMovementBytesTotal ) );
		pWatcher->addChild( "Overhead total",
			makeWatcher( &ServerConnection::overheadBytesTotal ) );

		pWatcher->addChild( "Latency",
			makeWatcher( &ServerConnection::latency ) );

		WatcherPtr interfaceWatcher = new BaseDereferenceWatcher( 
			Mercury::NetworkInterface::pWatcher() );

		ServerConnection * pNull = NULL;
		pWatcher->addChild( "Interface", interfaceWatcher,
									 &(pNull->pInterface_) );
	}

	return pWatcher;
}


#endif // ENABLE_WATCHERS


// -----------------------------------------------------------------------------
// Section: Mercury message handlers
// -----------------------------------------------------------------------------

/**
 *	This method authenticates the server to the client. Its use is optional,
 *	and determined by the server that we are connected to upon login.
 */
void ServerConnection::authenticate(
	const ClientInterface::authenticateArgs & args )
{
	if (args.key != sessionKey_)
	{
		ERROR_MSG( "ServerConnection::authenticate: "
				   "Unexpected key! (%x, wanted %x)\n",
				   args.key, sessionKey_ );
		return;
	}
}


/**
 * 	This message handles a bandwidthNotification message from the server.
 */
void ServerConnection::bandwidthNotification(
	const ClientInterface::bandwidthNotificationArgs & args )
{
	// TRACE_MSG( "ServerConnection::bandwidthNotification: %d\n", args.bps);
	bandwidthFromServer_ = args.bps;
}


/**
 *	This method handles the message from the server that informs us how
 *	frequently it is going to send to us.
 */
void ServerConnection::updateFrequencyNotification(
		const ClientInterface::updateFrequencyNotificationArgs & args )
{
	updateFrequency_ = (float)args.hertz;
}


/**
 *	This method handles a tick sync message from the server. It is used as
 *	a timestamp for the messages in the packet.
 */
void ServerConnection::tickSync(
	const ClientInterface::tickSyncArgs & args )
{
	serverTimeHandler_.tickSync( args.tickByte, this->appTime(), isEarly_ );
}


/**
 *	This method handles a setGameTime message from the server.
 *	It is used to adjust the current (server) game time.
 */
void ServerConnection::setGameTime(
	const ClientInterface::setGameTimeArgs & args )
{
	serverTimeHandler_.gameTime( args.gameTime, this->appTime() );
}


/**
 *	This method handles a resetEntities call from the server.
 */
void ServerConnection::resetEntities(
		const ClientInterface::resetEntitiesArgs & args )
{
	this->resetEntities( args.keepPlayerOnBase );
}


/**
 *	This method resets the entities in the AoI.
 */
void ServerConnection::resetEntities( bool keepPlayerOnBase )
{
	// proxy must have received our enableEntities if it is telling
	// us to reset entities (even if we haven't yet received any player
	// creation msgs due to reordering)
	MF_ASSERT_DEV( entitiesEnabled_ );

	// clear existing stale packet
	this->send();

	controlledEntities_.clear();
	passengerToVehicle_.clear();

	// forget about the base player entity too if so indicated
	if (!keepPlayerOnBase)
	{
		id_ = NULL_ENTITY_ID;

		// delete proxy data downloads in progress
		for (uint i = 0; i < dataDownloads_.size(); ++i)
		{
			bw_safe_delete( dataDownloads_[i] );
		}

		dataDownloads_.clear();
		// but not resource downloads as they're non-entity and should continue
	}

	// re-enable entities, which serves to ack the resetEntities
	// and flush/sync the incoming channel
	entitiesEnabled_ = false;
	this->enableEntities();

	isOnGround_ = IS_ON_GROUND_UNKNOWN;

	// and finally tell the client about it so it can clear out
	// all (or nigh all) its entities
	if (pHandler_)
	{
		pHandler_->onEntitiesReset( keepPlayerOnBase );
	}
}


/**
 *	This method handles a createPlayer call from the base.
 */
void ServerConnection::createBasePlayer( BinaryIStream & stream )
{
	// we have new player id
	EntityID playerID = NULL_ENTITY_ID;
	stream >> playerID;

	INFO_MSG( "ServerConnection::createBasePlayer: id %u\n", playerID );

	// this is now our player id
	id_ = playerID;

	EntityTypeID playerType = EntityTypeID(-1);
	stream >> playerType;

	if (pHandler_)
	{	// just get base data here
		pHandler_->onBasePlayerCreate( id_, playerType,
			stream );
	}
}


/**
 *	This method handles a createCellPlayer call from the cell.
 */
void ServerConnection::createCellPlayer( BinaryIStream & stream )
{
	MF_ASSERT( id_ != NULL_ENTITY_ID );
	INFO_MSG( "ServerConnection::createCellPlayer: id %u\n", id_ );

	EntityID vehicleID;
	Position3D pos;
	Direction3D	dir;
	stream >> spaceID_ >> vehicleID >> pos >> packedXZScale_ >> dir;

	MF_ASSERT( packedXZScale_ > 0.f );

	// assume that we control this entity too
	controlledEntities_.insert( id_ );

	this->setVehicle( id_, vehicleID );

	if (pHandler_)
	{
		// just get cell data here
		pHandler_->onCellPlayerCreate( id_,
			spaceID_, vehicleID, pos, dir.yaw, dir.pitch, dir.roll,
			stream );
	}

	this->detailedPositionReceived( id_, spaceID_, vehicleID, pos );

	// The channel to the server is now regular
	if (!this->channel().isTCP())
	{
		Mercury::UDPChannel & udpChannel =
			static_cast< Mercury::UDPChannel & >( this->channel() );
		udpChannel.isLocalRegular( true );
		udpChannel.isRemoteRegular( true );
	}

	isOnGround_ = IS_ON_GROUND_UNKNOWN;
}


/**
 *	This method handles keyed data about a particular space from the server.
 */
void ServerConnection::spaceData( BinaryIStream & stream )
{
	SpaceID spaceID;
	SpaceEntryID spaceEntryID;
	uint16 key;
	BW::string data;

	stream >> spaceID >> spaceEntryID >> key;
	int length = stream.remainingLength();
	data.assign( (char*)stream.retrieve( length ), length );

	TRACE_MSG( "ServerConnection::spaceData(%d): space %u key %hu\n",
		id_, spaceID, key );

	if (pHandler_)
	{
		pHandler_->spaceData( spaceID, spaceEntryID, key, data );
	}
}


/**
 *	This method is the common implementation of enterAoI and enterAoIOnVehicle.
 */
void ServerConnection::enterAoI( EntityID id, IDAlias idAlias,
		EntityID vehicleID )
{
	// Set this even if args.idAlias is NO_ID_ALIAS.
	idAlias_[ idAlias ] = id;

	if (pHandler_)
	{
		const CacheStamps * pStamps = pHandler_->onEntityCacheTest( id );

		if (pStamps)
		{
			this->requestEntityUpdate( id, *pStamps );
		}
		else
		{
			this->requestEntityUpdate( id );
		}
	}
}


/**
 *	This method handles the message from the server that an entity has entered
 *	our Area of Interest (AoI).
 */
void ServerConnection::enterAoI( const ClientInterface::enterAoIArgs & args )
{
	this->enterAoI( args.id, args.idAlias );
}


/**
 *	This method handles the message from the server that an entity has entered
 *	our Area of Interest (AoI).
 */
void ServerConnection::enterAoIOnVehicle(
	const ClientInterface::enterAoIOnVehicleArgs & args )
{
	this->setVehicle( args.id, args.vehicleID );
	this->enterAoI( args.id, args.idAlias, args.vehicleID );
}


/**
 *	This method handles the message from the server that an entity has left our
 *	Area of Interest (AoI).
 */
void ServerConnection::leaveAoI( BinaryIStream & stream )
{
	EntityID id;
	stream >> id;

	// TODO: What if the entity just leaves the AoI and then returns?
	if (controlledEntities_.erase( id ))
	{
		if (pHandler_)
		{
			pHandler_->onEntityControl( id, false );
		}
	}

	if (pHandler_)
	{
		CacheStamps stamps( stream.remainingLength() / sizeof(EventNumber) );

		CacheStamps::iterator iter = stamps.begin();

		while (iter != stamps.end())
		{
			stream >> (*iter);

			iter++;
		}

		pHandler_->onEntityLeave( id, stamps );
	}

	passengerToVehicle_.erase( id );
}


/**
 *	This method handles a createEntity call from the server.
 */
void ServerConnection::createEntity( BinaryIStream & rawStream )
{
	CompressionIStream stream( rawStream );

	EntityID id;
	stream >> id;

	MF_ASSERT_DEV( id != EntityID( -1 ) )	// old-style deprecated hack
	// Connected Entity gets createCellPlayer instead
	MF_ASSERT( id != id_ );

	EntityTypeID type;
	stream >> type;

	Position3D pos( 0.f, 0.f, 0.f );
	PackedYawPitchRoll< /* HALFPITCH */ false > compressedYPR;
	float yaw;
	float pitch;
	float roll;

	stream >> pos >> compressedYPR;

	compressedYPR.get( yaw, pitch, roll );

	EntityID vehicleID = this->getVehicleID( id );

	if (pHandler_)
	{
		pHandler_->onEntityCreate( id, type,
			spaceID_, vehicleID, pos, yaw, pitch, roll,
			stream );
	}

	this->detailedPositionReceived( id, spaceID_, vehicleID, pos );
}


/**
 *	This method handles a createEntityDetailed call from the server.
 */
void ServerConnection::createEntityDetailed( BinaryIStream & rawStream )
{
	CompressionIStream stream( rawStream );

	EntityID id;
	stream >> id;

	// Connected Entity gets createCellPlayer instead
	MF_ASSERT( id != id_ );

	EntityTypeID type;
	stream >> type;

	Position3D pos( 0.f, 0.f, 0.f );
	float yaw;
	float pitch;
	float roll;

	stream >> pos >> yaw >> pitch >> roll;

	EntityID vehicleID = this->getVehicleID( id );

	if (pHandler_)
	{
		pHandler_->onEntityCreate( id, type,
			spaceID_, vehicleID, pos, yaw, pitch, roll,
			stream );
	}

	this->detailedPositionReceived( id, spaceID_, vehicleID, pos );
}


/**
 *	This method handles an updateEntity call from the server.
 */
void ServerConnection::updateEntity( BinaryIStream & stream )
{
	if (pHandler_)
	{
		pHandler_->onEntityProperties( selectedEntityID_, stream );
	}
}


/**
 *	This method handles voice data that comes from another client.
 */
void ServerConnection::voiceData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & stream )
{
	if (pHandler_)
	{
		pHandler_->onVoiceData( srcAddr, stream );
	}
	else
	{
		ERROR_MSG( "ServerConnection::voiceData: "
			"Got voice data before a handler has been set.\n" );
	}
}


/**
 *	This method handles a message from the server telling us that we need to
 *	restore a state back to a previous point.
 */
void ServerConnection::restoreClient( BinaryIStream & stream )
{
	EntityID	id;
	SpaceID		spaceID;
	EntityID	vehicleID;
	Position3D	pos;
	Direction3D	dir;

	stream >> id >> spaceID >> vehicleID >> pos >> dir;

	if (pHandler_)
	{
		this->setVehicle( id, vehicleID );
		pHandler_->onRestoreClient( id, spaceID, vehicleID, pos, dir, stream );
	}
	else
	{
		ERROR_MSG( "ServerConnection::restoreClient(%d): "
				"No handler. Maybe already logged off.\n",
			id_ );
	}


	if (this->isOffline()) return;

	BaseAppExtInterface::restoreClientAckArgs args;
	// TODO: Put on a proper ack id.
	args.id = 0;
	this->bundle() << args;
	this->send();
}


/**
 *	This method is called to handle a request from the server to switch
 *	BaseApps.
 */
void ServerConnection::switchBaseApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const ClientInterface::switchBaseAppArgs & args )
{
	const Mercury::Address & baseAddr = args.baseAddr;
	const bool & shouldResetEntities = args.shouldResetEntities;

	INFO_MSG( "ServerConnection::switchBaseApp(%d): "
			"Client %s switching from %s to %s\n",
		id_,
		pInterface_->address().c_str(),
		pChannel_->c_str(),
		baseAddr.c_str() );

	if (header.replyID)
	{
		pChannel_->bundle().startReply( header.replyID );
		pChannel_->send();
	}

	// Save away the message handler and session key as they are cleared when
	// disconnecting.
	ServerMessageHandler * pHandler = pHandler_;
	SessionKey sessionKey = sessionKey_;

	this->disconnect( /* informServer */ false,
		/* shouldCondemnChannel */ true );

	pHandler_ = pHandler;

	LoginHandlerPtr pLoginHandler = new LoginHandler( this,
		LogOnStatus::NOT_SET );

	pLoginHandler->startWithBaseAddr( baseAddr, transport_, sessionKey );

	shouldResetEntitiesOnChannel_ = shouldResetEntities;

	this->onSwitchBaseApp( *pLoginHandler );
}


/**
 *	This method can be overridden by subclasses to handle the LoginHandler when
 *	we are switching BaseApps. The default is to pass it to the registered
 *	server message handler.
 *
 *	@param loginHandler 	The login handler, already initialised to connect
 *							to the new BaseApp.
 */
void ServerConnection::onSwitchBaseApp( LoginHandler & loginHandler )
{
	pHandler_->onSwitchBaseApp( loginHandler );
}


/**
 *  The header for a resource download from the server.
 */
void ServerConnection::resourceHeader( BinaryIStream & stream )
{
	// First destream the ID and make sure it isn't already in use
	uint16 id;
	stream >> id;

	DataDownload *pDD;
	DataDownloadMap::iterator it = dataDownloads_.find( id );

	// Usually there shouldn't be an existing download for this id, so make a
	// new one and map it in
	if (it == dataDownloads_.end())
	{
		pDD = new DataDownload( id );
		dataDownloads_[ id ] = pDD;
	}
	else
	{
		// If a download with this ID already exists, then we have data from
		// an old stream which was never completed, but the ID has been
		// reused in the meantime.
		ERROR_MSG( "ServerConnection::resourceHeader: "
			"Collision between new and existing download IDs (%hu), "
			"download will be corrupted\n", id );
		return;
	}

	// Destream the description
	pDD->setDesc( stream );
}


/**
 *	The server is giving us a fragment of a resource that we have requested.
 */
void ServerConnection::resourceFragment( BinaryIStream & stream )
{
	ClientInterface::ResourceFragmentArgs args;
	stream >> args;

	// Get existing DataDownload record if there is one
	DataDownload *pData;
	DataDownloadMap::iterator it = dataDownloads_.find( args.rid );

	if (it != dataDownloads_.end())
	{
		pData = it->second;
	}
	else
	{
		// If we get data before the header, then we're probably getting
		// data for a download we considered to have been aborted.
		// We'll accept the data, and resourceHeader() will raise an ERROR
		// about it, so we won't complain about it here, since if there's a
		// lot of data, we'd spam the error logs needlessly.
		// This means if we -never- receive the header, we'll never report
		// about this corrupted data. But nothing will ever see it
		// either, so it'll just hold some memory until dataDownloads_ is
		// cleaned out.
		pData = new DataDownload( args.rid );
		dataDownloads_[ args.rid ] = pData;
	}

	int length = stream.remainingLength();

	DownloadSegment *pSegment = new DownloadSegment(
		(char*)stream.retrieve( length ), length, args.seq );

	pData->insert( pSegment, args.flags == 1 );

	// If this DataDownload is now complete, invoke script callback and destroy
	if (pData->complete())
	{
		// Take ownership of pData
		dataDownloads_.erase( pData->id() );

		if (pHandler_ != NULL)
		{
			MemoryOStream stream;
			pData->write( stream );

			pHandler_->onStreamComplete( pData->id(), *pData->pDesc(), stream );
		}

		bw_safe_delete( pData );
	}
}


/**
 *	This method handles a message from the server telling us that we have been
 *	disconnected.
 */
void ServerConnection::loggedOff( const ClientInterface::loggedOffArgs & args )
{
	INFO_MSG( "ServerConnection::loggedOff(%d): "
			"The server has disconnected us. reason = %d\n",
		id_, args.reason );

	this->disconnect( /* informServer */ false,
		/* shouldCondemnChannel */ true );
}


/**
 *	Adds the given network interface to the condemned list. This list is
 *	cleared at the end of each call to processInput() when the network
 *	processing machinery is not actually in use.
 *
 *	@param pNetworkInterface		The network interface to be destroyed.
 */
void ServerConnection::addCondemnedInterface(
		Mercury::NetworkInterface * pNetworkInterface )
{
	condemnedInterfaces_.add( pNetworkInterface );
}

BW_END_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Mercury
// -----------------------------------------------------------------------------

#define DEFINE_INTERFACE_HERE
#include "login_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "baseapp_ext_interface.hpp"

#define DEFINE_SERVER_HERE
#include "client_interface.hpp"

// server_connection.cpp
