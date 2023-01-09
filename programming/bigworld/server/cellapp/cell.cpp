#include "script/first_include.hpp"

#ifdef _WIN32
// TODO: Fix these warnings
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning( disable: 4244 )

// 'this' : used in base member initializer list
#pragma warning (disable: 4355)
#endif

#include "cell.hpp"

#include "buffered_entity_messages.hpp"
#include "buffered_input_messages.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cellapp_interface.hpp"
#include "cell_app_channels.hpp"
#include "entity.hpp"
#include "entity_population.hpp"
#include "offload_checker.hpp"
#include "space.hpp"
#include "real_entity.hpp"
#include "replay_data_collector.hpp"
#include "witness.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"
#include "common/space_data_types.hpp"
#include "cstdmf/memory_stream.hpp"
#include "math/mathdef.hpp"
#include "network/channel_sender.hpp"
#include "script/script_object.hpp"

#include <algorithm>
#include <limits>

#include <stdlib.h>

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "cell.ipp"
#endif

namespace
{
inline
const char * asString( bool value )
{
	return value ? "True" : "False";
}
}


// -----------------------------------------------------------------------------
// Section: Constructor/Destructor
// -----------------------------------------------------------------------------

/**
 *	The constructor for Cell.
 *
 *	@param space	The space that this cell is in.
 *	@param cellInfo	The information associated with this cell.
 */
Cell::Cell( Space & space, const CellInfo & cellInfo ) :
	realEntities_(),
	shouldOffload_( true ),
	lastERTFactor_( 1.f ),
	lastERTCalcTime_( 0 ),
	initialTimeOfDay_( 0.f ),
	gameSecondsPerSecond_( 0.f ),
	isRetiring_( false ),
	isRemoved_( false ),
	space_( space ),
	backupIndex_( 0 ),
	pCellInfo_( &cellInfo ),
	pendingAcks_(),
	receivedAcks_(),
	pReplayData_( NULL ),
	stoppingState_( STOPPING_STATE_NOT_STOPPING )
{
	// TODO: Add this in once cell app manager is fixed.
	// MF_ASSERT( space_.pCell() == NULL );
	space_.pCell( this );
}


/**
 *	The destructor for Cell.
 */
Cell::~Cell()
{
	TRACE_MSG( "Cell::~Cell: for space %u\n", space_.id() );

	while (!realEntities_.empty())
	{
		int prevSize = realEntities_.size();

		realEntities_.front()->destroy();

		MF_ASSERT( prevSize > (int)realEntities_.size() );

		if (prevSize <= (int)realEntities_.size())
		{
			break;
		}
	}

	bw_safe_delete( pReplayData_ );

	MF_ASSERT_DEV( space_.pCell() == this );

	space_.pCell( NULL );
}


/**
 *
 */
void Cell::shutDown()
{
	while (!realEntities_.empty())
	{
		realEntities_.front()->destroy();
	}
}


/**
 *	This method returns whether this cell is ready to be deleted.
 */
bool Cell::isReadyForDeletion() const
{
	return
		// Don't delete if there are any possible pending entity creations.
		CellApp::instance().bufferedEntityMessages().isEmpty() &&		
		CellApp::instance().bufferedInputMessages().isEmpty() &&		
		isRemoved_ && 
		realEntities_.empty() &&
		space_.spaceEntities().empty() &&
		pendingAcks_.empty();
}


// -----------------------------------------------------------------------------
// Section: Time slices
// -----------------------------------------------------------------------------


/**
 *	We want to periodically update the CellApps that we have created ghosts
 *	for our reals on, and also offload them there if appropriate.
 *
 *	@return True if this cell should be killed, otherwise false.
 */
bool Cell::checkOffloadsAndGhosts()
{
	OffloadChecker offloadChecker( *this );
	offloadChecker.run();

	return this->isReadyForDeletion();
}


// -----------------------------------------------------------------------------
// Section: Entity Maintenance C++ methods
// -----------------------------------------------------------------------------

