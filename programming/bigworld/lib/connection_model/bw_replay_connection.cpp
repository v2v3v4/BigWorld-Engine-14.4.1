#include "pch.hpp"

#include "bw_replay_connection.hpp"

#include "bw_replay_event_handler.hpp"
#include "bw_space_data_storage.hpp"

#include "connection/client_server_protocol_version.hpp"
#include "connection/replay_controller.hpp"

#include "cstdmf/binary_stream.hpp"

#include "network/basictypes.hpp"

#include <sys/stat.h>

BW_BEGIN_NAMESPACE

int BWReplayConnection_Token = 0;

/**
 *	Constructor.
 *
 *	@param entityFactory 	The entity factory to use when requested to create
 *							entities.
 *	@param spaceDataStorage	The BWSpaceDataStorage for storing space data.
 *	@param entityDefConstants	The constant entity definition values to use
 *								during initialisation.
 *	@param initialClientTime	The client time of BWReplayConnection creation
 */
BWReplayConnection::BWReplayConnection( BWEntityFactory & entityFactory,
			BWSpaceDataStorage & spaceDataStorage,
			const EntityDefConstants & entityDefConstants,
			double initialClientTime ) :
	BWConnection( entityFactory, spaceDataStorage, entityDefConstants ),
	entityDefConstants_( entityDefConstants ),
	spaceDataStorage_( spaceDataStorage ),
	pReplayEventHandler_( NULL ),
	pReplayController_( NULL ),
	time_( initialClientTime ),
	lastSendTime_( 0. ),
	spaceID_( NULL_SPACE_ID ),
	verificationKey_(),
	isHandlingInitialData_( false ),
	playerAoIMap_()
{
	this->addEntitiesListener( this );
}


/**
 *	Destructor.
 */
BWReplayConnection::~BWReplayConnection()
{
	this->removeEntitiesListener( this );
}


/**
 *	This method associates a replay event handler with this object. It receives
 *	calls on specific significant events.
 *
 *	@param pHandler 	The handler to register.
 */
void BWReplayConnection::setHandler( BWReplayEventHandler * pHandler )
{
	pReplayEventHandler_ = pHandler;
}


/**
 *	This method configures the public key for the replay data signature.
 *
 *	@param	publicKeyString	The BW::string contents of the public key.
 *	@return	true if the key was set successfully, false otherwise.
 */
bool BWReplayConnection::setReplaySignaturePublicKeyString(
	const BW::string & publicKeyString )
{
	verificationKey_ = publicKeyString;
	return true;
}


/**
 *	This method reads the public key for the replay data signature from a
 *	file.
 *
 *	@param	publicKeyPath	The path to a file containing the public key.
 *	@return	true if the key was set successfully, false otherwise.
 */
bool BWReplayConnection::setReplaySignaturePublicKeyPath(
	const BW::string & publicKeyPath )
{
	struct stat publicKeyStat;
	if (0 != stat( publicKeyPath.c_str(), &publicKeyStat ))
	{
		return false;
	}

	FILE * pPublicKeyFile = fopen( publicKeyPath.c_str(), "r" );
	if (pPublicKeyFile == NULL)
	{
		return false;
	}

	char * buf = new char[publicKeyStat.st_size];

	if (1 != fread( buf, publicKeyStat.st_size, 1, pPublicKeyFile ))
	{
		bw_safe_delete_array( buf );
		fclose( pPublicKeyFile );
		return false;
	}

	fclose( pPublicKeyFile );

	verificationKey_.assign( buf, publicKeyStat.st_size );
	bw_safe_delete_array( buf );

	return true;
}


/**
 *	This method starts a replay playback from an existing local file.
 *
 *	@param	taskManager				The TaskManager for I/O activity.
 *	@param	replayFileName			The local filename for the replay data
 *	@param	volatileInjectionPeriod	How often to generate volatile position
 *									updates.
 *	@return	true if the replay was started, false otherwise
 *
 *	The replay will play back tick 0 (containing all the initial data for the
 *	replay space) and then wait for BWConnection::onInitialDataProcessed to be
 *	called, giving the application time to load relevant assets. If no
 *	handler is set when tick 0 playback is complete, the replay will not pause.
 */
