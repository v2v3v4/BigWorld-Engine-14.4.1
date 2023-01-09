#ifndef BW_SERVER_CONNECTION_HPP
#define BW_SERVER_CONNECTION_HPP

#include "bw_connection.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_safe_allocatable.hpp"
#include "connection/connection_transport.hpp"

BW_BEGIN_NAMESPACE

class BinaryOStream;
class BWEntityFactory;
class EntityDefConstants;
class LoginChallengeFactories;
class ServerFinder;
class ServerConnectionHandler;
class SmartServerConnection;


/**
 *	This class is a concrete BWConnection implementation used for talking to
 *	a normal BigWorld server cluster.
 */
class BWServerConnection : public BWConnection, public SafeAllocatable
{
public:
	BWENTITY_API BWServerConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		LoginChallengeFactories & challengeFactories,
		CondemnedInterfaces & condemnedInterfaces,
		const EntityDefConstants & entityDefConstants,
		double initialClientTime );
	BWENTITY_API ~BWServerConnection();

	BWENTITY_API void setHandler( ServerConnectionHandler * pHandler );

	BWENTITY_API
	bool setLoginAppPublicKeyString( const BW::string & publicKeyString );

	BWENTITY_API
	bool setLoginAppPublicKeyPath( const BW::string & publicKeyPath );

	BWENTITY_API void setArtificialLoss( float lossRatio, float minLatency,
		float maxLatency );
	BWENTITY_API
	void setArtificialLoss( const Mercury::PacketLossParameters & parameters );

	BWENTITY_API
	void findServers( ServerFinder & serverFinder, float timeout = 2.f );

	BWENTITY_API bool logOnTo( const BW::string & serverAddressString,
		const BW::string & username,
		const BW::string & password,
		BW::ConnectionTransport transport = CONNECTION_TRANSPORT_UDP );

	BWENTITY_API void logOff();

	BWENTITY_API bool isLoggingIn() const;
	BWENTITY_API bool isOnline() const;

	BWENTITY_API const Mercury::Address & serverAddress() const;
	BWENTITY_API ServerConnection * pServerConnection() const;
	BWENTITY_API Mercury::EventDispatcher & dispatcher();
	BWENTITY_API void pTaskManager( TaskManager * pTaskManager );

	ServerMessageHandler & serverMessageHandler() const /* override */;
	void serverMessageHandler( ServerMessageHandler * pHandler );

	// BWConnection overrides
	BWENTITY_API void disconnect() /* override */;

	BWENTITY_API SpaceID spaceID() const /* override */;

	BWENTITY_API void clearAllEntities( bool keepLocalEntities = false )  /* override */;

	BWENTITY_API double clientTime() const /* override */;
	BWENTITY_API double serverTime() const /* override */;
	BWENTITY_API double lastMessageTime() const /* override */;
	BWENTITY_API double lastSentMessageTime() const /* override */;

private:
	void preUpdate( float dt ) /* override */;
	void enqueueLocalMove( EntityID entityID, SpaceID spaceID, EntityID vehicleID,
		const Position3D & localPosition, const Direction3D & localDirection,
		bool isOnGround,
		const Position3D & worldReferencePosition ) /* override */;
	BinaryOStream * startServerMessage( EntityID entityID, int methodID,
		bool isForBaseEntity, bool & shouldDrop ) /* override */;
	void postUpdate() /* override */;
	bool shouldSendToServerNow() const /* override */;

	std::auto_ptr< SmartServerConnection > 	pServerConnection_;

	ServerMessageHandler * pServerMessageHandler_;
};

BW_END_NAMESPACE

#endif // BW_SERVER_CONNECTION_HPP