/**
 *	This method moves a real entity from this cell to an adjacent cell.
 *
 *	@param pEntity The entity to offload.
 *	@param pChannel The channel to send on.
 *	@param isTeleport Indicates whether this is a teleport
 */
void Cell::offloadEntity( Entity * pEntity, CellAppChannel * pChannel,
	   bool isTeleport )
{
	AUTO_SCOPED_PROFILE( "offloadEntity" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );

	// TRACE_MSG( "Cell::offloadEntity: id %d to cell %s\n", pEntity->id(),
	//		pChannel->address().c_str() );

	// Make sure it's real.
	MF_ASSERT( pEntity->pReal() != NULL );

	// Make sure the entity doesn't have a zero refcount when between lists.
	EntityPtr pCopy = pEntity;

	// If teleporting, this has already been called so that the channel is not
	// left with a partially streamed message.
	if (!isTeleport)
	{
		pEntity->callback( "onLeavingCell" );
	}

	// Move the entity from being real to a ghost.
	if (pEntity->isReal())
	{
		if (pReplayData_ && isTeleport)
		{
			pReplayData_->deleteEntity( pEntity->id() );
		}

		realEntities_.remove( pEntity );
		pEntity->offload( pChannel, isTeleport );
		pEntity->callback( "onLeftCell" );
	}
}


/**
 *	This method adds the input entity to the cell's internal list of real
 *	entities.
 *
 *	It is called from Cell::createEntity and by the Entity itself when it is
 *  onloaded.
 */
void Cell::addRealEntity( Entity * pEntity, bool shouldSendNow )
{
	if (!pEntity->isReal())
	{
		ERROR_MSG( "Cell::addRealEntity called on ghost entity id %u!\n",
			pEntity->id() );
		return;
	}

	pEntity->informBaseOfAddress( CellApp::instance().interface().address(),
		this->spaceID(), shouldSendNow );

	realEntities_.add( pEntity );
}


/**
 *	This method does everything that the cell has to do when a real entity is
 *	destroyed. Basically, it just removes it from the map of real entities.
 *
 *	It assumes that the entity's internal 'die' method has already been called.
 */
void Cell::entityDestroyed( Entity * pEntity )
{
	if (!pEntity->isDestroyed())
	{
		ERROR_MSG( "Cell::entityDestroyed: "
			"Someone tried to murder living entity id %u!\n", pEntity->id() );
		return;
	}

	if (!pEntity->isReal())
	{
		ERROR_MSG( "Cell::entityDestroyed called on ghost entity id %u!\n",
			pEntity->id() );
		return;
	}

	if (!realEntities_.remove( pEntity ))
	{
		WARNING_MSG( "Cell::entityDestroyed: Failed to remove entity %u\n",
				pEntity->id() );
	}

	INFO_MSG( "Cell::entityDestroyed: %s %u has been destroyed.\n",
		pEntity->pType()->name(), pEntity->id() );
}


/**
 *	This method creates a new real entity on this cell according to the
 *	parameters in 'data'. This method is meant to be called internally
 *	within the cell. It returns a pointer to the new Entity.
 *
 *	@param data		The stream containing the data used to create the entity.
 *	@param properties	A ScriptDict containing property values when created
 *						by local game script.
 *  @param isRestore	Boolean indicating whether the entity is being created
 *                      due fault tolerance.
 *	@param channelVersion Optional channel version, passed when restoring.
 *	@param pNearbyEntity	A pointer to a nearby entity to use when creating
 *							the real entity.
 *
 *  @return			A pointer to the entity.
 */
