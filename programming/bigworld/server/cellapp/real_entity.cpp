#include "script/first_include.hpp"

#include "real_entity.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "cell_app_channels.hpp"
#include "entity_population.hpp"
#include "passengers.hpp"
#include "passenger_extra.hpp"
#include "replay_data_collector.hpp"
#include "space.hpp"
#include "witness.hpp"

#include "cstdmf/memory_stream.hpp"

#include "chunk/chunk_space.hpp"

#include "network/bundle.hpp"
#include "network/channel_sender.hpp"
#include "network/compression_stream.hpp"

#include "server/auto_backup_and_archive.hpp"
#include "server/stream_helper.hpp"
#include "server/util.hpp"

#include "baseapp/baseapp_int_interface.hpp"
#include "passengers.hpp"

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

namespace
{
int g_numRealEntities;
int g_numRealEntitiesEver;
}

#if (__cplusplus < 201103L) 
#define decltype typeof
#endif

// -----------------------------------------------------------------------------
// Section: SmartBundle
// -----------------------------------------------------------------------------

namespace
{

/*
 *	This class is used to abstract whether we are sending via the channel
 *	or a once off bundle when sending to our controlledBy_.
 */
class SmartBundle
{
public:
	SmartBundle( RealEntity & realEntity );

	Mercury::Bundle & operator*()  { return sender_.channel().bundle(); }
	operator Mercury::Bundle &() { return sender_.channel().bundle(); }

private:
	SmartBundle();
	SmartBundle & operator=( const SmartBundle & );

	RealEntity & realEntity_;
	Mercury::ChannelSender sender_;
};

/**
 *	Constructor.
 *	@param realEntity The real entity that is sending this bundle.
 */
SmartBundle::SmartBundle( RealEntity & realEntity ) :
	realEntity_( realEntity ),
	sender_( realEntity.controlledByOther() ?
				CellApp::getChannel( realEntity_.controlledByRef().addr ) :
				realEntity_.channel() )
{
	BaseAppIntInterface::setClientArgs::start( *this ).id =
		realEntity_.controlledByOther() ?
			realEntity_.controlledByRef().id : realEntity_.entity().id();
}

} // anonymous namespace



// -----------------------------------------------------------------------------
// Section: RealEntity
// -----------------------------------------------------------------------------

/**
 *	The constructor for RealEntity.
 *
 *	@param owner			The entity associated with this object.
 */
RealEntity::RealEntity( Entity & owner ) :
		entity_( owner ),
		pWitness_( NULL ),
		removalHandle_( NO_ENTITY_REMOVAL_HANDLE ),
		velocity_( 0.f, 0.f, 0.f ),
		positionSample_( owner.position() ),
		positionSampleTime_( CellApp::instance().time() ),
		creationTime_( CellApp::instance().time() ),
		shouldAutoBackup_( AutoBackupAndArchive::YES ),
		pChannel_(
			new Mercury::UDPChannel( CellApp::instance().interface(),
				owner.baseAddr(),
				Mercury::UDPChannel::INTERNAL,
				DEFAULT_INACTIVITY_RESEND_DELAY,
				/* filter: */ NULL,
				Mercury::ChannelID( owner.id() ) ) ),
		recordingSpaceEntryID_()
{
	++g_numRealEntities;
	++g_numRealEntitiesEver;

	pChannel_->isLocalRegular( false );
	pChannel_->isRemoteRegular( false );

	controlledBy_.init();
}


/**
 *	This method initialise the RealEntity. Must be called immediately after
 *	RealEntity is constructed. Return true if ghost position of this entity
 *	needs to be updated.
 */
bool RealEntity::init( BinaryIStream & data, CreateRealInfo createRealInfo,
		Mercury::ChannelVersion channelVersion,
		const Mercury::Address * pBadHauntAddr )
{
	// The following could've been put on by:
	//	- py_createEntity
	//	- py_createEntityFromFile
	//	- eLoad
	//	- Base.createCellEntity
	//	- offloading
	//
	//	This is usually added to the stream using StreamHelper::addRealEntity.

	// Set the channel version if we have one
	if (channelVersion != Mercury::SEQ_NULL)
	{
		pChannel_->version( channelVersion );
		pChannel_->creationVersion( channelVersion );
	}

	bool requireGhostPosUpdate = false;
	bool hasChangedSpace = false;

	bool needsPhysicsCorrection = false;

	switch( createRealInfo )
	{
		case CREATE_REAL_FROM_OFFLOAD:
			needsPhysicsCorrection =
				this->readOffloadData( data, pBadHauntAddr, &hasChangedSpace );
			break;

		case CREATE_REAL_FROM_RESTORE:
			this->readBackupData( data );
			break;

		case CREATE_REAL_FROM_INIT:
			requireGhostPosUpdate = true;
			break;
	}

	uint8 hasWitness;
	data >> hasWitness;
	if (hasWitness == 'W')
	{
		// take control if we are being created from scratch with
		// a Witness in-place ('tho this is never currently done)

		// If this changes in the future, be wary of position being set in the
		// constructor which may occur after createCellPlayer is sent to the
		// client, in which case the initial createCellPlayer message would
		// have a possibly invalid position. We would need to defer the sending
		// of createCellPlayer until after the constructor has finished
		// executing.
		MF_ASSERT( createRealInfo != CREATE_REAL_FROM_INIT );

		if (createRealInfo == CREATE_REAL_FROM_INIT)
			controlledBy_.init( entity_.id(), entity_.baseAddr_,
				controlledBy_.BASE, entity_.pType()->typeID() );

		this->setWitness(
			new Witness( *this, data, createRealInfo, hasChangedSpace ) );

		if (needsPhysicsCorrection)
		{
			this->sendPhysicsCorrection();
		}
	}
	else if (hasWitness != '-')
	{
		ERROR_MSG( "RealEntity::init(%u): "
			"Illegal Witness indicator '%c' (stream corrupt)",
			entity_.id(), hasWitness );
	}

	if (needsPhysicsCorrection)
	{
		// Put a position update on event history for non-volatile entities.
		entity_.updateInternalsForNewPosition( entity_.position() );
	}

	return requireGhostPosUpdate;
}