bool BWReplayConnection::startReplayFromFile( TaskManager & taskManager,
	SpaceID spaceID, const BW::string & replayFileName,
	GameTime volatileInjectionPeriod )
{
	if (pReplayController_ != NULL)
	{
		ERROR_MSG( "BWReplayConnection::startReplayFromFile: Attempted to "
				"start a replay while an existing replay is in progress\n" );
		return false;
	}

	this->clearAllEntities();

	spaceID_ = spaceID;
	spaceDataStorage_.addNewSpace( spaceID_ );
	isHandlingInitialData_ = true;

	pReplayController_ = new ReplayController( taskManager, spaceID_,
		entityDefConstants_, verificationKey_, *this, replayFileName,
		ReplayController::FILE_ALREADY_DOWNLOADED, volatileInjectionPeriod );

	return true;
}


/**
 *	This method starts a replay playback from a streaming source. Data must be
 *	provided using the BWReplayConnection::addReplayData method.
 *
 *	@see BWReplayConnection::addReplayData
 *	@see BWReplayConnection::onReplayDataComplete
 *
 *	@param	taskManager				The TaskManager for I/O activity.
 *	@param	localCacheFileName		The path to cache stream data into.
 *	@param	shouldKeepCacheFile		false if the localCacheFileName should be
 *									deleted once the playback is terminated.
 *	@param	volatileInjectionPeriod	How often to generate volatile position
 *									updates.
 *	@return	true if the replay was started, false otherwise
 *
 *	The replay will play back tick 0 (containing all the initial data for the
 *	replay space) and then wait for BWConnection::onInitialDataProcessed to be
 *	called, giving the application time to load relevant assets. If no
 *	handler is set when tick 0 playback is complete, the replay will not pause.
 */
bool BWReplayConnection::startReplayFromStreamData(
	TaskManager & taskManager, SpaceID spaceID,
	const BW::string & localCacheFileName, bool shouldKeepCacheFile,
	GameTime volatileInjectionPeriod )
{
	if (pReplayController_ != NULL)
	{
		ERROR_MSG( "BWReplayConnection::startReplayFromStreamData: Attempted "
			"to start a replay while an existing replay in in progress\n" );
		return false;
	}

	this->clearAllEntities();

	spaceID_ = spaceID;
	spaceDataStorage_.addNewSpace( spaceID_ );
	isHandlingInitialData_ = true;

	pReplayController_ = new ReplayController( taskManager, spaceID_,
		entityDefConstants_, verificationKey_, *this, localCacheFileName,
		shouldKeepCacheFile ?
			ReplayController::FILE_KEEP : ReplayController::FILE_REMOVE,
		volatileInjectionPeriod );

	return true;
}


/**
 *	This method is called after the data read from the initial tick of the
 *	replay has been handled by the application, to allow the replay to proceed.
 */
void BWReplayConnection::onInitialDataProcessed()
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::onInitialDataProcessed: No replay in "
				"progress\n" );
		return;
	}

	isHandlingInitialData_ = false;
}


/**
 *	This method stops the existing replay and discards all the replay state.
 *	Entities and space data are not discarded until clearAllEntities() is
 *	called or a new replay playback is started.
 */
void BWReplayConnection::stopReplay()
{
	this->clearReplayController();

	// If we were stopped due to error, the error should have been handled
	// and the event handler would have been cleared.
	if (pReplayEventHandler_ != NULL)
	{
		this->notifyReplayTerminated( 
			BWReplayEventHandler::REPLAY_STOPPED_PLAYBACK );
	}
}


/**
 *	This method adds data to a streaming replay.
 *
 *	@param	data	The data to add to the replay.
 */