EntityPtr Cell::createEntityInternal( BinaryIStream & data,
		const ScriptDict & properties,
		bool isRestore, Mercury::ChannelVersion channelVersion,
		EntityPtr pNearbyEntity )
{
	AUTO_SCOPED_PROFILE( "createEntity" );

	EntityID id;
	EntityTypeID entityTypeID;
	data >> id >> entityTypeID;

	// If the ID is unknown, allocate one.
	bool shouldAllocateID = (id == 0);
	if (shouldAllocateID)
	{
		id = CellApp::instance().idClient().getID();

		if (id == 0)
		{
			ERROR_MSG( "Cell::createEntityInternal: No free IDs!\n" );
			return NULL;
		}
	}

	// If the (non-destroyed) entity already exists, bail early.
	EntityPopulation::const_iterator found = Entity::population().find( id );
	if (found != Entity::population().end() && !found->second->isDestroyed())
	{
		EntityPtr pExisting = found->second;

		// If we are restoring and there is already a ghost here, we can assume
		// that it is a zombie and the real has been lost (critical channels
		// guarantee this).  There is still a chance that zombie ghosts may be
		// left around on other CellApps.  The problem and solution are detailed
		// in http://bugs.bigworldtech.com/show_bug.cgi?id=14827
		if (!pExisting->isReal() && isRestore)
		{
			WARNING_MSG( "Cell::createEntityInternal: "
				"Destroying zombie ghost %u "
				"(expected real on %s) prior to restore\n",
				pExisting->id(), pExisting->realAddr().c_str() );

			while (!pExisting->isDestroyed())
			{
				// Destroying could trigger buffered ghost messages and this
				// could trigger recreation of the entity.

				pExisting->destroyZombie();
			}
		}
		else
		{
			WARNING_MSG( "Cell::createEntityInternal: %s %u already exists!\n",
				found->second->isReal() ? "Real" : "Ghost", found->second->id() );

			// You should never be able to hit this branch during a restore
			// because the critical channel stuff is designed to prevent you
			// ever restoring an entity that is still alive.
			MF_ASSERT( !isRestore );

			data.finish();

			return found->second;
		}
	}

	// Build up the Entity structure
	EntityPtr pNewEntity = space_.newEntity( id, entityTypeID );

	if (!pNewEntity)
	{
		return NULL;
	}

	MF_ASSERT( pNewEntity->nextInChunk() == NULL );
	MF_ASSERT( pNewEntity->prevInChunk() == NULL );
	MF_ASSERT( pNewEntity->pChunk() == NULL );

	Entity::callbacksPermitted( false ); // {

	if (!pNewEntity->initReal( data, properties, isRestore, channelVersion,
				pNearbyEntity ))
	{
		pNewEntity->setShouldReturnID( shouldAllocateID );
		pNewEntity->decRef();

		// TODO: Make a callbacksPermitted lock class to help manage the pairing
		// of these calls
		Entity::callbacksPermitted( true );

		return NULL;
	}

	// And add it to our list of reals
	// TODO: If init method destroyed or teleported this entity
	// then we should not do this (and destroying/offloading
	// would have caused an assert when it removed the entity anyway)
	//
	// Delay sending the currentCell message to the base so that it will be on
	// the same bundle as the backup. This guarantees that there is no window
	// between losing cellData and getting the first backup data.
	this->addRealEntity( pNewEntity.get(), /*shouldSendNow:*/false );

	INFO_MSG( "Cell::createEntityInternal: %s %-10s (%u)\n",
			isRestore ? "Restored" : "New",
			pNewEntity->pType() ?
				pNewEntity->pType()->name() : "INVALID",
			pNewEntity->id() );

	Entity::population().notifyObservers( *pNewEntity );

	Entity::callbacksPermitted( true ); // }

	if (pReplayData_)
	{
		if (isRestore)
		{
			// If we delete an entity that hasn't been created yet, BWEntities
			// (and pre-connection_model PC client) will silently ignore it.
			pReplayData_->deleteEntity( pNewEntity->id() );
		}

		// This needs to be after Entity::callbacksPermitted( true ) above, so
		// that any state set in the Python constructor is reflected in the
		// initial state of the recorded entity.
		pReplayData_->addEntityState( *pNewEntity );
	}

	// Note: This causes the currentCell message from above to be sent too.
	if (pNewEntity->pReal())
	{
		pNewEntity->pReal()->backup();	// Send backup to BaseApp immediately.
	}

	return pNewEntity;
}