/**
 *	Destructor.
 */
RealEntity::~RealEntity()
{
	--g_numRealEntities;

	this->entity_.profiler().reset();

	// tell our witness there's nothing more to see here
	if (pWitness_ != NULL)
	{	// don't really need to check if not NULL of course...
		delete pWitness_;
		this->setWitness( NULL );
	}

	entity_.stopRealControllers();

	MF_ASSERT( pChannel_ == NULL );
}

/**
 *  This method deletes this RealEntity. The pNextRealAddr parameter controls
 *  whether the Channel will be deleted immediately (for offloads) or
 *  condemned (all other cases).
 */
void RealEntity::destroy( const Mercury::Address * pNextRealAddr )
{
	// Offloading
	if (pNextRealAddr)
	{
		// Notify all ghosts that this real is about to be offloaded
		for (Haunts::iterator iter = haunts_.begin();
			 iter != haunts_.end(); ++iter)
		{
			Haunt & haunt = *iter;

			if (haunt.addr() != *pNextRealAddr)
			{
				CellAppInterface::ghostSetNextRealArgs & args =
					CellAppInterface::ghostSetNextRealArgs::start(
						haunt.bundle(), entity_.id() );

				args.nextRealAddr = *pNextRealAddr;
			}
		}

		// Clear out the channel's resend history so that when Channel::condemn() is
		// called it is destroyed immediately.  The resend history is now the
		// responsibility of the channel that will be created on the dest app.
		pChannel_->reset( Mercury::Address::NONE, false );
		// delete pChannel_;
		pChannel_->destroy();
	}

	// Destroying
	else
	{
		pChannel_->condemn();
	}

	pChannel_ = NULL;
	delete this;
}


/**
 *	This method is used to stream off data that was added to a stream using
 *	writeOffloadData. Return true if the ghost position of the entity needs
 *	to be updated.
 *
 *	@return Whether a physics correction should be sent to the client. This
 *		needs to be performed by the caller after it gets a witness.
 *
 *	@see writeOffloadData
 */
bool RealEntity::readOffloadData( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr, bool * pHasChangedSpace )
{
	pChannel_->initFromStream( data, entity_.baseAddr() );

	// Get velocity
	data >> velocity_ >> entity_.topSpeed_ >> entity_.topSpeedY_ >>
		entity_.physicsCorrections_;
	entity_.physicsLastValidated_ = timestamp(); // don't offload > once/sec
	//entity_.physicsNetworkJitterDebt_ = 0.f; // intentionally keeps old debt

	uint8 shouldAutoBackup;
	data >> shouldAutoBackup;
	shouldAutoBackup_ = AutoBackupAndArchive::Policy( shouldAutoBackup );

	SpaceID oldSpaceID;
	data >> oldSpaceID;
	bool isTeleported;
	data >> isTeleported;

	bool hasChangedSpace = (oldSpaceID != entity_.space().id());

	if (pHasChangedSpace)
	{
		*pHasChangedSpace = hasChangedSpace;
	}

	// If the data was sent from a previous RealEntity, the data should
	// contain a list of the haunts for this entity.
	unsigned int numHaunts;
	data >> numHaunts;

	MF_ASSERT( numHaunts > 0 );

	bool		areWeHaunted = false;
	CellAppInterface::ghostSetRealArgs setRealArgs;
	setRealArgs.numTimesRealOffloaded = entity_.numTimesRealOffloaded();
	setRealArgs.owner = CellApp::instance().interface().address();

	for (int i = 0; i < (int)numHaunts; i++)
	{
		Mercury::Address	addr;
		data >> addr;

		// we don't care if it's our own address -
		// it could be from a local teleport or some such
		if (addr == CellApp::instance().interface().address())
		{
			areWeHaunted = true;
			continue;
		}

		// Skip address if this is the bad haunt address - this is part of an
		// onload from a failed teleport back to the originating cellapp, and
		// no ghost was created on the destination.
		if (pBadHauntAddr && addr == *pBadHauntAddr)
		{
			WARNING_MSG( "RealEntity::readOffloadData: "
				"Not notifying app %s because of failed teleport for %u\n",
				pBadHauntAddr->c_str(), entity_.id() );
			continue;
		}

		// find the channel for that address
		CellAppChannel * pAppChannel = CellAppChannels::instance().get( addr );

		if ((pAppChannel == NULL) ||
				!pAppChannel->isGood())
		{
			WARNING_MSG( "RealEntity::readOffloadData: "
				"Not notifying ghost for %u on dead app %s\n",
				entity_.id(), addr.c_str() );

			continue;
		}

		Mercury::ChannelSender sender( pAppChannel->channel() );
		Mercury::Bundle & bundle = sender.bundle();

		if (!hasChangedSpace && !isTeleported)
		{
			this->addHaunt( *pAppChannel );

			CellAppInterface::ghostSetRealArgs & rGhostSetRealArgs =
				CellAppInterface::ghostSetRealArgs::start( bundle,
					entity_.id() );

			rGhostSetRealArgs = setRealArgs;
		}
		else
		{
			this->addDelGhostMessage( pAppChannel->bundle() );
		}
	}

	// They should not have offloaded the entity without creating a ghost
	// here first.
	MF_ASSERT( areWeHaunted );

	data >> controlledBy_;

	data >> entity_.profiler();

	CellApp::instance().adjustLoadForOffload( -entity_.profiler().load() );

	entity_.readRealControllersFromStream( data );

	data >> entity_.periodsWithoutWitness_;

	data >> recordingSpaceEntryID_;

	// INVALID_POSITION indicates position is not yet set. This will be
	// done in onTeleportSuccess.
	const bool needsPhysicsCorrection =
		isTeleported && (entity_.localPosition_ != Entity::INVALID_POSITION);

	return needsPhysicsCorrection;
}