void BWReplayConnection::addReplayData( BinaryIStream & data )
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::addReplayData: Tried to add data to a "
				"replay when no replay is in progress\n" );
		if (pReplayEventHandler_ != NULL)
		{
			this->notifyReplayTerminated( 
				BWReplayEventHandler::REPLAY_STOPPED_PLAYBACK );
		}
		return;
	}

	if (!pReplayController_->isReceivingData())
	{
		ERROR_MSG( "BWReplayConnection::addReplayData: Tried to add data to a "
			"replay that is not accepting replay data\n" );
		return;
	}

	if (!pReplayController_->addReplayData( data ))
	{
		ERROR_MSG( "BWReplayConnection::addReplayData: "
				"Error while adding data: %s\n",
			pReplayController_->errorMessage().c_str() );

		this->clearReplayController();

		this->notifyReplayTerminated(
			BWReplayEventHandler::REPLAY_ABORTED_CORRUPTED_DATA );
	}
}


/**
 *	This method is called to notify us that there will be no more streaming
 *	data added to this replay.
 */
void BWReplayConnection::onReplayDataComplete()
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::onReplayDataComplete: Tried to notify "
			"replay of data completion when no replay is in progress\n" );
		return;
	}

	if (!pReplayController_->isReceivingData())
	{
		ERROR_MSG( "BWReplayConnection::onReplayDataComplete: Tried to notify "
			"replay of data completion that is not accepting replay data\n" );
		return;
	}

	pReplayController_->replayDataFinalise();
}


/**
 *	This method pauses playback of a replay.
 */
void BWReplayConnection::pauseReplay()
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::onReplayDataComplete: Tried to pause "
			"replay when no replay is in progress\n" );
		return;
	}

	pReplayController_->pausePlayback();
}


/**
 *	This method resumes playback of a paused replay.
 */
void BWReplayConnection::resumeReplay()
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::onReplayDataComplete: Tried to unpause "
			"replay when no replay is in progress\n" );
		return;
	}

	pReplayController_->resumePlayback();
}


/**
 *	This method returns the current playback speed scale of the replay.
 */
float BWReplayConnection::speedScale() const
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::speedScale: Tried to get playback "
			"speed of a replay when no replay is in progress\n" );
		return 1.f;
	}

	return pReplayController_->speedScale();
}


/**
 *	This method sets the playback speed scale of the replay.
 */
void BWReplayConnection::speedScale( float newValue )
{
	if (pReplayController_ == NULL)
	{
		ERROR_MSG( "BWReplayConnection::speedScale: Tried to set playback "
			"speed of a replay when no replay is in progress\n" );
		return;
	}

	pReplayController_->speedScale( newValue );
}


/**
 *	This method disconnects by stopping playback.
 */
void BWReplayConnection::disconnect() /* override */
{
	this->stopReplay();
}


/**
 *	This method returns the space ID.
 */
SpaceID BWReplayConnection::spaceID() const /* override */
{
	return spaceID_;
}


/**
 *	This method clears out all the entities if we are not running a replay. If
 *	they are not cleared out after replay is terminated, they will be on the
 *	next replay's start.
 */
void BWReplayConnection::clearAllEntities(
		bool keepLocalEntities /* = false */ ) /* override */
{
	if (pReplayController_ == NULL)
	{
		this->BWConnection::clearAllEntities( keepLocalEntities );
	}

	playerAoIMap_.clear();
}


/**
 *	This returns the client-side elapsed time.
 */
double BWReplayConnection::clientTime() const /* override */
{
	return time_;
}


/**
 *	This method returns the server time.
 */
double BWReplayConnection::serverTime() const /* override */
{
	// TODO: Read the serverTime replay start from the replay header
	// so that this is consistent across playbacks of the same replay.
	return pReplayController_->getCurrentTickTimeInSeconds();
}


/**
 *	This method returns the last time a message was received from the server.
 */
double BWReplayConnection::lastMessageTime() const /* override */
{
	return time_;
}


/**
 *	This method returns the last time a message was sent to the server.
 */
double BWReplayConnection::lastSentMessageTime() const /* override */
{
	return lastSendTime_;
}


/**
 *	This method ticks the connection before the entities are ticked to process
 *	any new messages received.
 *
 *	@param dt 	The time delta.
 */