/**
 *	This method is called to write backup data for real entities on this cell.
 *	The cell should back up part of itself each time this is called.
 *
 *	@param	index	A value in the range [0, period - 1] that increments with
 *					each call.
 *	@param	period	Indicates what the index can go up to.
 */
void Cell::backup( int index, int period )
{
	// One thought with this is to have a trigger in the range list. We traverse
	// from here when we want to back up an entity. If a real entity moves
	// 'behind' the trigger, we can back it up then also.
	int newBackupIndex = realEntities_.size() * (index + 1)/ period;

	if (index == 0)
	{
		backupIndex_ = 0;
	}

	if (backupIndex_ < newBackupIndex)
	{
		Entities::const_iterator iter = realEntities_.begin() + backupIndex_;
		Entities::const_iterator end = realEntities_.begin() + newBackupIndex;

		while (iter != end)
		{
			(*iter)->pReal()->autoBackup();

			++iter;
		}
	}

	backupIndex_ = newBackupIndex;
}


// -----------------------------------------------------------------------------
// Section: Communication message handlers concerning entities
// -----------------------------------------------------------------------------

/**
 *	This method creates a new real entity on this cell according to the
 *	parameters in 'data'.
 *
 *	@param srcAddr	The address from which this request originated.
 *	@param header	The mercury header
 *	@param data		The data stream
 *	@param pNearbyEntity A pointer to a nearby entity to use during entity
 *						creation.
 *
 *	@see createEntityInternal
 */
void Cell::createEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data,
		EntityPtr pNearbyEntity )
{
	Mercury::ChannelVersion channelVersion = Mercury::SEQ_NULL;
	data >> channelVersion;

	bool isRestore;
	data >> isRestore;

	EntityPtr pEntity = this->createEntityInternal( data, ScriptDict(),
		isRestore, channelVersion, pNearbyEntity );

	if (isRestore)
	{
		return;
	}

	if (header.replyID == Mercury::REPLY_ID_NONE)
	{
		WARNING_MSG( "Cell::createEntity: Handling non-request createEntity\n" );
		return;
	}

	if (pNearbyEntity == NULL)
	{
		// Only CellAppMgr sends without a nearby entity.
		CellAppMgrGateway & cellAppMgr = CellApp::instance().cellAppMgr();

		cellAppMgr.bundle().startReply( header.replyID );
		cellAppMgr.bundle() << (pEntity ? pEntity->id() : NULL_ENTITY_ID);
		cellAppMgr.send();
		return;
	}

	Mercury::Channel * pChannel = header.pChannel.get();
	
	if (pEntity &&
			pEntity->isReal() &&
			pEntity->pReal()->channel().isConnected())
	{
		// Use the base entity channel if we have it.
		pChannel = &(pEntity->pReal()->channel());
	}

	if (pChannel->isDestroyed())
	{
		// Our channel object might have been destroyed due to this message
		// being buffered.
		WARNING_MSG( "Cell::createEntity: Channel %s has been destroyed, "
				"cannot send reply\n",
			pChannel->c_str() );

		return;
	}

	Mercury::Bundle & bundle = pChannel->bundle();

	bundle.startReply( header.replyID );
	bundle << (pEntity ? pEntity->id() : NULL_ENTITY_ID);

	pChannel->send();
}


// -----------------------------------------------------------------------------
// Section: Instrumentation
// -----------------------------------------------------------------------------

/**
 *	Static method to get a generic watcher for the class Cell.
 */
WatcherPtr Cell::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		Cell * pNull = NULL;

		pWatcher->addChild( "numReals",
				makeWatcher( pNull->realEntities_, &Entities::size ) );

		pWatcher->addChild( "spaceID", makeWatcher( &Cell::spaceID ) );

		pWatcher->addChild( "rect", makeWatcher( &Cell::rect ) );

		Watcher * watchREntities =
			new SequenceWatcher< Entities >( pNull->realEntities_ );
		watchREntities->addChild( "*",
				new BaseDereferenceWatcher( Entity::pWatcher() ) );
		pWatcher->addChild( "reals", watchREntities );

		// misc
		pWatcher->addChild( "shouldOffload",
				makeWatcher( &Cell::shouldOffload_ ) );

		pWatcher->addChild( "profile",
							CellProfiler::pWatcher(),
							&pNull->profiler_ );

	}

	return pWatcher;
}