/**
 *	This method reads data back from the stream that was written out by
 *	writeBackupData.
 */
void RealEntity::readBackupData( BinaryIStream & data )
{
	CompressionIStream compressedData( data );
	this->readBackupDataInternal( compressedData );
}


/**
 *
 */
void RealEntity::readBackupDataInternal( BinaryIStream & data )
{
	// Note: We revert back to looking like a ghost for this step so that the
	// ghost controllers are not confused. It may be better to review this
	// streaming so that it more logically matches creating a ghost and then
	// creating the real.
	MF_ASSERT( entity_.pReal_ == this );
	entity_.pReal_ = NULL;
	entity_.readGhostControllersFromStream( data );
	entity_.pReal_ = this;

	data >> entity_.topSpeed_ >> entity_.topSpeedY_;
	entity_.physicsLastValidated_ = timestamp();
	entity_.physicsNetworkJitterDebt_ = 0.f;
	data >> controlledBy_;

	uint8 shouldAutoBackup;
	data >> shouldAutoBackup;
	shouldAutoBackup_ = AutoBackupAndArchive::Policy( shouldAutoBackup );

	entity_.readRealControllersFromStream( data );

	data >> entity_.periodsWithoutWitness_;

	// send a physics correction if we are controlled by a client
	// hmmmmmmmmmmmm Murph please check this
	if (controlledBy_.id != NULL_ENTITY_ID)
	{
		this->sendPhysicsCorrection();
	}
}


/**
 *  This method sets the witness associated with this real entity.
 */
void RealEntity::setWitness( Witness * pWitness )
{
	pWitness_ = pWitness;

	if (pChannel_)
	{
		bool isRegular = (pWitness != NULL);
		pChannel_->isLocalRegular( isRegular );
		pChannel_->isRemoteRegular( isRegular );
	}
}


/**
 *	This method should put the relevant data into the input BinaryOStream so
 *	that this entity can be onloaded to another cell. It is mostly read off
 *	in the readOffloadData except for a bit done in our constructor above.
 *
 *	@param data		The stream to place the data on.
 *	@param isTeleport Indicates whether this is a teleport.
 */
void RealEntity::writeOffloadData( BinaryOStream & data, bool isTeleport )
{

	StreamHelper::addRealEntity( data );
	// --------	above here read off in our constructor above

	pChannel_->addToStream( data );

	// Put on velocity
	data << velocity_ << entity_.topSpeed_ << entity_.topSpeedY_
		<< entity_.physicsCorrections_;

	data << uint8( shouldAutoBackup_ );

	// Write current space ID so a change can be detected
	data << entity_.space().id();

	data << isTeleport;

	bool isAlreadyHaunted = false;

	const Mercury::Address & ourAddr =
		CellApp::instance().interface().address();
	for (Haunts::iterator iter = haunts_.begin(); iter != haunts_.end(); ++iter)
	{
		isAlreadyHaunted |= (iter->addr() == ourAddr);
	}

	int numHaunts = haunts_.size();

	if (!isAlreadyHaunted)
	{
		numHaunts += 1;
	}

	data << numHaunts;

	// Send all the addresses it has ghosts on. We don't send cell IDs
	// because if the destination doesn't have a cell ID that we have a
	// ghost on, it needs the address so it can tell the cell to delGhost
	for (Haunts::const_iterator iter = haunts_.begin();
			iter != haunts_.end();
			++iter)
	{
		const Haunt & haunt = *iter;
		data << haunt.addr();
	}

	// Always add our address since we will turn into a ghost after the offload.
	if (!isAlreadyHaunted)
	{
		data << ourAddr;
		//ERROR_MSG( "RealEntity::writeDataToStream: "
		//	"Offload destination does not have a ghost!\n" );
	}

	data << controlledBy_;

	CellApp::instance().adjustLoadForOffload( entity_.profiler().load() );

	data << entity_.profiler();

	entity_.writeRealControllersToStream( data );

	data << entity_.periodsWithoutWitness_;

	data << recordingSpaceEntryID_;

	// ----- below here read off in our constructor above
	if (pWitness_ != NULL)
	{
		data << 'W';
		pWitness_->writeOffloadData( data );
	}
	else
	{
		data << '-';
	}
}


/**
 *	This method adds the backup data associated with this real entity to the
 *	input stream.
 */
void RealEntity::writeBackupData( BinaryOStream & data ) const
{
	StreamHelper::addRealEntity( data );
	// --------	above here read off in our constructor above

	// Scoped so that destructor called before extra streaming.
	{
		CompressionOStream compressedData( data,
			entity_.pType()->description().internalNetworkCompressionType() );
		this->writeBackupDataInternal( compressedData );
	}

	// ----- below here read off in our constructor above
	if (pWitness_ != NULL)
	{
		data << 'W';
		pWitness_->writeBackupData( data );
	}
	else
	{
		data << '-';
	}
}


