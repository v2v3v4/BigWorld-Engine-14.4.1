#include "pch.hpp"

#include "bw_server_connection.hpp"

#include "connection/movement_filter.hpp"
#include "connection/server_finder.hpp"
#include "connection/smart_server_connection.hpp"

#include <sys/stat.h>


BW_BEGIN_NAMESPACE

int BWServerConnection_Token = 0;

/**
 *	Constructor.
 *
 *	@param entityFactory 	The entity factory to use when requested to create
 *							entities.
 *	@param spaceDataStorage	The BWSpaceDataStorage for storing space data.
 *	@param entityDefConstants	The constant entity definition values to use
 *								during initialisation.
 *	@param initialClientTime	The client time of BWServerConnection creation
 */
BWServerConnection::BWServerConnection( BWEntityFactory & entityFactory,
			BWSpaceDataStorage & spaceDataStorage,
			LoginChallengeFactories & challengeFactories,
			CondemnedInterfaces & condemnedInterfaces,
			const EntityDefConstants & entityDefConstants,
			double initialClientTime ) :
	BWConnection( entityFactory, spaceDataStorage, entityDefConstants ),
	pServerConnection_(
		new SmartServerConnection( challengeFactories, condemnedInterfaces,
			entityDefConstants ) ),
	pServerMessageHandler_( NULL )
{
	if (initialClientTime > 0.0)
	{
		// TODO: This should roughly be a no-op... Should I give
		// SmartServerConnection an interface to set the initial client time?
		pServerConnection_->update( (float)initialClientTime, NULL );
	}
}


/**
 *	Destructor.
 */
BWServerConnection::~BWServerConnection()
{
}


/**
 *	This method associates a connection handler with this object. It receives
 *	calls on specific significant events.
 *
 *	@param pHandler 	The handler to register.
 */
void BWServerConnection::setHandler( ServerConnectionHandler * pHandler )
{
	pServerConnection_->setConnectionHandler( pHandler );
}


/**
 *	This method sets the LoginApp public key to use for connection session
 *	handshake from a key string.
 *
 *	@param publicKeyString 	The string contents of the public key.
 */
bool BWServerConnection::setLoginAppPublicKeyString(
		const BW::string & publicKeyString )
{
	return pServerConnection_->setLoginAppPublicKeyString( publicKeyString );
}


/**
 *	This method sets the LoginApp public key to use for connection session
 *	handshake from a file path.
 *
 *	@param publicKeyPath 	The path to a file containing the public key.
 */
bool BWServerConnection::setLoginAppPublicKeyPath(
	const BW::string & publicKeyPath )
{
	return pServerConnection_->setLoginAppPublicKeyPath( publicKeyPath );
}


/**
 *	This method sets the client-side artificial loss parameters for any
 *	channels that are used by this connection instance.
 *
 *	@param lossRatio 	The ratio of artificially lost sent packets.
 *	@param minLatency 	The minimum amount of artificial latency to add to sent
 *						packets (in seconds).
 *	@param maxLatency 	The maximum amount of artificial latency to add to sent
 *						packets (in seconds).
 */
void BWServerConnection::setArtificialLoss( float lossRatio, float minLatency,
		float maxLatency )
{
	pServerConnection_->setArtificialLoss( lossRatio, minLatency, maxLatency );
}


/**
 *	This method sets the client-side artificial loss parameters for any
 *	channels that are used by this connection instance.
 *
 *	@param parameters 	The loss parameters.
 */
void BWServerConnection::setArtificialLoss(
		const Mercury::PacketLossParameters & parameters )
{
	pServerConnection_->setArtificialLoss( parameters );
}


/**
 *	This method starts a local network server discovery.
 *
 *	@param serverFinder 	The server finder to use for notifications of
 *							locally running servers.
 *	@param timeout			The timeout period to use.
 */
void BWServerConnection::findServers( ServerFinder & serverFinder, float timeout )
{
	serverFinder.findServers( &(pServerConnection_->networkInterface()),
							 timeout );
}


/**
 *	This method is used to log onto a server.
 *
 *	@param serverAddressString	The server address string, in the form
 *									&lt;hostname&gt;[:port]
 *								If not specified, port is assumed to be 20013.
 *	@param username				The username.
 *	@param password				The password.
 *
 *	@return 	true if the login attempt has started, false if failed.
 */
bool BWServerConnection::logOnTo( const BW::string & serverAddressString,
	const BW::string & username,
	const BW::string & password,
	BW::ConnectionTransport transport )
{
	this->clearAllEntities();

	return pServerConnection_->logOnTo( serverAddressString,
			username, password, transport );
}


/**
 *	This method disconnects the connection.
 */
void BWServerConnection::logOff()
{
	pServerConnection_->logOff();
}


/**
 *	This method returns whether or not the connection is in the process of
 *	logging in.
 */
bool BWServerConnection::isLoggingIn() const
{
	return pServerConnection_->isLoggingIn();
}


/**
 *	This method returns whether or not the connection is active.
 */