// -----------------------------------------------------------------------------
// Section: Communication message handlers concerning load balancing
// -----------------------------------------------------------------------------

/**
 *  This method returns true if this cell is allowed to offload and the app is
 *  permitting offloads.
 */
bool Cell::shouldOffload() const
{
	return shouldOffload_ && CellApp::instance().shouldOffload();
}


/**
 *	This method enables or disables offloading of entities.
 */
void Cell::shouldOffload( bool shouldOffload )
{
	shouldOffload_ = shouldOffload;
}


/**
 *	This method enables or disables offloading of entities.
 */
void Cell::shouldOffload( BinaryIStream & data )
{
	data >> shouldOffload_;
}


/**
 *	This method tells us to remove this cell from the application.
 */
void Cell::retireCell( BinaryIStream & data )
{
	bool isRetiring;

	data >> isRetiring;

	INFO_MSG( "Cell::retireCell: "
				"space = %u. isRetiring = %d. isRemoved = %d\n",
			this->spaceID(), isRetiring, isRemoved_ );

	MF_ASSERT( !isRemoved_ );
	MF_ASSERT( isRetiring == !isRetiring_ );

	isRetiring_ = isRetiring;
}


/**
 *	This method is called by the CellAppMgr to inform us that this cell should
 *	be removed. It also sends a list of CellApp addresses. This cell should not
 *	be deleted until all these CellApps have confirmed that no more entities
 *	are on their way.
 */
void Cell::removeCell( BinaryIStream & data )
{
	INFO_MSG( "Cell::removeCell(%u)\n", this->spaceID() );

	MF_ASSERT( !isRemoved_ );

	isRemoved_ = true;

	while (data.remainingLength())
	{
		Mercury::Address addr;
		data >> addr;

		RemovalAcks::iterator iAck = receivedAcks_.find( addr );

		if (iAck != receivedAcks_.end())
		{
			receivedAcks_.erase( iAck );
		}
		else
		{
			pendingAcks_.insert( addr );
		}
	}
}


/**
 *	This method handles a message from the CellAppMgr telling us that a
 *	neighbouring cell has been removed. This CellApp send a message to the cell
 *	being removed to inform it that no more entities will be created on it from
 *	this app and it is safe to remove.
 */
void Cell::notifyOfCellRemoval( BinaryIStream & data )
{
	Mercury::Address removedAddress;
	data >> removedAddress;
	CellAppChannel & channel =
		*CellAppChannels::instance().get( removedAddress );
	Mercury::Bundle & bundle = channel.bundle();
	bundle.startMessage( CellAppInterface::ackCellRemoval );
	bundle << this->spaceID();

	this->space().setPendingCellDelete( removedAddress );

	channel.send();
}


/**
 *	This method is call by neighbouring CellApps to inform us that it will no
 *	longer create any entities in this cell and so it is safe to remove.
 */
void Cell::ackCellRemoval( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	RemovalAcks::iterator iAck = pendingAcks_.find( srcAddr );

	if (iAck != pendingAcks_.end())
	{
		pendingAcks_.erase( iAck );

		INFO_MSG( "Cell::ackCellRemoval(%u): Got removal ack from %s,"
				"have %zu left, with %zu total pending\n",
			this->spaceID(), srcAddr.c_str(),
			pendingAcks_.count( srcAddr ), pendingAcks_.size() );
	}
	else
	{
		receivedAcks_.insert( srcAddr );

		WARNING_MSG( "Cell::ackCellRemoval(%u): "
				"%s not in pendingAcks_, have now received %zu acks\n",
			this->spaceID(),
			srcAddr.c_str(),
			receivedAcks_.count( srcAddr ) );
	}
}


/**
 *	This method causes this cell to come back from the dead.
 */