/**
 *
 */
void RealEntity::writeBackupDataInternal( BinaryOStream & data ) const
{
	entity_.writeGhostControllersToStream( data );

	data << entity_.topSpeed_ << entity_.topSpeedY_ << controlledBy_;
	data << uint8( shouldAutoBackup_ );

	entity_.writeRealControllersToStream( data );

	data << entity_.periodsWithoutWitness_;
}


/**
 *	This method adds or deletes the Witness of this RealEntity.
 */
void RealEntity::enableWitness( BinaryIStream & data, Mercury::ReplyID replyID )
{
	// Send an empty reply to ack this message
	this->channel().bundle().startReply( replyID );

	bool isRestore;
	data >> isRestore;

	if (data.remainingLength() > 0)
	{
		INFO_MSG( "RealEntity::enableWitness: adding witness for %u%s\n",
			entity_.id(), isRestore ? " (restoring)" : "" );

		// take control
		if (controlledBy_.id == NULL_ENTITY_ID)
		{
			controlledBy_.init( entity_.id(), entity_.baseAddr_,
				controlledBy_.BASE, entity_.pType()->typeID() );
		}

		int bytesToClientPerPacket;
		data >> bytesToClientPerPacket;

		MemoryOStream witnessData;
		StreamHelper::addRealEntityWithWitnesses( witnessData,
				bytesToClientPerPacket,
				CellAppConfig::defaultAoIRadius() );

		// make the witness
		MF_ASSERT( pWitness_ == NULL );

		// Delay calls to onEnteredAoI
		Entity::callbacksPermitted( false ); //{

		this->setWitness( new Witness( *this, witnessData,
			isRestore ? CREATE_REAL_FROM_RESTORE : CREATE_REAL_FROM_INIT ) );

		Entity::callbacksPermitted( true ); //}

		Entity::nominateRealEntity( entity_ );
		Script::call( PyObject_GetAttrString( &entity_, "onGetWitness" ),
				PyTuple_New( 0 ), "onGetWitness", true );
		Entity::nominateRealEntityPop();
	}
	else
	{
		this->disableWitness( isRestore );
	}

	if (this->entity().cell().pReplayData())
	{
		this->entity().cell().pReplayData()->addEntityPlayerStateChange( 
			this->entity() );
	}
}


/**
 *	This method disables (destroys) the witness associated with this entity.
 */
void RealEntity::disableWitness( bool isRestore )
{
	INFO_MSG( "RealEntity::disableWitness: %u%s\n",
		entity_.id(), isRestore ? " (restoring)" : "" );

	// break the witness
	MF_ASSERT( pWitness_ != NULL );

	Entity::nominateRealEntity( entity_ );
	Script::call( PyObject_GetAttrString( &entity_, "onLoseWitness" ),
			PyTuple_New( 0 ), "onLoseWitness", true );
	Entity::nominateRealEntityPop();

	delete pWitness_;
	this->setWitness( NULL );

	// lose control
	if (controlledBy_.id == entity_.id())
	{
		controlledBy_.id = NULL_ENTITY_ID;
	}
}


/**
 *	This method adds a message on to the event history.
 */
HistoryEvent * RealEntity::addHistoryEvent( uint8 type,
	MemoryOStream & stream,
	const MemberDescription & description,
	int16 msgStreamSize,
	HistoryEvent::Level level )
{
	HistoryEvent * pNewEvent =
		entity_.addHistoryEventLocally( type, stream, description,
			msgStreamSize, level );

	// Send to ghosts.
	Haunts::iterator iter = haunts_.begin();

	while (iter != haunts_.end())
	{
		Haunt & haunt = *iter;
		Mercury::Bundle & bundle = haunt.bundle();

		uint32 startLength = bundle.size();

		bundle.startMessage( CellAppInterface::ghostHistoryEvent );

		bundle << this->entity().id();

		pNewEvent->addToStream( bundle );

		description.stats().countSentToGhosts( bundle.size() - startLength );

		++iter;
	}

	return pNewEvent;
}


/**
 *  This method adds the delGhost message for this entity to the provided bundle.
 */
void RealEntity::addDelGhostMessage( Mercury::Bundle & bundle )
{
	// TODO: Better handling of prefixed empty messages
	bundle.startMessage( CellAppInterface::delGhost );
	bundle << entity_.id();
}


/**
 *	This method informs all of the Haunts of this entity that they should delete
 *	this entity.
 */
void RealEntity::deleteGhosts()
{
	for (Haunts::iterator iter = haunts_.begin(); iter != haunts_.end(); ++iter)
  	{
		this->addDelGhostMessage( iter->bundle() );
  	}

  	haunts_.clear();
}


/**
 *	This method sends a physics correction to whichever client we are being
 *	controlled by.
 */