bool BWServerConnection::isOnline() const
{
	return pServerConnection_->isOnline() || pServerConnection_->isSwitchingBaseApps();
}


/**
 *	This method returns the server address of the connection.
 */
const Mercury::Address & BWServerConnection::serverAddress() const
{
	return pServerConnection_->addr();
}


/**
 *	This method return the ServerConnection underneath this BWServerConnection
 */
ServerConnection * BWServerConnection::pServerConnection() const
{
	return pServerConnection_.get();
}


/**
 *	This method returns the dispatcher.
 *
 *	This can be used to add timers or register listeners for file-descriptor
 *	events from application code.
 */
Mercury::EventDispatcher & BWServerConnection::dispatcher()
{
	return pServerConnection_->dispatcher();
}


/**
 *	This method disconnects by logging off.
 */
void BWServerConnection::disconnect() /* override */
{
	this->logOff();
}


/**
 *	This method returns the space ID, or NULL_SPACE_ID if not connected to a
 *	space yet.
 */
SpaceID BWServerConnection::spaceID() const /* override */
{
	return (this->pPlayer() != NULL) ? 
		this->pPlayer()->spaceID() :
		NULL_SPACE_ID;
}


/**
 *	This method clears out all the entities if we are disconnected. If they are
 *	not cleared out after disconnection, they will be on the next reconnection.
 */
void BWServerConnection::clearAllEntities(
		bool keepLocalEntities /* = false */ ) /* override */
{
	if (pServerConnection_->isOffline())
	{
		this->BWConnection::clearAllEntities( keepLocalEntities );
	}
}


/**
 *	This returns the client-side elapsed time.
 */
double BWServerConnection::clientTime() const /* override */
{
	return pServerConnection_->clientTime();
}


/**
 *	This method returns the server time.
 */
double BWServerConnection::serverTime() const /* override */
{
	return pServerConnection_->serverTime();
}


/**
 *	This method returns the last time a message was received from the server.
 */
double BWServerConnection::lastMessageTime() const /* override */
{
	return pServerConnection_->lastMessageTime();
}


/**
 *	This method returns the last time a message was sent to the server.
 */
double BWServerConnection::lastSentMessageTime() const /* override */
{
	return pServerConnection_->lastSendTime();
}


/**
 *	This method ticks the connection before the entities are ticked to process
 *	any new messages received.
 *
 *	@param dt 	The time delta.
 */
void BWServerConnection::preUpdate( float dt ) /* override */
{
	pServerConnection_->update( dt, &this->serverMessageHandler() );
}


/**
 *	This method adds a local entity movement to be sent in the next update.
 */
void BWServerConnection::enqueueLocalMove( EntityID entityID, SpaceID spaceID,
	EntityID vehicleID, const Position3D & localPosition,
	const Direction3D & localDirection, bool isOnGround,
	const Position3D & worldReferencePosition ) /* override */
{
	pServerConnection_->addMove( entityID, spaceID, vehicleID, localPosition,
		localDirection.yaw, localDirection.pitch, localDirection.roll,
		isOnGround, worldReferencePosition );
}


/**
 *	This method gets a BinaryOStream for an entity message to be sent to
 *	the server in the next update.
 */
BinaryOStream * BWServerConnection::startServerMessage( EntityID entityID,
	int methodID, bool isForBaseEntity, bool & shouldDrop ) /* override */
{
	if (!pServerConnection_->isOnline())
	{
		shouldDrop = true;
		return NULL;
	}

	return pServerConnection_->startServerEntityMessage( methodID, entityID,
		isForBaseEntity );
}


/**
 *	This method sends any updated state to the server after entities have
 *	updated their state.
 */
void BWServerConnection::postUpdate() /* override */
{
	pServerConnection_->updateServer();
}


/**
 *	This method checks that we're not trying to send too quickly for the
 *	ServerConnection's current settings.
 */
bool BWServerConnection::shouldSendToServerNow() const /* override */
{
	const double timeSinceLastSend =
		pServerConnection_->clientTime() - pServerConnection_->lastSendTime();

	return timeSinceLastSend > pServerConnection_->minSendInterval();
}


/*
 *	Override from BWConnection. 
 */
ServerMessageHandler & BWServerConnection::serverMessageHandler() const /* override */
{
	if (!pServerMessageHandler_)
	{
		return this->BWConnection::serverMessageHandler();
	}

	return *pServerMessageHandler_;
}


/**
 *	This method sets the server message handler for use with this connection.
 */
void BWServerConnection::serverMessageHandler( ServerMessageHandler * pHandler )
{
	pServerMessageHandler_ = pHandler;
	pServerConnection_->setMessageHandler( &(this->serverMessageHandler()) );
}


/**
 *	This method sets the task manager instance to use for background tasks such
 *	as performing login challenges.
 */
void BWServerConnection::pTaskManager( TaskManager * pTaskManager )
{
	pServerConnection_->pTaskManager( pTaskManager );
}


BW_END_NAMESPACE

// bw_server_connection.cpp

