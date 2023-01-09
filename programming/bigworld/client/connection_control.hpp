#ifndef CONNECTION_CONTROL_HPP
#define CONNECTION_CONTROL_HPP

#include "entity_factory.hpp"
#include "space_data_storage.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "connection/condemned_interfaces.hpp"
#include "connection/log_on_status.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/server_connection_handler.hpp"
#include "connection_model/bw_connection_helper.hpp"

#include "network/basictypes.hpp"
#include "network/event_dispatcher.hpp"

#if ENABLE_WATCHERS
#include "network/endpoint.hpp"
#include "network/logger_message_forwarder.hpp"
#endif

#include <memory>


BW_BEGIN_NAMESPACE

class BWConnection;
class BWNullConnection;
class BWReplayConnection;
class BWReplayEventHandler;
class BWServerConnection;
class ClientSpace;
typedef SmartPointer< ClientSpace > ClientSpacePtr;
class Entity;
class LoginHandler;
typedef SmartPointer< LoginHandler > LoginHandlerPtr;
class StreamEncoder;

namespace URL
{
	class Manager;
}


/**
 *	This class is used to visit entities in a ConnectionControl instance.
 */
class EntityVisitor
{
public:
	virtual bool onVisitEntity( Entity * pEntity ) = 0;
};


/**
 *	This class controls the connection to the BigWorld server.
 */
class ConnectionControl : public ServerConnectionHandler,
	public Singleton<ConnectionControl>
{
public:
	ConnectionControl();
	~ConnectionControl();

	void fini();

	void connect( const BW::string & server,
		ScriptObject loginParams, ScriptObject progressFn );
	void disconnect( bool shouldCallProgressFn = true );

	SpaceID startReplayFromFile( const BW::string & fileName,
		BWReplayEventHandler * pHandler, const BW::string & publicKey,
		int volatileInjectionPeriod = -1 );
	SpaceID startReplayFromStream( const BW::string & localCacheFileName,
		bool shouldKeepCacheFile, BWReplayEventHandler * pHandler,
		const BW::string & publicKey, int volatileInjectionPeriod = -1 );
	void stopReplay();

	void notifyOfShutdown();
	bool isShuttingDown() const;

	const BW::string & server() const;
	void probe( const BW::string & server, ScriptObject progressFn );

	void tick( float dt );

	bool isLoginInProgress() const;

	void onBasePlayerCreated();

	static Mercury::EventDispatcher & dispatcher();

	void	clearSpaces();

	SpaceID createLocalSpace();
	void	clearLocalSpace( SpaceID spaceID );

	void visitEntities( EntityVisitor & visitor ) const;
	size_t numEntities() const;

	BWConnection *			pConnection() const;
	BWServerConnection *	pServerConnection() const;
	BWNullConnection *		pNullConnection() const;
	BWReplayConnection *	pReplayConnection() const;

	BWConnection *			pSpaceConnection( SpaceID spaceID ) const;

	double gameTime() const;
	Entity * pPlayer() const;
	Entity * findEntity( EntityID entityID ) const;

	enum LogOnStage
	{
		STAGE_INITIAL 		= 0,
		STAGE_LOGIN 		= 1,
		STAGE_DATA 			= 2,
		STAGE_DISCONNECTED 	= 6
	};

	enum Mode
	{
		NONE,
		ONLINE,		// BWServerConnection
		OFFLINE,	// BWNullConnection
		REPLAY		// BWReplayConnection
	};

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	ConnectionControl( const ConnectionControl& );
	ConnectionControl& operator=( const ConnectionControl& );

	// ServerConnectionHandler implementation
	void onLoggedOff();
	void onLoggedOn();
	void onLogOnFailure( const LogOnStatus & status,
		const BW::string & message );

	// ConnectionControl private methods
	bool initLogOnParamsEncoder( BW::string publicKeyPath );

	void disconnected( bool isShuttingDown, bool shouldCallProgressFn = true );

	void callConnectionCallback( LogOnStage stage, 
		bool shouldCallNextFrame = false, 
		LogOnStatus status = LogOnStatus::NOT_SET,
		const BW::string & message = "" );

	void destroyMainConnection();

	SpaceID nextLocalSpaceID();

	Mercury::EventDispatcher	dispatcher_;
	EntityFactory				entityFactory_;
	SpaceDataStorage			spaceDataStorage_;
	Mercury::PacketLossParameters* pPacketLossParameters_;
	BWConnection *				pConnection_;
	BWConnectionHelper 			connectionHelper_;

	typedef BW::vector< BWConnection * > Connections;
	Connections 				condemnedConnections_;

	typedef BW::map< SpaceID, BWNullConnection * > ClientSpaceConnections;
	ClientSpaceConnections		clientSpaceConnections_;

	typedef BW::map< SpaceID, ClientSpacePtr > ClientSpaces;
	ClientSpaces 				clientSpaces_;

	bool					isExpectingPlayerCreation_;

	ScriptObject			progressFn_;
	BW::string				server_;
	Mode					mode_;
	bool					isInTick_;

	double					offlineTime_;

#if ENABLE_WATCHERS
	Endpoint 				loggerMessageEndpoint_;
	LoggerMessageForwarder * pLoggerMessageForwarder_;

	WatcherProvider *		pCommsWatcherProvider_;
	WatcherProvider *		pEntitiesWatcherProvider_;
	WatcherProvider *		pNetworkInterfaceWatcherProvider_;
#endif

	bool shuttingDown_;

	// progressFn codes:
	//  0: initial checks
	//  1: login (will get disconnect if successful)
	//	2: data
	//	6: disconnected (last call)
	// progressFn params:
	//  <=0: failure
	//  1: success
};

BW_END_NAMESPACE

#endif // CONNECTION_CONTROL_HPP