void RealEntity::sendPhysicsCorrection()
{
	MF_ASSERT( entity_.localPosition() != Entity::INVALID_POSITION );

	if ((controlledBy_.id == NULL_ENTITY_ID) && (pWitness_ == NULL))
	{
		// Noone to send to.
		return;
	}

	// Don't expect physics correction acknowledgements
	// for server-controlled entities
	if (controlledBy_.id != NULL_ENTITY_ID)
	{
		++entity_.physicsCorrections_;
	}

	if ((Entity::s_physicsCorrectionsOutstandingWarningLevel > 0) &&
		(entity_.physicsCorrections_ ==
			Entity::s_physicsCorrectionsOutstandingWarningLevel))
	{
		WARNING_MSG( "RealEntity::sendPhysicsCorrection(%u): "
			"Client has %u unacknowledged physics corrections.\n",
			 entity_.id_, entity_.physicsCorrections_ );
	}
	else if (entity_.physicsCorrections_ ==
		std::numeric_limits< decltype( entity_.physicsCorrections_ ) >::max())
	{
		ERROR_MSG( "RealEntity::sendPhysicsCorrection(%u): "
			"Client has %u unacknowledged physics corrections, potential "
			"integer wraparound.\n",
			 entity_.id_, entity_.physicsCorrections_ );
	}

	SmartBundle bundle( *this );

	BaseAppIntInterface::forcedPositionArgs & fpa =
		BaseAppIntInterface::forcedPositionArgs::start( bundle );

	fpa.id = entity_.id();
	fpa.spaceID = entity_.space().id();
	fpa.vehicleID = entity_.vehicleID();
	fpa.position = entity_.localPosition();
	fpa.direction = entity_.localDirection();
}


/**
 *	This method informs us that our entity is now at a new position.
 *	We may use this opportunity to update our velocity record.
 */
void RealEntity::newPosition( const Vector3 & position )
{
	if (pWitness_ != NULL)
		pWitness_->newPosition( position );

	GameTime now = CellApp::instance().time();

	uint32 dur = now - positionSampleTime_;
	if (dur > 1)
	{
		if (positionSample_ == Entity::INVALID_POSITION)
		{
			positionSample_ = position;
		}

		float tinv = float(CellAppConfig::updateHertz()) / float(dur);
		velocity_ = (position - positionSample_) * tinv;
		positionSample_ = position;
		positionSampleTime_ = now;
	}
}


/**
 *  This method adds a haunt for this real on the provided channel.
 */
void RealEntity::addHaunt( CellAppChannel & channel )
{
	haunts_.push_back( Haunt( &channel, CellApp::instance().time() ) );
}


/**
 *  This method deletes a haunt for this real.  It returns an iterator that
 *  points to the next haunt in the sequence.
 */
RealEntity::Haunts::iterator RealEntity::delHaunt( Haunts::iterator iter )
{
	return haunts_.erase( iter );
}


/**
 *	This method is called periodically to automatically back up this entity.
 */
void RealEntity::autoBackup()
{
	if (shouldAutoBackup_ == AutoBackupAndArchive::NO)
	{
		return;
	}

	this->backup();

	if (shouldAutoBackup_ == AutoBackupAndArchive::NEXT_ONLY)
	{
		shouldAutoBackup_ = AutoBackupAndArchive::NO;
	}
}


/**
 *	This method backs up the data of this entity so that it can be recreated
 *	in the event that this cell application crashes.
 */
void RealEntity::backup()
{
#if ENABLE_PROFILER
	ScopedProfiler __backupProfiler( entity_.pType()->name() );
#endif
	if (entity_.hasBase())
	{
		Mercury::Bundle & bundle = pChannel_->bundle();
#if ENABLE_WATCHERS
		int initialBundleSize = bundle.size();
#endif
		BaseAppIntInterface::setClientArgs setClientArgs = { entity_.id() };
		bundle << setClientArgs;
		bundle.startMessage( BaseAppIntInterface::backupCellEntity );

		this->writeBackupProperties( bundle );
#if ENABLE_WATCHERS
		entity_.pEntityType_->stats().countSentToBase( bundle.size() -
													   initialBundleSize);
#endif
		pChannel_->send();
	}
}


/**
 *  This method adds the information for a backup cell entity to a stream.
 *  It is used both in backups and in writeToDB since it requires the entity
 *  to be backed up before writing to the database.
 *  Since all DB data can be extracted from backup data, it will logically be
 *  exactly the same data sent in exactly the same format for both operations.
 */
void RealEntity::writeBackupProperties( BinaryOStream & data ) const
{
	// Indicate whether there is witness data on this backup. This is
	// needed because BaseApp stores away this stream opaquely, and sends
	// it back to a CellApp on restore; but it needs to know whether
	// witness data exists so it can correct the witness state on restore
	// if necessary.
	data << bool( pWitness_ != NULL );

	StreamHelper::AddEntityData addEntityData( entity_.id(),
											   entity_.localPosition_,
											   entity_.isOnGround_, 
											   entity_.entityTypeID(),
											   entity_.localDirection_,
											   false, /* hasTemplate */
											   entity_.baseAddr_ );
	StreamHelper::addEntity( data, addEntityData );

	int32 cellClientSize = 0;

	// writeToDB concerns itself only with the following data, thus it will
	// skim over the preceding data in the cell backup to get here.
	const EntityDescription & description =
		entity_.pEntityType_->description();

	// Populate local and delegate properties to dict and ...
	ScriptDict tempDict = entity_.createDictWithProperties(
			EntityDescription::CELL_DATA );
	
	if (!tempDict)
	{
		CRITICAL_MSG( "RealEntity::writeBackupProperties(%d): "
				"Failed to populate entity attributes to dict\n",
			entity_.id() );
	}
	
	// TODO: It would be good to have this compressed. It's hard to do since
	// part of the backup data (of size cellClientSize) is sent to the client.
	// Read by:
	//     Entity::readRealDataInEntityFromStreamForInitOrRestore
	//     Base::getDBCellData
	if (!description.addDictionaryToStream( tempDict, data,
			EntityDescription::CELL_DATA, &cellClientSize, 1 ))
	{
		// NOTE: If anyone ever downgrades this from a CRITICAL, you need to
		// use a local data instead of the channel's data for this
		// message because of this early return, otherwise you risk leaving
		// the channel's data in a bad state.
		CRITICAL_MSG( "Entity::backup: "
				"addDictionaryToStream failed for %s %u\n", 
			entity_.pType()->name(),
			entity_.id_ );
		return;
	}

	data << entity_.volatileInfo_;

	this->writeBackupData( data );

	StreamHelper::CellEntityBackupFooter
		backupFooter( cellClientSize, entity_.vehicleID() );
	data << backupFooter;

	// Display a warning if this is getting too large
	{
		static int maxBackupSize = 10000;
		int dataSize = data.size();
		if (dataSize > maxBackupSize)
		{
			maxBackupSize = dataSize;
			WARNING_MSG( "Entity::backup: "
						 "New maximum backup size of %d bytes by %s %u\n",
						 maxBackupSize, description.name().c_str(), 
						 entity_.id_ );
		}
		else if (dataSize > g_profileBackupSizeLevel)
		{
			WARNING_MSG( "Profile backup/sizeWarningLevel exceeded "
						 "(%s %u of size %d)\n",
						 description.name().c_str(), entity_.id_, dataSize );
		}
	}
}