void BWReplayConnection::preUpdate( float dt ) /* override */
{
	// When isHandlingInitialData is true, we don't tick the replay controller
	// past tick 0, to give the client system time to handle all the space and
	// entity loading it normally requires.
	if (pReplayController_ != NULL &&
		(!isHandlingInitialData_ || pReplayController_->getCurrentTick() == 0))
	{
		pReplayController_->tick( dt );
	}

	time_ += dt;
}


/**
 *	This method adds a local entity movement to be sent in the next update.
 */
void BWReplayConnection::enqueueLocalMove( EntityID entityID, SpaceID spaceID,
	EntityID vehicleID, const Position3D & localPosition,
	const Direction3D & localDirection, bool isOnGround,
	const Position3D & worldReferencePosition ) /* override */
{
}


/**
 *	This method gets a BinaryOStream for an entity message to be sent to
 *	the server in the next update.
 */
BinaryOStream * BWReplayConnection::startServerMessage( EntityID entityID,
	int methodID, bool isForBaseEntity, bool & shouldDrop ) /* override */
{
	shouldDrop = true;
	return NULL;
}


/**
 *	This method sends any updated state to the server after entities have
 *	updated their state.
 */
void BWReplayConnection::postUpdate() /* override */
{
	lastSendTime_ = time_;
}


/**
 *	This method checks that we're not trying to send too quickly for the
 *	ServerConnection's current settings.
 */
