#ifndef SERVER_CONNECTION_HPP
#define SERVER_CONNECTION_HPP

#include "client_interface.hpp"
#include "connection_transport.hpp"
#include "login_interface.hpp"
#include "log_on_status.hpp"

#include "cstdmf/md5.hpp"
#include "cstdmf/stdmf.hpp"

#include "math/stat_with_rates_of_change.hpp"
#include "math/vector3.hpp"

#include "network/block_cipher.hpp"
#include "network/channel_listener.hpp"
#include "network/event_dispatcher.hpp"
#include "network/msgtypes.hpp"
#include "network/packet_loss_parameters.hpp"
#include "network/public_key_cipher.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class CondemnedInterfaces;
class DataDownload;
class EntityDefConstants;
class LoginChallengeFactories;
class ServerMessageHandler;
class StreamEncoder;
class TaskManager;

namespace Mercury
{
	class Channel;
	class NetworkInterface;
}

class LoginHandler;
typedef SmartPointer< LoginHandler > LoginHandlerPtr;

/**
 *	This class is used to represent a connection to the server.
 *
 *	@ingroup network
 */
#if defined(__INTEL_COMPILER)
// force safe inheritance member pointer declaration for Intel compiler
#pragma pointers_to_members( pointer-declaration, full_generality )
#endif