/**
 *	This method is used to dump debug information.
 */
void RealEntity::debugDump()
{
	if (pWitness_ != NULL)
		pWitness_->debugDump();
}


// -----------------------------------------------------------------------------
// Section: RealEntity's Python methods (previously in Entity)
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( RealEntity )

PY_BEGIN_METHODS( RealEntity )
	/*~ function Entity teleport
	*  @components{ cell }
	*  The teleport function sets the position and orientation of the Entity.
	*  Optionally, a Mailbox can be provided to this function which
	*  refers to a cell entity that should be near the Entity's expected
	*  destination (and in the desired destination space).
	*
	*  Note that Entities which have a volatileInfo that does not allow
	*  transmission of position data will send their updated position to clients
	*  in an inefficient way. Care should be taken not to call this method
	*  continuously on a non-volatile entity.
	*
	*  The teleport function can only be called on real entities.
	*  @param nearbyMBRef A Mailbox that determines which Space and CellApp the
	*  Entity is teleporting to, as it is assumed to be near the teleport
	*  destination. This can be set to None, in which case it will remain
	*  on the current cell until cell migration occurs via normal means.
	*  @param position The position argument is the location to which the Entity is
	*  to be teleported, as a sequence of three floats (x, y, z), in world space.
	*  @param direction The direction argument is the direction to which the Entity
	*  is to be facing after the teleport, as a sequence of three floats ( roll,
	*  pitch, yaw), in world space.
	*  @return Always returns None.
	*/
	PY_METHOD( teleport )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RealEntity )
	/*~ attribute Entity controlledBy
	*  @components{ cell }
	*  This attribute is the BaseEntityMailBox of the client that is controlling
	*  the normal movement of this entity. If only the server is controlling it,
	*  then it is None. When a client attaches to a base with giveClientTo
	*  (or via login), it is automatically set to the base of that client.
	*
	*  If a script makes changes to the pose of an entity, e.g. sets its
	*  direction directly, or teleports it to another space, then those
	*  changes are sent down to the client as a forced pose change. Future
	*  pose updates from the client are ignored until the forced change is
	*  acknowledged. This same mechanism is used if the physics checking
	*  system rules a client-initiated movement illegal.
	*
	*  Scripts may set this on any (real) entity to grant a client control
	*  of that entity - or revoke it. It is even possible to revoke or redirect
	*  the client's control of its own entity, in which case its position
	*  (if volatile) will be sent to the client in every update. (Any volatile
	*  positions of vehicles the client is on will also be sent every update.)
	*
	*  @type BaseEntityMailBox
	*/
	PY_ATTRIBUTE( controlledBy )

	/*~ attribute Entity artificialMinLoad
	 *	@components{ cell }
	 *
	 *	This attribute allows this entity to report an amount of
	 *	artificial minimal load. Setting a value to greater than 0 guarantees
	 *	the reported load for the given entity will not be less than the
	 *	artificial minimal load.
	 *
	 *	The value should be in range of [0, +inf). Values less than 1e-6
	 *	will be changed to 0. The default value for this attribute is 0
	 *	which effectively disables artificial load.
	 *
	 *	It is useful as a hint to the load balancing if it is expected that
	 *	this entity's load may increase in the future.
	 *
	 *	Any added load contributes directly to the load of the local CellApp
	 *	process. This means that the units are the fraction of the entire
	 *	CellApp load that should be artificially accounted. This does assume
	 *	that the CellApp machines are fairly compatible.
	 *
	 *	The value should generally be small, say 0.01 or less.
	 *
	 *	This feature should be used with care. Setting this value too large,
	 *	for example, could adversely affect the load balancing.
	 */
	PY_ATTRIBUTE( artificialMinLoad )

	/*~ attribute Entity artificialLoad
	 *	@components{ cell }
	 *
	 *	This attribute has been deprecated.
	 */
	PY_ATTRIBUTE_ALIAS( artificialMinLoad, artificialLoad )

	/*~ attribute Entity isWitnessed
	 *	@components{ cell }
	 *	This read-only attribute indicates whether or not this real entity is
	 *	considered as being witnessed. If the state of this property changes,
	 *	Entity.onWitnessed is called.
	 *
	 *	An entity is considered as being witnessed if it in a client-controlled
	 *	entity's Area of Interest (AoI). This attribute is False when the
	 *	entity is first created and will be set to True immediately upon
	 *	entering the AoI of a client-controlled entity. However, BigWorld uses
	 *	a lazy algorithm to change this attribute from True to False.
	 *	Currently, it can take between 8 to 12 minutes after leaving the AoI of
	 *	the last client-controlled entity before this attribute is set to
	 *	False.
	 */
	PY_ATTRIBUTE( isWitnessed )

	/*~ attribute Entity hasWitness
	 *	@components{ cell }
	 *	This read-only attribute indicates whether or not this real entity
	 *	has a witness object attached. Witness objects relay the world as seen
	 *	from the point of view of the entity to a client, via a base entity
	 *
	 *	@type boolean
	 */
	PY_ATTRIBUTE( hasWitness )

	/*~ attribute Entity shouldAutoBackup
	 *	@components{ cell }
	 *	This attribute determines the policy for automatic backups. 
	 *	If set to True, automatic backups are enabled, if set to False,
	 *	automatic backups are disabled. 
	 *	If set to BigWorld.NEXT_ONLY, automatic backups are enabled for the
	 *	next scheduled time; after the next automatic backup, this is attribute
	 *	is set to False.
	 *
	 *	This can be set to True, False or BigWorld.NEXT_ONLY.
	 */
	PY_ATTRIBUTE( shouldAutoBackup )