bool BWReplayConnection::shouldSendToServerNow() const /* override */
{
	static const double sendInterval = 100.; // 100ms between "sends".
	const double timeSinceLastSend = time_ - lastSendTime_;

	return timeSinceLastSend > sendInterval;
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerGotBadVersion(
	ReplayController & controller,
	const ClientServerProtocolVersion & version ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	TRACE_MSG( "BWReplayConnection::onReplayControllerGotBadVersion: "
			"%s (current: %s)\n",
		version.c_str(),
		ClientServerProtocolVersion::currentVersion().c_str() );

	if (pReplayEventHandler_)
	{
		this->notifyReplayTerminated(
			BWReplayEventHandler::REPLAY_ABORTED_VERSION_MISMATCH );
	}

	pReplayController_ = NULL;
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerGotCorruptedData()
{
	if (pReplayEventHandler_)
	{
		this->notifyReplayTerminated(
			BWReplayEventHandler::REPLAY_ABORTED_CORRUPTED_DATA );
	}

	pReplayController_ = NULL;
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
bool BWReplayConnection::onReplayControllerReadHeader(
	ReplayController & controller, const ReplayHeader & header ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		// This is an event from a previous ReplayController so drop it.
		return false;
	}

	if (header.digest() == entityDefConstants_.digest())
	{
		if (pReplayEventHandler_)
		{
			pReplayEventHandler_->onReplayHeaderRead( header );
		}

		return true;
	}

	TRACE_MSG( "BWReplayConnection::onReplayControllerReadHeader: "
			"bad digest %s (current: %s)\n",
		header.digest().quote().c_str(),
		entityDefConstants_.digest().quote().c_str() );

	if (pReplayEventHandler_)
	{
		this->notifyReplayTerminated(
			BWReplayEventHandler::REPLAY_ABORTED_ENTITYDEF_MISMATCH );
	}

	pReplayController_ = NULL;

	return false;
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
bool BWReplayConnection::onReplayControllerReadMetaData(
	ReplayController & controller,
	const ReplayMetaData & metaData ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return false;
	}

	if (pReplayEventHandler_ == NULL ||
		pReplayEventHandler_->onReplayMetaData( metaData ))
	{
		return true;
	}

	this->notifyReplayTerminated(
		BWReplayEventHandler::REPLAY_ABORTED_METADATA_REJECTED );

	pReplayController_ = NULL;

	return false;
}


/*
 *	Override from BWConnection.
 */
bool BWReplayConnection::isWitness( EntityID entityID ) const /* override */
{
	return (playerAoIMap_.count( entityID ) != 0);
}


/*
 *	Override from BWConnection.
 */
bool BWReplayConnection::isInAoI( EntityID entityID,
		EntityID playerID ) const /* override */
{
	PlayerAoIMap::const_iterator iter = playerAoIMap_.find( playerID );

	if (iter == playerAoIMap_.end())
	{
		return false;
	}

	return (iter->second.count( entityID ) != 0);
}


/*
 *	Override from BWConnection.
 */
size_t BWReplayConnection::numInAoI( EntityID playerID ) const /* override */
{
	PlayerAoIMap::const_iterator iter = playerAoIMap_.find( playerID );

	if (iter == playerAoIMap_.end())
	{
		return 0;
	}

	return iter->second.size();
}


/*
 *	Override from BWConnection.
 */
void BWReplayConnection::visitAoIEntities( EntityID witnessID,
		AoIEntityVisitor & visitor ) /* override */
{
	PlayerAoIMap::const_iterator iPlayer = playerAoIMap_.find( witnessID );

	if (iPlayer == playerAoIMap_.end())
	{
		return;
	}

	AoISet::const_iterator iAoIEntity = iPlayer->second.begin();
	while (iAoIEntity != iPlayer->second.end())
	{
		BWEntityPtr pAoIEntity = this->entities().find( *iAoIEntity );
		
		visitor.onVisitAoIEntity( pAoIEntity.get() );

		++iAoIEntity;
	}
}


/*
 *	Override from BWEntitiesListener.
 */
void BWReplayConnection::onEntityEnter( BWEntity * pEntity ) /* override */
{
	// We call the appropriate AoI callbacks when an entity enters world.

	{
		// See if the entity is a witness, and inform for the client change and
		// for the AoI changes for each entity in its AoI.

		// Hold onto the entity in case it is destroyed in the callbacks.
		BWEntityPtr pEntityOwner = pEntity;

		PlayerAoIMap::const_iterator iPlayer = playerAoIMap_.find( 
			pEntity->entityID() );

		if (iPlayer != playerAoIMap_.end())
		{
			pReplayEventHandler_->onReplayEntityClientChanged( pEntity );
			
			AoISet::iterator iAoIEntity = iPlayer->second.begin();
			while (iAoIEntity != iPlayer->second.end())
			{
				BWEntityPtr pAoIEntity = this->entities().find( *iAoIEntity );
				if (pAoIEntity)
				{
					pReplayEventHandler_->onReplayEntityAoIChanged( pEntity,
						pAoIEntity.get(), /* isEnter */ true );
				}
				++iAoIEntity;
			}
		}
	}

	{
		// Now we need to deal with all the in-world witnesses that have this 
		// entity in their AoI.
		PlayerAoIMap::const_iterator iPlayer = playerAoIMap_.begin();
		while (iPlayer != playerAoIMap_.end())
		{
			if (iPlayer->second.count( pEntity->entityID() ))
			{
				BWEntityPtr pPlayer = this->entities().find( iPlayer->first );
				if (pPlayer)
				{
					pReplayEventHandler_->onReplayEntityAoIChanged(
						pPlayer.get(), pEntity, /* isEnter */ true );
				}
			}
			++iPlayer;
		}
	}
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerEntityPlayerStateChange(
	ReplayController & controller, EntityID playerID,
	bool hasBecomePlayer ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	if (hasBecomePlayer)
	{
		AoISet newAoISet;
		newAoISet.insert( playerID );
		playerAoIMap_.insert( PlayerAoIMap::value_type( playerID,
			newAoISet ) );
	}
	else if (playerAoIMap_.erase( playerID ) == 0)
	{
		ERROR_MSG( "BWReplayConnection::"
				"onReplayControllerEntityPlayerStateChange: "
				"Got player state change to become non-player without first "
				"becoming player for entity %d\n",
			playerID );

		// Server created a malformed replay file, but shouldn't be fatal for 
		// client.
		return;
	}

	BWEntity * pEntity = this->entities().find( playerID );
	if (!pEntity)
	{
		// When the entity enters world, the onReplayEntityClientChanged() is 
		// called.
		return;
	}

	MF_ASSERT( !pEntity->isDestroyed() );

	pReplayEventHandler_->onReplayEntityClientChanged( pEntity );
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerAoIChange(
	ReplayController & controller, EntityID playerID, EntityID entityID,
	bool hasEnteredAoI ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	BWEntity * pWitness = this->entities().find( playerID );
	BWEntity * pEntity = this->entities().find( entityID );

	PlayerAoIMap::iterator iter = playerAoIMap_.find( playerID );
	MF_ASSERT( iter != playerAoIMap_.end() );

	if (hasEnteredAoI)
	{
		iter->second.insert( entityID );
	}
	else if (iter->second.erase( entityID ) == 0)
	{
		ERROR_MSG( "BWReplayConnection::onReplayControllerAoIChange: "
				"Got AoI exit event without first getting AoI entry event for "
				"entity %d in witness %d\n",
			entityID, playerID );

		// Server created a malformed replay file, but shouldn't be fatal for 
		// client.
		return;
	}

	if (!pWitness || !pEntity)
	{
		// When both witness and entity are in-world,
		// onReplayEntityAoIChanged will be called. 
		return;
	}

	MF_ASSERT( !pWitness->isDestroyed() );
	MF_ASSERT( !pEntity->isDestroyed() );

	pReplayEventHandler_->onReplayEntityAoIChanged( pWitness, pEntity,
		hasEnteredAoI );
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerError(
	ReplayController & controller, const BW::string & error ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	if (pReplayEventHandler_)
	{
		pReplayEventHandler_->onReplayError( error );
	}
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerFinish(
	ReplayController & controller ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	if (pReplayEventHandler_)
	{
		pReplayEventHandler_->onReplayReachedEnd();
	}
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerDestroyed(
	ReplayController & controller ) /* override */
{
	// We might have already forgotten this controller if we already
	// called BWReplayEventHandler::onReplayTerminated elsewhere and started
	// a new replay afterwards.
	if (&controller != pReplayController_.get())
	{
		return;
	}

	// We're done with this replay controller now.
	pReplayController_ = NULL;
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerPostTick(
	ReplayController & controller, GameTime tickTime )
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	if (pReplayEventHandler_)
	{
		pReplayEventHandler_->onReplayTicked( tickTime,
			controller.numTicksTotal() );
	}
	else if (tickTime == 0)
	{
		// If there's no event handler, then we should just let the replay
		// continue playing by itself.
		isHandlingInitialData_ = false;
	}
}


/*
 *	This method overrides the one in ReplayControllerHandler
 */
void BWReplayConnection::onReplayControllerIncreaseTotalTime( 
	ReplayController& controller,
	double dTime ) /* override */
{
	if (&controller != pReplayController_.get())
	{
		return;
	}

	if (pReplayEventHandler_)
	{
		pReplayEventHandler_->onReplaySeek( dTime );
	}
}


/*
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onBasePlayerCreate( EntityID id, EntityTypeID type,
	BinaryIStream & data ) /* override */
{
	// This should never happen.
	MF_ASSERT( false );
	/*
	this->serverMessageHandler().onBasePlayerCreate( id, type, data );
	*/
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onCellPlayerCreate( EntityID id, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos, float yaw, float pitch,
	float roll, BinaryIStream & data ) /* override */
{
	// This should never happen.
	MF_ASSERT( false );
	/*
	this->serverMessageHandler().onCellPlayerCreate( id, spaceID, vehicleID,
		pos, yaw, pitch, roll, data );
	*/
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityControl( EntityID id,
	bool control ) /* override */
{
	// We never allow local control of replay entities.
	MF_ASSERT( control );
	return;
	/*
	this->serverMessageHandler().onEntityControl( id, control );
	*/
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
const CacheStamps * BWReplayConnection::onEntityCacheTest(
	EntityID id ) /* override */
{
	return this->serverMessageHandler().onEntityCacheTest( id );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityLeave( EntityID id,
	const CacheStamps & stamps ) /* override */
{
	this->serverMessageHandler().onEntityLeave( id, stamps );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityCreate( EntityID id, EntityTypeID type,
	SpaceID spaceID, EntityID vehicleID, const Position3D & pos, float yaw,
	float pitch, float roll, BinaryIStream & data ) /* override */
{
	this->serverMessageHandler().onEntityCreate( id, type, spaceID, vehicleID,
		pos, yaw, pitch, roll, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityProperties( EntityID id,
	BinaryIStream & data ) /* override */
{
	this->serverMessageHandler().onEntityProperties( id, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityMethod( EntityID id, int methodID,
	BinaryIStream & data ) /* override */
{
	this->serverMessageHandler().onEntityMethod( id, methodID, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityProperty( EntityID id, int propertyID,
	BinaryIStream & data ) /* override */
{
	this->serverMessageHandler().onEntityProperty( id, propertyID, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onNestedEntityProperty( EntityID id,
	BinaryIStream & data, bool isSlice ) /* override */
{
	this->serverMessageHandler().onNestedEntityProperty( id, data, isSlice );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
int BWReplayConnection::getEntityMethodStreamSize( EntityID id,
	int methodID, bool isFailAllowed ) const /* override */
{
	return this->serverMessageHandler().getEntityMethodStreamSize( id,
		methodID, isFailAllowed );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
int BWReplayConnection::getEntityPropertyStreamSize( EntityID id,
	int propertyID, bool isFailAllowed ) const /* override */
{
	return this->serverMessageHandler().getEntityPropertyStreamSize( id,
		propertyID, isFailAllowed );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntityMoveWithError( EntityID id, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos, const Vector3 & posError,
	float yaw, float pitch, float roll, bool isVolatile ) /* override */
{
	this->serverMessageHandler().onEntityMoveWithError( id, spaceID, vehicleID,
		pos, posError, yaw, pitch, roll, isVolatile );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::spaceData( SpaceID spaceID, SpaceEntryID entryID,
	uint16 key, const BW::string & data ) /* override */
{
	this->serverMessageHandler().spaceData( spaceID, entryID, key, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::spaceGone( SpaceID spaceID ) /* override */
{
	this->serverMessageHandler().spaceGone( spaceID );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onStreamComplete( uint16 id, const BW::string & desc,
	BinaryIStream & data ) /* override */
{
	this->serverMessageHandler().onStreamComplete( id, desc, data );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onEntitiesReset( bool keepPlayerOnBase ) /* override */
{
	// We should never be trying to destroy the cell-side of a player.
	// TODO: Stop non-server messages calling onEntitiesReset()?
	MF_ASSERT( !keepPlayerOnBase );
	this->serverMessageHandler().onEntitiesReset( keepPlayerOnBase );
}


/**
 *	This method overrides the one in ServerMessageHandler
 */
void BWReplayConnection::onRestoreClient( EntityID id, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos, const Direction3D & dir,
	BinaryIStream & data ) /* override */
{
	// This should never be called.
	MF_ASSERT( false );
	/*
	this->serverMessageHandler().onRestoreClient( id, spaceID, vehicleID, pos,
		dir, data );
	*/
}


/**
 *	This method clears our replay controller.
 */
void BWReplayConnection::clearReplayController()
{
	if (pReplayController_ != NULL)
	{
		// This will clear pReplayController_
		pReplayController_->destroy();
		MF_ASSERT( pReplayController_ ==  NULL );
	}
}


/**
 *	This method notifies our event handler that replay has terminated,
 *	and cleans up our reference to the handler.
 */
void BWReplayConnection::notifyReplayTerminated(
		BWReplayEventHandler::ReplayTerminatedReason reason )
{
	if (pReplayEventHandler_ == NULL)
	{
		WARNING_MSG( "BWReplayConnection::nofityReplayTerminated: "
			"Called without an event handler (possibly twice)\n" );
		return;
	}

	pReplayEventHandler_->onReplayTerminated( reason );
	pReplayEventHandler_ = NULL;
}


BW_END_NAMESPACE

// bw_replay_connection.cpp