bool Cell::reuse()
{
	WARNING_MSG( "Cell::reuse: isRetiring_ = %d. isRemoved_ = %d\n",
			isRetiring_, isRemoved_ );

	bool isOkay = isRetiring_ && isRemoved_;

	isRetiring_ = isRemoved_ = false;

	return isOkay;
}


/**
 *	This method is used to notify this cell that a cell application has died
 *	unexpectedly.
 */
void Cell::handleCellAppDeath( const Mercury::Address & addr )
{
	// TODO:Balance Remove CellInfo from space! (probably)

	pendingAcks_.erase( addr );
	receivedAcks_.erase( addr );
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous
// -----------------------------------------------------------------------------

/**
 *	This method sends this entity positions to the cell app manager.
 */
void Cell::sendEntityPositions( Mercury::Bundle & bundle ) const
{
	if (realEntities_.empty())
	{
		return;
	}

	float xMin = realEntities_.front()->position().x;
	float zMin = realEntities_.front()->position().z;
	float xMax = xMin + 1.f; // Add a bit so that width is non-zero.
	float zMax = zMin + 1.f;

	Entities::const_iterator iter = realEntities_.begin();

	while (iter != realEntities_.end())
	{
		const Position3D & pos = (*iter)->position();

		xMin = std::min( xMin, pos.x );
		xMax = std::max( xMax, pos.x );

		zMin = std::min( zMin, pos.z );
		zMax = std::max( zMax, pos.z );

		iter++;
	}

	bundle << xMin << xMax << zMin << zMax;

	const float xRange = xMax - xMin;
	const float zRange = zMax - zMin;

	iter = realEntities_.begin();

	while (iter != realEntities_.end())
	{
		const Position3D & pos = (*iter)->position();
		uint8 x = uint8((pos.x - xMin) / xRange * 255.f);
		uint8 z = uint8((pos.z - zMin) / zRange * 255.f);

		bundle << x << z;

		iter++;
	}
}


/**
 *	This method returns the id of this cell. This id is unique between cells.
 */
SpaceID Cell::spaceID() const
{
	return this->space().id();
}


/**
 *	This method is called when this space wants to be destroyed.
 */
void Cell::onSpaceGone()
{
	BW::vector< EntityPtr > entities( realEntities_.size() );
	std::copy( realEntities_.begin(), realEntities_.end(), entities.begin() );

	BW::vector< EntityPtr >::iterator iter = entities.begin();

	while (iter != entities.end())
	{
		EntityPtr pEntity = *iter;

		if (!pEntity->isDestroyed())
		{
			Entity::nominateRealEntity( *pEntity );

			PyObject * pMethod =
				PyObject_GetAttrString( pEntity.get(), "onSpaceGone" );
			Script::call( pMethod, PyTuple_New( 0 ),
					"onSpaceGone", true/*okIfFnNull*/ );

			if (!pEntity->isDestroyed() &&
					pEntity->isReal() &&
					&pEntity->space() == &this->space())
			{
				pEntity->destroy();
			}

			Entity::nominateRealEntityPop();
		}

		++iter;
	}
}


// -----------------------------------------------------------------------------
// Section: Debugging
// -----------------------------------------------------------------------------

void Cell::debugDump()
{
	DEBUG_MSG( "Cell - spaceID = %u\n", this->spaceID() );
	DEBUG_MSG( "Reals (%" PRIzu "):\n", realEntities_.size() );

	Entities::iterator iter = realEntities_.begin();
	while (iter != realEntities_.end())
	{
		(*iter)->debugDump();

		iter++;
	}

	space_.debugRangeList();
}


// -----------------------------------------------------------------------------
// Section: Cell::Entities
// -----------------------------------------------------------------------------

/**
 *	This method adds the input entity to this collection.
 */
bool Cell::Entities::add( Entity * pEntity )
{
	MF_ASSERT( pEntity->pReal() != NULL );
	MF_ASSERT( pEntity->pReal()->removalHandle() == NO_ENTITY_REMOVAL_HANDLE );

	if (!pEntity->pReal() &&
		pEntity->pReal()->removalHandle() != NO_ENTITY_REMOVAL_HANDLE)
	{
		ERROR_MSG( "Cell::Entities::add: Failed to add %u\n", pEntity->id() );

		return false;
	}

	pEntity->pReal()->removalHandle( this->size() );
	collection_.push_back( pEntity );

	// Choose random existing entity to swap with to randomise the list.
	// Processes iterating through this collection (such as entity backup)
	// will be effectively randomised.
	EntityPtr pSwapEntity = collection_[random() % collection_.size()];
	this->swapWithBack( pSwapEntity.get() );

	return true;
}


/**
 *	This method swaps the current entity (assumed to be in the collection) with
 *	the entity at the back of the vector.
 */
void Cell::Entities::swapWithBack( Entity * pEntity )
{
	SpaceRemovalHandle swapHandle = pEntity->pReal()->removalHandle();
	EntityPtr pBackEntity = collection_.back();

	collection_[collection_.size() - 1] = pEntity;
	pEntity->pReal()->removalHandle( collection_.size() - 1 );

	collection_[swapHandle] = pBackEntity;
	pBackEntity->pReal()->removalHandle( swapHandle );
}


/**
 *	This method removes the input entity from this collection.
 */
bool Cell::Entities::remove( Entity * pEntity )
{
	if (!pEntity->isReal())
	{
		CRITICAL_MSG( "Cell::Entities::remove: Entity %u is not real\n",
			pEntity->id() );
		return false;
	}

	EntityRemovalHandle handle = pEntity->pReal()->removalHandle();

	if (handle >= collection_.size())
	{
		// This can actually happen if you call Entity.destroy in __init__.
		return false;
	}

	MF_ASSERT( collection_[ handle ] == pEntity );

	if (handle >= collection_.size() ||
		collection_[ handle ] != pEntity )
	{
		WARNING_MSG( "Cell::Entities::remove: Failed to remove %u\n",
						pEntity->id() );
		return false;
	}

	this->swapWithBack( pEntity );
	collection_.pop_back();

	pEntity->pReal()->removalHandle( NO_ENTITY_REMOVAL_HANDLE );

	return true;
}


/**
 *	This method streams on various boundaries to inform the CellAppMgr for the
 *	purposes of load balancing.
 */
void Cell::writeBounds( BinaryOStream & stream ) const
{
	if (!this->isRemoved())
	{
		this->space().writeBounds( stream );
	}
}


/**
 *	This method starts the collection of replay data for this space.
 *
 *	@param entryID 				The space data entry ID used to register the
 *								recording space data key.
 *	@param name 				The name to give to the recorder to use to
 *								identify this replay data.
 *	@param shouldRecordAoIEvents 
								If true, then AoI events will be recorded, otherwise they will not be.
 *	@param isInitial 			If true, indicates that this is the initial
 *								script call from the originating CellApp.
 *								Otherwise, this is being called in response
 *								from space data change.
 *
 *	@return 					true on success, false otherwise.
 */
bool Cell::startRecording( const SpaceEntryID & entryID, 
		const BW::string & name, bool shouldRecordAoIEvents, bool isInitial )
{
	if (pReplayData_ != NULL)
	{
		WARNING_MSG( "Cell::startRecording: "
				"space %d, already recording with name \"%s\", "
				"got request to start for \"%s\"\n",
			this->spaceID(), pReplayData_->name().c_str(), name.c_str() );

		return false;
	}

	pReplayData_ = new ReplayDataCollector( name, shouldRecordAoIEvents );
	pReplayData_->recordingSpaceEntryID( entryID );

	if (isInitial)
	{
		// Initial space data.
		for (SpaceDataMapping::const_iterator it =
					space_.spaceDataMapping().begin();
				it != space_.spaceDataMapping().end();
				it++)
		{
			const SpaceDataMapping::DataValue & value = it->second;
			if (value.key() >= SPACE_DATA_FIRST_CELL_ONLY_KEY)
			{
				continue;	// but don't record cell-only data
			}

			pReplayData_->addSpaceData( it->first, value.key(), value.data() );
		}
	}

	// Entity initial states.
	Entities::const_iterator ipEntity = realEntities_.begin();
	while (ipEntity != realEntities_.end())
	{
		const Entity & entity = **ipEntity;

		pReplayData_->addEntityState( entity );

		++ipEntity;
	}

	if (isInitial)
	{
		ScriptEvents & scriptEvents = CellApp::instance().scriptEvents();
		scriptEvents.triggerEvent( "onRecordingStarted",
			Py_BuildValue( "(is#)",
				this->spaceID(),
				pReplayData_->name().data(), pReplayData_->name().size() ) );

		MemoryOStream spaceDataStream;
		spaceDataStream << name << uint8( shouldRecordAoIEvents );
		BW::string spaceDataString( 
			(char *)(spaceDataStream.retrieve( spaceDataStream.size() )),
			spaceDataStream.size() );

		space_.spaceDataEntry( entryID, SPACE_DATA_RECORDING, 
			spaceDataString, Space::UPDATE_CELL_APP_MGR,
			Space::ALREADY_EFFECTED );
	}

	return true;
}


/**
 *	This method collects this tick's worth of replay data.
 */
void Cell::tickRecording()
{
	if (pReplayData_ == NULL)
	{
		return;
	}

	pReplayData_->addVolatileDataForQueuedEntities();

	GameTime gameTime = CellApp::instance().time();

	ScriptEvents & scriptEvents = CellApp::instance().scriptEvents();

	BinaryIStream & data = pReplayData_->data();
	int size = data.remainingLength();

	scriptEvents.triggerEvent( "onRecordingTickData", 
		Py_BuildValue( "(iIs#ls#)", 
			this->spaceID(),
			gameTime,
			pReplayData_->name().data(), pReplayData_->name().size(),
			this->space().numCells(),
			reinterpret_cast< const char * >( data.retrieve( size ) ), size ) );

	pReplayData_->clear();

	if (stoppingState_ != STOPPING_STATE_NOT_STOPPING)
	{
		if (stoppingState_ == STOPPING_STATE_STOPPING_SHOULD_USE_CALLBACKS)
		{
			BW::string emptyString;
			space_.spaceDataEntry( pReplayData_->recordingSpaceEntryID(), 
				uint16(-1), emptyString, Space::UPDATE_CELL_APP_MGR,
				Space::ALREADY_EFFECTED );

			ScriptEvents & scriptEvents = CellApp::instance().scriptEvents();

			scriptEvents.triggerEvent( "onRecordingStopped", 
				Py_BuildValue( "(is#)", 
					this->spaceID(),
					pReplayData_->name().data(), 
					pReplayData_->name().size() ) );
		}

		stoppingState_ = STOPPING_STATE_NOT_STOPPING;

		bw_safe_delete( pReplayData_ );
	}
}


/**
 *	This method stops the collection of replay data for this space.
 */
void Cell::stopRecording( bool isInitial )
{
	if (pReplayData_ == NULL)
	{
		return;
	}

	pReplayData_->addFinish();
	stoppingState_ = isInitial ? 
		STOPPING_STATE_STOPPING_SHOULD_USE_CALLBACKS :
		STOPPING_STATE_STOPPING_SHOULD_NOT_USE_CALLBACKS;
}

/**
 * This method ticks profilers on the real entities on a Cell instance.
 */
void Cell::tickProfilers( uint64 tickDtInStamps, float smoothingFactor )
{
	Cell::Entities::iterator iEntity = realEntities_.begin();
	while (iEntity != realEntities_.end())
	{
		EntityProfiler & profiler = (*iEntity)->profiler();
		EntityTypeProfiler & typeProfiler = (*iEntity)->pType()->profiler();

		profiler.tick( tickDtInStamps, smoothingFactor, typeProfiler );

		profiler_.addEntityLoad( profiler.load(), profiler.rawLoad() );

		++iEntity;
	}

	profiler_.tick();
}

BW_END_NAMESPACE

// cell.cpp