PY_END_ATTRIBUTES()

PY_FAKE_PYOBJECTPLUS_BASE( RealEntity )


/**
 *	This method handles a message telling us to teleport to another space
 *	indicated by a cell mailbox.
 */
void RealEntity::teleport( const EntityMailBoxRef & dstMailBoxRef )
{
	Vector3 direction( 0.f, 0.f, 0.f );

	if (!this->teleport( dstMailBoxRef, Entity::INVALID_POSITION, direction ))
	{
		ERROR_MSG( "RealEntity::teleport: Failed\n" );
		PyErr_Print();
	}
}


/**
 *	This method allows scripts to teleport this entity to another
 *	location in the world - possibly to another space.
 *	It sets a Python error if it fails.
 */
bool RealEntity::teleport( const EntityMailBoxRef & nearbyMBRef,
	const Vector3 & position, const Vector3 & direction )
{
	// make sure the input position is valid
	bool isValidPos = Entity::isValidPosition( position ) ||
		(position == Entity::INVALID_POSITION && nearbyMBRef.id != 0);

	if (!isValidPos)
	{
		char numbuf[128];
		bw_snprintf( numbuf, sizeof(numbuf), "(%8.4E, %8.4E, %8.4E)",
				position.x, position.y, position.z );
		PyErr_Format( PyExc_ValueError, "Entity.teleport() "
			"Cannot teleport to position %s\n", numbuf );
		return false;
	}

	// make sure the mailbox is what we want
	if (nearbyMBRef.id == 0)
	{
		Vector3 localPos = position;
		// XXX: This constructor takes its input as (roll, pitch, yaw)
		Direction3D localDir( direction );

		EntityPtr pVehicle = entity_.pVehicle();
		if (pVehicle != NULL)
		{
			Passengers & passengers = Passengers::instance( *pVehicle );
			passengers.transformToVehiclePos( localPos );
			passengers.transformToVehicleDir( localDir );
		}

		entity_.setPositionAndDirection( localPos, localDir );

		entity_.onTeleportSuccess( NULL );

		// If the entity is non-volatile, then the above call to
		// setPositionAndDirection will have added the detailed position.
		if (entity_.volatileInfo().hasVolatile( 0.f ))
		{
			entity_.addDetailedPositionToHistory( /* isLocalOnly */ false );
		}

		this->sendPhysicsCorrection();

		return true;
	}

	if (nearbyMBRef.component() != nearbyMBRef.CELL)
	{
		PyErr_SetString( PyExc_TypeError, "Entity.teleport() "
			"Cannot teleport near to non-cell entities" );
		return false;
	}

	if (Passengers::instance.exists( entity_ ))
	{
		PyErr_SetString( PyExc_ValueError, "Entity.teleport() "
				"Cannot teleport vehicles. "
				"Consider just setting position if not changing spaces" );
		return false;
	}

	// Note: This may actually be a channel that leads back to this CellApp, but
	// this is OK.  Having a circular channel works fine when single-threaded.
	CellAppChannel * pChannel =
		CellAppChannels::instance().get( nearbyMBRef.addr );

	if (pChannel == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "Entity.teleport() "
				"Invalid destination mailbox" );
		return false;
	}

	entity().relocated();

	// reset entity vehicle if there is one. For player avatar, if the
	// teleporting destination is a vehicle, client will automatic inform us
	// that in next cycle. For normal NPC teleported to another vehicle,
	// script should make sure another boardVehicle is called.

	if (entity_.pVehicle())
	{
		if (!PassengerExtra::instance( entity_ ).alightVehicle())
		{
			PyErr_Clear();
		}
	}

	// Save our old pos and dir and temporarily set to the teleport ones
	Vector3 oldPos = entity_.localPosition_;
	Direction3D oldDir = entity_.localDirection_;
	entity_.localPosition_ = position;
	// XXX: This constructor takes its input as (roll, pitch, yaw)
	entity_.localDirection_ = Direction3D( direction );

	// Delete all ghosts
	while (!haunts_.empty())
	{
		// Delete ghosts from other haunts - they are recreated when the
		// entity is created on the destination CellApp. We do this because
		// of issues when the teleport failure occurs.
		CellAppChannel * pCellAppChannel =
			CellAppChannels::instance().get( haunts_.front().addr(),
			/* shouldCreate = */ false );

		Mercury::Bundle & hauntBundle = pCellAppChannel->bundle();
		this->addDelGhostMessage( hauntBundle );
		this->delHaunt( haunts_.begin() );
	}

	recordingSpaceEntryID_ = SpaceEntryID();

	// Call this before the message is started so that the channel is in a good
	// state. This should also be above the creation of the new ghost. If not,
	// ghosted property changes would arrive before the ghost is created. 
	entity_.callback( "onLeavingCell" );

	Mercury::ChannelSender sender( pChannel->channel() );
	Mercury::Bundle & bundle = sender.bundle();

	// Re-add haunt for destination.
	this->addHaunt( *pChannel );

	// We always have to create the ghost anew, even if there's already one
	// there, because we have no idea which space the entity on the cellapp
	// we're going to is in.

	MemoryOStream ghostDataStream;
	entity_.writeGhostDataToStream( ghostDataStream );

	bundle.startMessage( CellAppInterface::onloadTeleportedEntity );
	bundle << nearbyMBRef.id;

	// so the receiver knows where the onload message starts
	bundle << uint32( ghostDataStream.remainingLength() );

	bundle.transfer( ghostDataStream, ghostDataStream.remainingLength() );

	// And offload it
	// Save a reference since offloadEntity decrefs 'this'!
	EntityPtr pThisEntity = &entity_;

	pThisEntity->population().rememberBaseChannel( *pThisEntity, pChannel->addr() );

	pThisEntity->cell().offloadEntity( pThisEntity.get(), pChannel,
			/* isTeleport: */ true );

	// Restore our old pos and dir
	pThisEntity->localPosition_ = oldPos;
	pThisEntity->localDirection_ = oldDir;

	return true;
}