class ServerConnection :
		public Mercury::BundlePrimer,
		public Mercury::ChannelListener,
		public TimerHandler
{
public:
	static const float NO_POSITION;
	static const float NO_DIRECTION;

	ServerConnection(
		LoginChallengeFactories & challengeFactories,
		CondemnedInterfaces & condemnedInterfaces,
		const EntityDefConstants & entityDefConstants );

	virtual ~ServerConnection();

	// If possible, do not use. Here for legacy client support.
	void setConstants( const EntityDefConstants & constants );

	bool processInput();
	void registerInterfaces( Mercury::NetworkInterface & networkInterface );

	void setInactivityTimeout( float seconds );

	// Deprecated. Do not use.
	LogOnStatus logOn( ServerMessageHandler * pHandler,
		const char * serverName,
		const char * username,
		const char * password,
		uint16 port = 0 );

	LoginHandlerPtr logOnBegin(
		const char * serverName,
		const char * username,
		const char * password,
		uint16 port = 0,
		ConnectionTransport = CONNECTION_TRANSPORT_UDP );

	LogOnStatus logOnComplete(
		LoginHandlerPtr pLRH,
		ServerMessageHandler * pHandler );

	void enableReconfigurePorts() { tryToReconfigurePorts_ = true; }
	void enableEntities();

	bool isOnline() const;
	bool isOffline() const				{ return !this->isOnline(); }

	// Deprecated.
	bool online() const					{ return this->isOnline(); }
	bool offline() const				{ return !this->isOnline(); }

	void disconnect( bool informServer = true,
		bool shouldCondemnChannel = true );

	void channel( Mercury::Channel & channel );
	void pInterface( Mercury::NetworkInterface * pInterface );
	bool hasInterface() const 
		{ return pInterface_ != NULL; }

	void setArtificialLoss( float lossRatio, float minLatency, 
						   float maxLatency );

	void setArtificialLoss( const Mercury::PacketLossParameters & parameters );

	// Stuff that would normally be provided by ChannelOwner, however we can't
	// derive from it because of destruction order.
	Mercury::Channel & channel();
	Mercury::Bundle & bundle() { return this->channel().bundle(); }
	const Mercury::Address & addr() const;

	/** This method returns the block cipher used for symmetric encryption. */
	Mercury::BlockCipherPtr pBlockCipher() const { return pBlockCipher_; }

	void addMove( EntityID id, SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, float yaw, float pitch, float roll,
		bool onGround, const Position3D & globalPos );
	//void rcvMoves( EntityID id );

	BinaryOStream & startBasePlayerMessage( int methodID );
	BinaryOStream & startCellPlayerMessage( int methodID );

	BinaryOStream & startCellEntityMessage( int methodID, EntityID entityID );

	BinaryOStream * startServerEntityMessage( int methodID, EntityID entityID,
			bool isForBaseEntity );

	const BW::string & serverMsg() const	{ return serverMsg_; }
	const BW::string & errorMsg() const { return this->serverMsg(); }

	EntityID connectedID() const			{ return id_; }
	void sessionKey( SessionKey key ) { sessionKey_ = key; }


	/**
	 *	Return the logon params encoding method.
	 *
	 *	@return The method used to encode the LogOnParams. NULL indicates that
	 *	no encoding be used.
	 */
	StreamEncoder * pLogOnParamsEncoder() const 
		{ return pLogOnParamsEncoder_; }


	/**
	 *	Set the logon params encoding method.
	 *
	 *	@param pEncoder 	The new encoder. This class takes no
	 *						responsibility for deleting pEncoder.
	 */
	void pLogOnParamsEncoder( StreamEncoder * pEncoder )
		{ pLogOnParamsEncoder_ = pEncoder; }

	/**
	 *	This method returns the task manager for use in login challenges.
	 */
	TaskManager * pTaskManager() { return pTaskManager_; }

	/**
	 *	This method sets the task manager for e.g. use in login challenges.
	 */
	void pTaskManager( TaskManager * pTaskManager )
	{ pTaskManager_ = pTaskManager; }

	// ---- Statistics ----

	float latency() const;

	double bpsIn() const;
	double bpsOut() const;

	double packetsPerSecondIn() const;
	double packetsPerSecondOut() const;

	double messagesPerSecondIn() const;
	double messagesPerSecondOut() const;

	int		bandwidthFromServer() const;
	void	bandwidthFromServer( int bandwidth );

	double movementBytesPercent() const;
	double nonMovementBytesPercent() const;
	double overheadBytesPercent() const;

	int movementBytesTotal() const;
	int nonMovementBytesTotal() const;
	int overheadBytesTotal() const;

	uint32 packetsIn() const;

	void pTime( const double * pTime );

	/**
	 *	This method is used to return the pointer to current time.
	 *	It is used for server statistics and for syncronising between
	 *	client and server time.
	 */
	const double * pTime()					{ return pTime_; }

	double		serverTime( double gameTime ) const;
	double 		clientTimeOfLastMessage() const;
	GameTime	gameTimeOfLastMessage() const;

	// These two methods have been deprecated due to naming change
	double 		lastMessageTime() const { return clientTimeOfLastMessage(); }
	GameTime	lastGameTime() const { return gameTimeOfLastMessage(); }

	double		lastSendTime() const	{ return lastSendTime_; }
	double		minSendInterval() const	{ return minSendInterval_; }

	// ---- InterfaceMinder handlers ----
	void authenticate(
		const ClientInterface::authenticateArgs & args );
	void bandwidthNotification(
		const ClientInterface::bandwidthNotificationArgs & args );
	void updateFrequencyNotification(
		const ClientInterface::updateFrequencyNotificationArgs & args );

	void setGameTime( const ClientInterface::setGameTimeArgs & args );

	void resetEntities( const ClientInterface::resetEntitiesArgs & args );
	void resetEntities( bool keepPlayerOnBase = false );
	void createBasePlayer( BinaryIStream & stream );
	void createCellPlayer( BinaryIStream & stream );

	void spaceData( BinaryIStream & stream );

	void enterAoI( const ClientInterface::enterAoIArgs & args );
	void enterAoIOnVehicle(
		const ClientInterface::enterAoIOnVehicleArgs & args );
	void leaveAoI( BinaryIStream & stream );

	void createEntity( BinaryIStream & stream );
	void createEntityDetailed( BinaryIStream & stream );
	void updateEntity( BinaryIStream & stream );

	// This unattractive bit of macros is used to declare all of the handlers
	// for (fixed length) messages sent from the cell to the client. It includes
	// methods such as all of the avatarUpdate handlers.
#define MF_EMPTY_COMMON_RELIABLE_MSG( MESSAGE )	\
	void MESSAGE();

#define MF_BEGIN_COMMON_RELIABLE_MSG( MESSAGE )	\
	void MESSAGE( const ClientInterface::MESSAGE##Args & args );

#define MF_BEGIN_COMMON_PASSENGER_MSG MF_BEGIN_COMMON_RELIABLE_MSG
#define MF_BEGIN_COMMON_UNRELIABLE_MSG MF_BEGIN_COMMON_RELIABLE_MSG

#define MF_COMMON_ARGS( ARGS )
#define MF_END_COMMON_MSG()
#define MF_COMMON_ISTREAM( NAME, XSTREAM )
#define MF_COMMON_OSTREAM( NAME, XSTREAM )
#include "common_client_interface.hpp"
#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_PASSENGER_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG
#undef MF_COMMON_ARGS
#undef MF_END_COMMON_MSG
#undef MF_COMMON_ISTREAM
#undef MF_COMMON_OSTREAM

	void detailedPosition( const ClientInterface::detailedPositionArgs & args );
	void controlEntity( const ClientInterface::controlEntityArgs & args );

	void voiceData( const Mercury::Address & srcAddr,
					const Mercury::UnpackedMessageHeader & header,
					BinaryIStream & stream );

	void restoreClient( BinaryIStream & stream );

	void switchBaseApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const ClientInterface::switchBaseAppArgs & args );

	void resourceHeader( BinaryIStream & stream );
	void resourceFragment( BinaryIStream & stream );

	void loggedOff( const ClientInterface::loggedOffArgs & args );

	void entityMethod( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

	void entityProperty( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

	void nestedEntityProperty( BinaryIStream & stream );

	void sliceEntityProperty( BinaryIStream & stream );

	int entityMethodGetStreamSize( Mercury::MessageID msgID ) const;
	int entityPropertyGetStreamSize( Mercury::MessageID msgID ) const;

	void setMessageHandler( ServerMessageHandler * pHandler )
	{
		if (pHandler_ != NULL)
		{
			pHandler_ = pHandler;
		}
	}

	ServerMessageHandler * getMessageHandler() const
	{
		return pHandler_;
	}

	Mercury::NetworkInterface & networkInterface();

	Mercury::EventDispatcher & dispatcher() { return dispatcher_; };

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

	const MD5::Digest digest() const			{ return digest_; }

	double sendTimeReportThreshold() const 
	{ return sendTimeReportThreshold_; } 

	void sendTimeReportThreshold( double threshold ) 
	{ sendTimeReportThreshold_ = threshold; } 

	void sendIfNecessary();
	void send();

	LoginChallengeFactories & challengeFactories()
	{
		return challengeFactories_;
	}

	void addCondemnedInterface( Mercury::NetworkInterface * pInterface );

	/**
	 *	The frequency of updates from the server.
	 */
	float updateFrequency() const { return updateFrequency_; }

	static const char * transportToCString( ConnectionTransport transport );


	/**
	 * Sets whether messages should are processed out of order (early)
	 * (for use by ClientMessageHandler only)
	 * @newValue value to be set
	 *
	 */
	void processingEarlyMessageNow( bool newValue )
	{
		isEarly_ = newValue;
	}

	/**
	 * Returns whether messages are processed out of order (early)
	 */
	bool processingEarlyMessageNow() const
	{
		return isEarly_;
	}
protected:
	virtual void onSwitchBaseApp( LoginHandler & loginHandler );

private:
	typedef StatWithRatesOfChange< uint32 > Stat;

	// Not defined.
	ServerConnection( const ServerConnection & );
	ServerConnection& operator=( const ServerConnection & );

	void requestEntityUpdate( EntityID id,
		const CacheStamps & stamps = CacheStamps() );

	void enterAoI( EntityID id, IDAlias idAlias,
		EntityID vehicleID = NULL_ENTITY_ID );

	double appTime() const;
	void updateStats();

	// Override from Mercury::ChannelListener.
	virtual void onChannelSend( Mercury::Channel & channel );
	virtual void onChannelGone( Mercury::Channel & channel );

	void setVehicle( EntityID passengerID, EntityID vehicleID );
	EntityID getVehicleID( EntityID passengerID ) const
	{
		PassengerToVehicleMap::const_iterator iter =
			passengerToVehicle_.find( passengerID );

		return iter == passengerToVehicle_.end() ?
												NULL_ENTITY_ID : iter->second;
	}

	void initialiseConnectionState();

	virtual void primeBundle( Mercury::Bundle & bundle );
	virtual int numUnreliableMessages() const;

	virtual void handleTimeout( TimerHandle handle, void * arg );

	double getRatePercent( const Stat & stat ) const;

	/**
	 *	This method returns whether the entity with the input id is controlled
	 *	locally by this client as opposed to controlled by the server.
	 */
	bool isControlledLocally( EntityID id ) const
	{
		return controlledEntities_.find( id ) != controlledEntities_.end();
	}

	void detailedPositionReceived( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & position );


	// ---- Data members ----
	SessionKey		sessionKey_;
	BW::string	username_;

	// ---- Statistics ----

	Stat numMovementBytes_;
	Stat numNonMovementBytes_;
	Stat numOverheadBytes_;

	ServerMessageHandler * pHandler_;

	EntityID	id_;
	EntityID	selectedEntityID_;
	SpaceID		spaceID_;
	int			bandwidthFromServer_;

	const double * pTime_;
	uint64 	lastReceiveTime_;
	double	lastSendTime_;
	double	minSendInterval_;
	double	sendTimeReportThreshold_;
	float	updateFrequency_;	// frequency of updates from the server.

	Mercury::EventDispatcher	dispatcher_;
	Mercury::NetworkInterface *	pInterface_;
	Mercury::ChannelPtr			pChannel_;
	ConnectionTransport 		transport_;

	Mercury::PacketLossParameters packetLossParameters_;

	bool 		shouldResetEntitiesOnChannel_;
	bool		tryToReconfigurePorts_;
	bool		entitiesEnabled_;
	uint8		isOnGround_;

	float				inactivityTimeout_;
	MD5::Digest			digest_;

	/// This is a simple class to handle what time the client thinks is on the
	/// server.
	class ServerTimeHandler
	{
	public:
		ServerTimeHandler();

		void pServerConnection( ServerConnection * pConnection )
		{
			pConnection_ = pConnection;
		}

		void tickSync( uint8 newSeqNum, double currentTime, bool isEarly );
		void gameTime( GameTime gameTime, double currentTime );

		double		serverTime( double gameTime ) const;
		double 		clientTimeOfLastMessage() const;
		GameTime 	gameTimeOfLastMessage() const;


	private:
		ServerConnection * pConnection_;
		uint8 tickByte_;
		bool hasReceivedGameTime_;
		double timeAtSequenceStart_;
		GameTime gameTimeAtSequenceStart_;
		mutable double lastReturnedServerTime_;
		bool lastPacketWasOrdered_;
		uint8 lastOrderedTickByte_;
	} serverTimeHandler_;

	BW::string serverMsg_;

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	uint8	sendingSequenceNumber_;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	EntityID	idAlias_[ 256 ];

	float packedXZScale_;

	typedef BW::map< EntityID, EntityID > PassengerToVehicleMap;
	PassengerToVehicleMap passengerToVehicle_;

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	Position3D		sentPositions_[ 256 ];
	Position3D		referencePosition_;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	typedef BW::set< EntityID >		ControlledEntities;
	ControlledEntities controlledEntities_;

	typedef BW::map< uint16, DataDownload* > DataDownloadMap;
	DataDownloadMap dataDownloads_;

	Mercury::BlockCipherPtr pBlockCipher_;

	StreamEncoder * pLogOnParamsEncoder_;

	TimerHandle timerHandle_;

	LoginChallengeFactories & challengeFactories_;
	CondemnedInterfaces & condemnedInterfaces_;

	int maxClientMethodCount_;
	int maxBaseMethodCount_;
	int maxCellMethodCount_;

	TaskManager * pTaskManager_;

	const int FIRST_AVATAR_UPDATE_MESSAGE;
	const int LAST_AVATAR_UPDATE_MESSAGE;

	bool isEarly_;
};

#ifdef CODE_INLINE
#include "server_connection.ipp"
#endif

#if defined(__INTEL_COMPILER)
// revert pointer declaration pragma to its default value
#pragma pointers_to_members( pointer-declaration, best_case )
#endif

BW_END_NAMESPACE

#endif // SERVER_CONNECTION_HPP