/**
 *	Return who controls us, or None if we have no master
 */
BaseEntityMailBoxPtr RealEntity::controlledBy()
{
	if (controlledBy_.id == NULL_ENTITY_ID)
	{
		return NULL;
	}

	// only reason we go through the smart pointer here is to get
	// the auto type checking for the set accessor below
	return BaseEntityMailBoxPtr( (BaseEntityMailBox*)
		BaseEntityMailBox::constructFromRef( controlledBy_ ),
		/*alreadyIncremented:*/true );
}


/**
 *	This method sends a message to the appropriate Entity notifying it
 *	of a change of control.
 *
 *	@param hasControl  Whether control was gained or lost.
 */
void RealEntity::notifyWardOfControlChange( bool hasControl )
{
	if (pWitness_ == NULL || controlledBy_.id != entity_.id())
	{
		SmartBundle bundle( *this );

		BaseAppIntInterface::modWardArgs & mwa =
			BaseAppIntInterface::modWardArgs::start( bundle );
		mwa.id = entity_.id();
		mwa.on = hasControl;
	}
	else
	{
		MemoryOStream m;
		m << entity_.id() << bool( hasControl );
		pWitness_->sendToProxy( BaseAppIntInterface::modWard.id(), m);
	}
}


/**
 *	Set who controls us, to None if we should have no master
 */
void RealEntity::controlledBy( BaseEntityMailBoxPtr pNewMaster )
{
	// first see if they are the same
	if (!pNewMaster && controlledBy_.id == NULL_ENTITY_ID)
	{
		return;
	}

	if (pNewMaster &&
		controlledBy_.id == pNewMaster->id() &&
		controlledBy_.addr == pNewMaster->address())
	{
		return;
	}

	// tell our old master it's been kicked out
	if (controlledBy_.id != NULL_ENTITY_ID)
	{
		this->notifyWardOfControlChange( /*hasControl*/ false );
	}

	// set it
	if (pNewMaster)
	{
		controlledBy_ = pNewMaster->ref();
	}
	else
	{
		controlledBy_.id = NULL_ENTITY_ID;
	}

	// tell our new master it has taken the reins
	if (controlledBy_.id != NULL_ENTITY_ID)
	{
		this->notifyWardOfControlChange( /*hasControl*/ true );
	}
}


/**
 *	This method is called to inform us that the controlling entity no longer
 *	exists.
 */
void RealEntity::delControlledBy( EntityID deadID )
{
	if (controlledBy_.id == deadID)
	{
		controlledBy_.id = NULL_ENTITY_ID;
		controlledBy_.addr = Mercury::Address( 0, 0 );
		Entity::nominateRealEntity( entity_ );
		Script::call( PyObject_GetAttrString( &entity_, "onLoseControlledBy" ),
				Py_BuildValue( "(i)", deadID ),
				"onLoseControlledBy", true );
		Entity::nominateRealEntityPop();
	}
	else
	{
		WARNING_MSG( "RealEntity::delControlledBy: "
					"%u does not match current controller %u\n",
			   deadID, controlledBy_.id	);
	}
}


/**
 *	This methods returns whether or not this entity is currently considered as
 *	being witnessed.
 */
bool RealEntity::isWitnessed() const
{
	return entity_.periodsWithoutWitness_ < Entity::NOT_WITNESSED_THRESHOLD;
}


/**
 *	This static method adds the watchers associated with real entities.
 */
void RealEntity::addWatchers()
{
	MF_WATCH( "stats/numRealEntities", g_numRealEntities );
	MF_WATCH( "stats/totalRealEntitiesEver", g_numRealEntitiesEver );
	Witness::addWatchers();
}

BW_END_NAMESPACE


// real_entity.cpp
