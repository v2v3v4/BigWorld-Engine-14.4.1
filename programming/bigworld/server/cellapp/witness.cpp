#include "script/first_include.hpp"

#include "witness.hpp"

#include "cell.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "entity_range_list_node.hpp"
#include "mobile_range_list_node.hpp"
#include "noise_config.hpp"
#include "range_trigger.hpp"
#include "replay_data_collector.hpp"
#include "space.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "common/space_data_types.hpp"

#include "chunk/chunk_space.hpp"

#include "network/bundle.hpp"
#include "network/compression_stream.hpp"

#include "pyscript/script_math.hpp"
#include "server/stream_helper.hpp"
#include "server/util.hpp"

#include "cstdmf/log_msg.hpp"
#include "cstdmf/memory_stream.hpp"


DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

namespace
{
uint32 g_numWitnesses;
uint32 g_numWitnessesEver;

uint32 g_downstreamBundles;
uint32 g_downstreamBytes;
uint32 g_downstreamPackets;

/*
 *	This class is used with make_heap, push_heap and pop_heap to maintain the
 *	priority queue of EntityCaches.
 *
 *	isPrioritised() is considered numerically lower than anything else.
 */
class PriorityCompare
{
public:
	bool operator()( const EntityCache * l, const EntityCache * r ) const
	{
		return l->priority() > r->priority();
	}
};

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: AoITrigger
// -----------------------------------------------------------------------------

/**
 *	This class is used for AoI triggers. It is similar to the Range trigger
 *	except that it will add the other entity to its owner entity's AoI list.
 */
class AoITrigger : public RangeTrigger
{
public:
	AoITrigger( Witness & owner, RangeListNode * pCentralNode, float range ) :
		RangeTrigger( pCentralNode, range,
				RangeListNode::FLAG_LOWER_AOI_TRIGGER,
				RangeListNode::FLAG_UPPER_AOI_TRIGGER,
				RangeListNode::FLAG_NO_TRIGGERS,
				RangeListNode::FLAG_NO_TRIGGERS ),
		owner_( owner )
	{
		// Collect the large entities whose range we currently sit.
		owner_.entity().space().visitLargeEntities(
			pCentralNode->x(),
			pCentralNode->z(),
			*this );

		this->insert();
	}

	~AoITrigger()
	{
		this->removeWithoutContracting();
	}

	virtual BW::string debugString() const;

	virtual void triggerEnter( Entity & entity );
	virtual void triggerLeave( Entity & entity );
	virtual Entity * pEntity() const { return &owner_.entity(); }

private:
	Witness & owner_;
};


// -----------------------------------------------------------------------------
// Section: Witness
// -----------------------------------------------------------------------------

typedef std::pair< SpaceEntryID, uint16 > RecentEntry;
typedef BW::set< RecentEntry > RecentEntriesSet;


/**
 *	The constructor for Witness.
 *
 *	@param owner			The RealEntity associated with this object.
 *	@param data				The data to use to create this object.
 *	@param createRealInfo	Indicates where the data came from.
 */
Witness::Witness( RealEntity & owner, BinaryIStream & data,
		CreateRealInfo createRealInfo, bool hasChangedSpace ) :
	real_( owner ),
	entity_( owner.entity() ),
	noiseCheckTime_     ( CellApp::instance().time() ),
	noisePropagatedTime_( CellApp::instance().time() ),
	noiseMade_( false ),
	maxPacketSize_( 0 ),
	entityQueue_(),
	aoiMap_(),
	shouldAddReplayAoIUpdates_( false ),
	stealthFactor_( 0.f ),
	aoiHyst_( 5.0 ),
	aoiRadius_( CellAppConfig::defaultAoIRadius() ),
	pAoIRoot_( entity_.pRangeListNode() ),
	bandwidthDeficit_( 0 ),
	numFreeAliases_( 0 ),
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	referencePosition_( Vector3::zero() ),
	referenceSeqNum_( 0 ),
	hasReferencePosition_( false ),
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
	knownSpaceDataSeq_( -1 ),
	allSpacesDataChangeSeq_( Space::s_allSpacesDataChangeSeq_ - 1 ),
	pAoITrigger_( NULL ),
	lastSentReliableGameTime_( 0 ),
	lastSentReliablePosition_(),
	lastSentReliableDirection_()
{
	++g_numWitnesses;
	++g_numWitnessesEver;

	// In the freeAliases_ array, we want the first numFreeAliases_ to contain
	// the free aliases. To do this, we first fill the array with 1s. We then go
	// through and set the elements whose index is used to 0. We then traverse
	// this array. For each 1 we find, we know that we add that index to the set
	// and increase the number found by 1.
	memset( freeAliases_, 1, sizeof( freeAliases_ ) );

	this->readOffloadData( data, createRealInfo, hasChangedSpace );

	// Finish setting up freeAliases_. Currently, the array has 1s corresponding
	// to free ids and 0s corresponding to used. We want the set of free numbers
	// to be at the start of the array.

	// Make sure that this NO_ID_ALIAS is reserved, and not actually used as an
	// ID.
	freeAliases_[ NO_ID_ALIAS ] = 0;

	for (uint i = 0; i < sizeof( freeAliases_ ); i++)
	{
		// If temp is 0, next space is written but numFreeAliases_ is not
		// incremented. Doing it this way avoids any if statements.
		int temp = freeAliases_[i];
		freeAliases_[ numFreeAliases_ ] = i;
		numFreeAliases_ += temp;
	}

	CellApp::instance().registerWitness( this );

	// Tell the proxy what is the tickSync for the first message.
	BaseAppIntInterface::tickSyncArgs::
		start( this->bundle() ).tickByte = 
				(uint8)(CellApp::instance().time() +
				(CellApp::instance().hasCalledWitnesses() ? 1 : 0) );

	// call our own init method
	this->init();

	// tell the client about us if the witness (us) has just been enabled
	if (createRealInfo == CREATE_REAL_FROM_INIT)
	{
		// send a special createPlayer message to the client
		// TODO: really consider merging this with createEntity msg...
		// ends up the same on the client anyway
		Mercury::Bundle & bundle = this->bundle();
		bundle.startMessage( BaseAppIntInterface::createCellPlayer );

		// Hopefully this is not too early to look at space()...

		bundle << entity_.space().id() << entity_.vehicleID();
		bundle << entity_.position();
		bundle << CellAppConfig::packedXZScale();
		bundle << entity_.direction();

		const EntityDescription & desc = entity_.pType()->description();

		
		// Populate local and delegate properties to dict and ...
		ScriptDict tempDict = entity_.createDictWithProperties( 
				EntityDescription::FROM_CELL_TO_CLIENT_DATA );

		if (!tempDict)
		{
			CRITICAL_MSG( "Witness::Witness(%d): "
					"Failed to populate entity attributes to dict\n",
				entity_.id() );
		}
		
		// ... write dict to stream
		if (!desc.addDictionaryToStream( tempDict, bundle,
				EntityDescription::FROM_CELL_TO_CLIENT_DATA ))
		{
			CRITICAL_MSG( "Witness::Witness(%d): "
					"Failed to write entity attributes dict to stream\n",
				entity_.id() );
		}
	}

	shouldAddReplayAoIUpdates_ = true;
}


/**
 *	This method write the space data information to a stream for offloading.
 *
 *	@param data The stream to write the space data to.
 */
void Witness::writeSpaceDataEntries( BinaryOStream & data ) const
{
	const Space & space = entity_.space();
	if (knownSpaceDataSeq_ < space.begDataSeq() ||
		knownSpaceDataSeq_ > space.endDataSeq() )
	{
		// We don't know anything about this space
		data << uint32( 0 );
		return;
	}

	// Find the sequence number of the oldest entry that is recent
	// enough to be interesting
	int32 oldestRecentSeq;
	for ( oldestRecentSeq = knownSpaceDataSeq_ - 1;
		oldestRecentSeq >= space.begDataSeq();
		oldestRecentSeq-- )
	{
		// Note that we stream dataRecencyLevel < 2 but the onload
		// will only look at entries with dataRecencyLevel < 1
		// If this is the most recent entry, stream it anyway.
		if (oldestRecentSeq != knownSpaceDataSeq_ - 1 &&
			space.dataRecencyLevel( oldestRecentSeq ) > 1)
		{
			break;
		}
	}
	oldestRecentSeq++;

	data << uint32( knownSpaceDataSeq_ - oldestRecentSeq );
	for ( int32 seq = oldestRecentSeq; seq < knownSpaceDataSeq_; seq++ )
	{
		SpaceEntryID entryID;
		uint16 key;
		space.dataBySeq( seq, entryID, key );
		data << entryID << key;
	}
}


/**
 *	This method reads the space data entries when being onloaded to a new Cell.
 *
 *	@param data The input stream to read the space data from.
 *	@param hasChangedSpace Boolean indicating whether the witness has changed
 *	       space since being offloaded.
 */
void Witness::readSpaceDataEntries( BinaryIStream & data, bool hasChangedSpace )
{
	// NOTE: This needs to match StreamHelper::addRealEntityWithWitnesses
	// in bigworld/src/lib/server/stream_helper.hpp

	uint32 numSpaceDataEntries = 0;
	data >> numSpaceDataEntries;

	if (numSpaceDataEntries == 0)
	{
		return;
	}

	if (hasChangedSpace)
	{
		// We don't care about this data if we've changed spaces
		data.retrieve( numSpaceDataEntries *
				 ( sizeof( RecentEntry::first_type ) +
					sizeof( RecentEntry::second_type ) ) );
		return;
	}

	const Space & space = entity_.space();
	RecentEntriesSet recentEntries;
	for ( uint32 i = 0; i < numSpaceDataEntries; i++)
	{
		RecentEntry entrySpec;
		data >> entrySpec.first >> entrySpec.second;
		recentEntries.insert( entrySpec );
	}

	int32 seq;
	// Now find the most recent DataSeq for this space that we have
	// contigious entries for since begDataSeq.
	for ( seq = space.begDataSeq(); seq < space.endDataSeq(); seq++ )
	{
		// Note that we only check dataRecencyLevel < 1 but the
		// stream actually contains dataRecencyLevel < 2
		if (space.dataRecencyLevel( seq ) != 0)
		{
			// An offload really shouldn't leave us that far behind,
			// so we will assume the client already knows about this
			// entry.
			continue;
		}

		RecentEntry entrySpec;
		space.dataBySeq( seq, entrySpec.first, entrySpec.second );

		RecentEntriesSet::const_iterator found =
			recentEntries.find( entrySpec );
		if (found != recentEntries.end())
		{
			// We'd already seen this one.
			continue;
		}
		// OK, found the first new sequence entry.
		// We can't do holes in spaceData sequence, so even if we'd
		// somehow gotten later data to the client, we will just
		// resend it next Witness::update.
		break;
	}

	// Did we match anything at all?
	if (seq > space.begDataSeq())
	{
		knownSpaceDataSeq_ = seq;
	}
}


/**
 *	This method writes the AoI from the Witness to a stream to be used in
 *	onloading.
 *
 *	@param data The output stream to write the witness' AoI to.
 */
void Witness::writeAoI( BinaryOStream & data ) const
{
	// NOTE: This needs to match StreamHelper::addRealEntityWithWitnesses
	// in bigworld/src/lib/server/stream_helper.hpp
	// and Witness::readAoI

	data << (uint32)aoiMap_.size();
	data << aoiRadius_ << aoiHyst_;

	data << this->isAoIRooted();

	if (this->isAoIRooted())
	{
		data << this->pAoIRoot()->x() << this->pAoIRoot()->z();
	}

	// write the AoI
	aoiMap_.writeToStream( data );
}


/**
 *	This method reads the AoI from the stream to initialise the Witness.
 *
 *	@param data	The input stream containing the AoI initialise from.
 */
void Witness::readAoI( BinaryIStream & data )
{
	// NOTE: This needs to match StreamHelper::addRealEntityWithWitnesses
	// in bigworld/src/lib/server/stream_helper.hpp
	// and Witness::writeAoI

	uint32 entityQueueSize = 0;
	data >> entityQueueSize;

	data >> aoiRadius_ >> aoiHyst_;

	bool isAoIRooted;
	data >> isAoIRooted;

	MF_ASSERT( !this->isAoIRooted() );

	if (isAoIRooted)
	{
		float rootX, rootZ;
		data >> rootX >> rootZ;
		pAoIRoot_ = new MobileRangeListNode( rootX, rootZ,
			RangeListNode::FLAG_NO_TRIGGERS,
			RangeListNode::FLAG_NO_TRIGGERS );
		// TODO: Remove this const_cast.
		const_cast< RangeList & >( entity().space().rangeList() ).add(
			pAoIRoot_ );
	}

	MF_ASSERT( isAoIRooted == this->isAoIRooted() );

	// Read in the Area of Interest entities. We need to be careful that the
	// new AoI that is constructed matches what the client thinks the AoI is.
	MF_ASSERT( entityQueueSize < 1000000 );

	if (entityQueueSize == 0)
	{
		return;
	}

	// Number of entities that do not even exist on this CellApp.
	int numLostEntities = 0;

	entityQueue_.reserve( entityQueueSize );

	for (size_t i = 0; i < entityQueueSize; i++)
	{
		EntityID id;
		data >> id;

		Entity * pEntity = CellApp::instance().findEntity( id );

		if (pEntity == NULL)
		{
			// We are not concerned about the alias due to the leave being sent
			// before the client is notified of enters
			EntityCache dummyCache( NULL );
			data >> dummyCache;

			EntityID expectedVehicleID = 0;
			if (dummyCache.vehicleChangeNum() ==
				EntityCache::VEHICLE_CHANGE_NUM_HAS_VEHICLE)
			{
				// This is written in EntityCache's streaming operator
				data >> expectedVehicleID;
			}

			if (!dummyCache.isEnterPending())
			{
				this->entity().callback( "onLeftAoIID",
					Py_BuildValue( "(i)", id ),
					"onLeftAoIID", true );
				Mercury::Bundle & bundle = this->bundle();
				dummyCache.addLeaveAoIMessage( bundle, id );
			}

			++numLostEntities;
			continue;
		}

		EntityCache * pCache = aoiMap_.add( *pEntity );

		data >> *pCache;

		EntityID expectedVehicleID = 0;
		if (pCache->vehicleChangeNum() ==
			EntityCache::VEHICLE_CHANGE_NUM_HAS_VEHICLE)
		{
			// This is written in EntityCache's streaming operator
			data >> expectedVehicleID;
		}
		entityQueue_.push_back( pCache );
		// we add pending entities to the queue too (init removes them)

		MF_ASSERT( pCache->idAlias() == NO_ID_ALIAS ||
				freeAliases_[ pCache->idAlias() ] == 1 );
		freeAliases_[ pCache->idAlias() ] = 0;

		MF_ASSERT( !pEntity->isInAoIOffload() );
		pEntity->isInAoIOffload( true );

		pCache->setGone();
		// our triggers will bring it back if it's not really gone

		if ((expectedVehicleID == pEntity->vehicleID()) &&
			(pCache->vehicleChangeNum() !=
					EntityCache::VEHICLE_CHANGE_NUM_OLD))
		{
			pCache->vehicleChangeNum( pEntity->vehicleChangeNum() );
		}
		else
		{
			pCache->vehicleChangeNum( pEntity->vehicleChangeNum() - 1 );
		}

		if (pCache->isManuallyAdded())
		{
			this->addToAoI( pEntity, /* setManuallyAdded */ true );
		}
	}

	std::make_heap( entityQueue_.begin(), entityQueue_.end(), PriorityCompare() );

	if (numLostEntities > 0)
	{
		INFO_MSG( "Witness::readAoI: Lost %d of %u entities from AoI\n",
				numLostEntities, entityQueueSize );
	}
}


/**
 *	This method initialises this object some more. It is only separate from
 *	the constructor for historical reasons.
 */
void Witness::init()
{
	// Disabling callbacks is not needed since no script should be triggered but
	// it's helpful for debugging.
	Entity::callbacksPermitted( false );

	// Create AoI triggers around ourself.
	{
		SCOPED_PROFILE( SHUFFLE_AOI_TRIGGERS_PROFILE );
		pAoITrigger_ = new AoITrigger( *this, pAoIRoot_, aoiRadius_ );
		if (this->isAoIRooted())
		{
			MobileRangeListNode * pRoot =
				static_cast< MobileRangeListNode * >( pAoIRoot_ );
			pRoot->addTrigger( pAoITrigger_ );
		}
		else
		{
			entity().addTrigger( pAoITrigger_ );
		}
	}

	Entity::callbacksPermitted( true );

	KnownEntityQueue::size_type i = 0;

	// Sort out entities that didn't make it back into our AoI.
	while (i < entityQueue_.size())
	{
		KnownEntityQueue::iterator iter = entityQueue_.begin() + i;
		EntityCache * pCache = *iter;
		Entity * pEntity = const_cast< Entity * >( pCache->pEntity().get() );

		if (pEntity->isInAoIOffload())
		{
			pEntity->isInAoIOffload( false );

			this->entity().callback( "onLeftAoI",
					PyTuple_Pack( 1, pEntity ),
					"onLeftAoI", true );

			// 'gone' should still be set on it
			MF_ASSERT( pCache->isGone() );
			if (!pCache->isRequestPending())
			{
				++i;
			}
			else
			{
				*iter = entityQueue_.back();
				entityQueue_.pop_back();
			}
		}
		else if (pCache->isRequestPending())
		{
			// these don't go in the queue until they're acknowledged
			*iter = entityQueue_.back();
			entityQueue_.pop_back();
		}
		else
		{
			if (pCache->isManuallyAdded())
			{
				this->addToAoI( pEntity, /* setManuallyAdded */ true );
			}

			// entity could be ENTER_PENDING, CREATE_PENDING, or normal
			++i;
		}
	}

	// And finally make the entity queue into a heap.
	std::make_heap( entityQueue_.begin(), entityQueue_.end(), PriorityCompare() );
}


/**
 *	The destructor for Witness.
 */
Witness::~Witness()
{
	--g_numWitnesses;

	MF_VERIFY( CellApp::instance().deregisterWitness( this ) );

	// Delete any entity caches not in the aoiMap
	KnownEntityQueue::iterator iter = entityQueue_.begin();
	while (iter != entityQueue_.end())
	{
		if (!(*iter)->pEntity()) delete *iter;
		iter++;
	}

	// Disabling callbacks is not needed since no script should be triggered but
	// it's helpful for debugging.
	Entity::callbacksPermitted( false );

	if (this->isAoIRooted())
	{
		MobileRangeListNode * pRoot =
			static_cast< MobileRangeListNode * >( pAoIRoot_ );
		pRoot->delTrigger( pAoITrigger_ );
	}
	else
	{
		this->entity().delTrigger( pAoITrigger_ );
	}

	bw_safe_delete( pAoITrigger_ );

	if (this->isAoIRooted())
	{
		static_cast< MobileRangeListNode * >( pAoIRoot_ )->remove();
		bw_safe_delete( pAoIRoot_ );
	}

	Entity::callbacksPermitted( true );

	// The aoiMap will delete all its nodes itself
}


/**
 *	This method should put the relevant data into the input BinaryOStream so
 *	that this entity can be onloaded to another cell.
 *
 *	@param data		The stream to place the data on.
 */
void Witness::writeOffloadData( BinaryOStream & data ) const
{
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	data << referencePosition_ << referenceSeqNum_ << hasReferencePosition_;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	data << maxPacketSize_;

	data << stealthFactor_;

	this->writeSpaceDataEntries( data );

	this->writeAoI( data );
}


/**
 *	This method reads the relevant data from the stream to restore a Witness
 *	on a new cell.
 *
 *	@param data The input stream to restore from.
 *	@param createRealInfo Enumeration to indicate how the reference position
 *	                      should be initialised.
 *	@param hasChangedSpace Boolean indicating whether the space changed during
 *	                       the offload process.
 */
void Witness::readOffloadData( BinaryIStream & data,
	CreateRealInfo createRealInfo, bool hasChangedSpace )
{
	// NOTE: This needs to match StreamHelper::addRealEntityWithWitnesses
	// in bigworld/src/lib/server/stream_helper.hpp

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	if (createRealInfo == CREATE_REAL_FROM_OFFLOAD)
	{
		data >> referencePosition_ >> referenceSeqNum_ >> hasReferencePosition_;
	}
	else
	{
		// initialise the reference position, client gets sent this position
		// for its initial reference position
		referencePosition_ = ::BW_NAMESPACE calculateReferencePosition( 
			entity_.position() );
	}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	data >> maxPacketSize_;

	data >> stealthFactor_;

	// Stream off the spaceData entries we already knew about and determine what
	// sequence number we're up to for this space.
	this->readSpaceDataEntries( data, hasChangedSpace );

	// Read the Area of Interest settings from the data stream.
	this->readAoI( data );
}



/**
 *	This method adds the backup data associated with this real entity to the
 *	input stream.
 */
void Witness::writeBackupData( BinaryOStream & data ) const
{
	StreamHelper::addRealEntityWithWitnesses( data,
		maxPacketSize_,
		aoiRadius_,
		aoiHyst_,
		this->isAoIRooted(),
		this->pAoIRoot()->x(),
		this->pAoIRoot()->z() );
}


/**
 *	This method lets the client know the server's view of which vehicle it is
 *	on.
 */
void Witness::vehicleChanged()
{
	AUTO_SCOPED_ENTITY_PROFILE( &entity_ );

	BaseAppIntInterface::setVehicleArgs & sva =
		BaseAppIntInterface::setVehicleArgs::start( this->bundle() );
	sva.passengerID = entity_.id();
	sva.vehicleID = entity_.pVehicle() ?
		entity_.pVehicle()->id() : NULL_ENTITY_ID;

	// As we may not have been given control back at this point we should
	// always push the update.
	// PM: I don't really understand why this is necessary. It would be good to
	// remove, if possible.
	this->flushToClient();
}


/**
 *	This method adds any appropriate changes to space data to a bundle.
 */
void Witness::addSpaceDataChanges( Mercury::Bundle & bundle )
{
	// see if any space data has changed
	if (allSpacesDataChangeSeq_ != Space::s_allSpacesDataChangeSeq_)
	{
		allSpacesDataChangeSeq_ = Space::s_allSpacesDataChangeSeq_;

		// see if we want to send any more data
		Space & space = entity_.space();

		if (knownSpaceDataSeq_ < space.begDataSeq())
		{
			// if we're not in the seq we have to send it all
			SpaceDataMapping::const_iterator it;
			SpaceDataMapping::const_iterator endIter =
					space.spaceDataMapping().end();
			for (it = space.spaceDataMapping().begin();
				it != endIter;
				it++)
			{
				if (it->second.key() >= SPACE_DATA_FIRST_CELL_ONLY_KEY)
					continue;	// but don't send cell-only data
				bundle.startMessage( BaseAppIntInterface::spaceData );
				bundle << space.id();
				bundle << it->first << it->second.key();
				bundle.addBlob( it->second.data().data(), it->second.data().length() );
			}
			knownSpaceDataSeq_ = space.endDataSeq();
		}
		while (knownSpaceDataSeq_ < space.endDataSeq())
		{
			// otherwise only send stuff that's new
			bundle.startMessage( BaseAppIntInterface::spaceData );
			bundle << space.id();

			SpaceEntryID seid;
			uint16 key;
			const BW::string * pValue = space.dataBySeq(
				knownSpaceDataSeq_++, seid, key );

			bundle << seid << key;
			if (pValue != NULL) // (key != uint16(-1))
			{	// data might not still be there if added+removed quickly
				bundle.addBlob( pValue->data(), pValue->length() );
			}
		}
	}
}


#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
/**
 *	This method adds the appropriate reference position message to the input
 *	bundle.
 *
 *	@param bundle	The bundle to put the information on.
 *	@return			A pointer to the boolean making the reference position
 *					message reliable, or NULL if no such message was added.
 */
bool * Witness::addReferencePosition( Mercury::Bundle & bundle )
{
	// Fast and efficient path: Client-controlled witness which has sent us a
	// reference position at some point.
	if (real_.controlledByRef().id == entity_.id() && hasReferencePosition_)
	{
		BaseAppIntInterface::makeNextMessageReliableArgs & reliableArgs =
			BaseAppIntInterface::makeNextMessageReliableArgs::start( bundle );
		reliableArgs.isReliable = false;

		// Make sure there is a reference position for this tick
		BaseAppIntInterface::relativePositionReferenceArgs::start(
			bundle ).sequenceNumber = referenceSeqNum_;

		return &(reliableArgs.isReliable);
	}

	// Clear out any now-useless client-supplied reference position
	if (hasReferencePosition_)
	{
		this->cancelReferencePosition();
	}

	MF_ASSERT( !hasReferencePosition_ );

	// The witness is either server-controlled, or newly client-controlled
	// and hasn't sent us a reference position to use. So we choose a position.
	this->calculateReferencePosition();

	if (entity_.pVehicle() != NULL ||
		real_.controlledByRef().id == entity_.id())
	{
		// Worst-case bandwidth: The witness is on a vehicle, or
		// client-controlled, so doesn't have any shared state we can use as a
		// reference position. So we need to send one.

		// This only happens to client-controlled entities which recently
		// gained control and so have not yet sent us a position update.

		BaseAppIntInterface::makeNextMessageReliableArgs & reliableArgs =
			BaseAppIntInterface::makeNextMessageReliableArgs::start( bundle );
		reliableArgs.isReliable = false;

		BaseAppIntInterface::relativePositionArgs::start( bundle ).position =
			referencePosition_;

		return &(reliableArgs.isReliable);
	}

	// If we didn't send BaseAppIntInterface::relativePositionArgs, we're
	// relying on the position sent by addDetailedPlayerPosition (for a volatile
	// entity) or the most recent detailedPosition (for a non-volatileEntity)
	// as the reference position.

	return NULL;
}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */


/**
 *	This method adds a detailed volatile position message for the
 *	entity of this witness.
 *
 *	@param bundle	The bundle to put the information on.
 *	@return			A pointer to the boolean making the detailed volatile
 *					position message reliable, or NULL if no such message
 *					was added.
 */
bool * Witness::addDetailedPlayerPosition( Mercury::Bundle & bundle,
	bool isReferencePosition ) const
{
	// Non-volatile or witness-controlled entity: don't send the position
	if (!entity_.volatileInfo().hasVolatile( 0.f ) ||
		entity_.pReal()->controlledByRef().id == entity_.id())
	{
		return NULL;
	}

	// Check that we're allowed to skip an unchanged player position
	// update.
	const bool hasSentPlayerPosition = lastSentReliableGameTime_ != 0;

	const bool alwaysSkipUnchanged =
		CellAppConfig::maxTimeBetweenPlayerUpdatesInTicks() == -1;

	const GameTime timeSinceLastSend =
		CellApp::instance().time() - lastSentReliableGameTime_;

	// This deliberately skips the message if
	// maxTimeBetweenPlayerUpdatesInTicks() is 0 and timeSinceLastSend is 0,
	// because in that case, a detailedPosition message has been added during
	// this tick, so we don't need to send the player position again.
	const bool lastSendWasRecentEnough = alwaysSkipUnchanged ||
		(timeSinceLastSend <=
			(GameTime)CellAppConfig::maxTimeBetweenPlayerUpdatesInTicks());

	const bool shouldSkipUnchangedPlayerPosition =
		hasSentPlayerPosition && lastSendWasRecentEnough;


	if (!isReferencePosition && shouldSkipUnchangedPlayerPosition &&
		entity_.localPosition() == lastSentReliablePosition_ &&
		entity_.localDirection() == lastSentReliableDirection_)
	{
		return NULL;
	}


	// This message may need to be reliable, if it is being used as the
	// reference position and reliable entity positions were added.
	BaseAppIntInterface::makeNextMessageReliableArgs & reliableArgs =
		BaseAppIntInterface::makeNextMessageReliableArgs::start( bundle );

	// If the maxTimeBetweenPlayerUpdates optimisation is enabled,
	// then it's reliable unless it's a reference position.
	// Reference position may still be made reliable by the normal
	// reference-position reliability logic in Witness::update()
	reliableArgs.isReliable = !isReferencePosition &&
		CellAppConfig::maxTimeBetweenPlayerUpdatesInTicks() != 0;

	// Witness::vehicleChanged will have sent any vehicle change already.
	BaseAppIntInterface::avatarUpdatePlayerDetailedArgs & rAvatarUpdatePlayer =
		BaseAppIntInterface::avatarUpdatePlayerDetailedArgs::start( bundle );

	rAvatarUpdatePlayer.position = entity_.localPosition();
	rAvatarUpdatePlayer.dir = entity_.localDirection();

	return &(reliableArgs.isReliable);
}



/**
 *	This method writes a message to the given bundle indicating which entity
 *	the following history events are destined for.
 *
 *	@param bundle	The bundle to put the information on.
 *	@param cache	The EntityCache for the Entity to select
 *
 *	@return		True unless the Entity was not in AoI or could not receive
 *				history events.
 */
bool Witness::selectEntity( Mercury::Bundle & bundle,
	EntityID targetID ) const
{
	if (targetID == entity_.id())
	{
		bundle.startMessage(
			BaseAppIntInterface::selectPlayerEntity );
	}
	else
	{
		EntityCache * pCache = aoiMap_.find( targetID );

		if (!pCache || !pCache->isUpdatable())
		{
			ERROR_MSG( "Witness::selectEntity: Witness %d trying "
					"to select Entity %d outside its AoI\n",
				entity_.id(), targetID );
			return false;
		}
		pCache->addEntitySelectMessage( bundle );
	}
	return true;
}

/**
 * This method processes an entity cache state change
 *
 * @param ppCache   The current entity cache. Can be NULL after the call if it was invalidated.
 * @param queueEnd Iterator pointing to the last entry in the heap
 */
void Witness::handleStateChange( EntityCache ** ppCache,
				KnownEntityQueue::iterator & queueEnd )
{
	MF_ASSERT( ppCache != NULL );

	Mercury::Bundle & bundle = this->bundle();
	EntityCache * pCache = *ppCache;

	MF_ASSERT( !pCache->isRequestPending() );
	if (pCache->isGone())
	{
		this->deleteFromSeen( bundle, queueEnd );
		*ppCache = NULL;
	}
	else if (pCache->isWithheld())
	{
		// If client knows about a withheld entity, make it "un-know".
		if (!pCache->isEnterPending())
		{
			this->deleteFromClient( bundle, *queueEnd );

			// The WITHHELD flag should be retained in deleteFromClient which
			// resets client related state of pCache.
			MF_ASSERT( pCache->isWithheld() );

			pCache->setEnterPending();
		}
		// Leave withheld entities
	}
	else if (pCache->isEnterPending())
	{
		this->handleEnterPending( bundle, queueEnd );
	}
	else if (pCache->isCreatePending())
	{
		this->sendCreate( bundle, pCache );
		pCache->updatePriority( entity_.position() );
	}
	else if (pCache->isRefresh())
	{
		pCache->clearRefresh();
		// We are effectively resetting this entity cache's
		// history, tell the client that it's gone and (re-)entered.
		this->deleteFromClient( bundle, pCache );

		pCache->setEnterPending();
		this->handleEnterPending( bundle, queueEnd );
	}
}

/**
 * This method writes client update data to bundle
 *
 * @param pCache  The current entity cache 
 * @return		  True if a reliable position update message was included in the
 *				  bundle, false otherwise.
 */
bool Witness::sendQueueElement( EntityCache * pCache )
{
	Mercury::Bundle & bundle = this->bundle();

	const Entity & otherEntity = *pCache->pEntity();
	bool hasAddedReliableRelativePosition = false;

	// Recalculate the priority value of this entity
	float distSqr = pCache->getLoDPriority( entity_.position() );

	// Send data updates to the client for this entity

#define DEBUG_BANDWIDTH
#ifdef DEBUG_BANDWIDTH
	int oldSize = bundle.size();
#endif // DEBUG_BANDWIDTH

#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	otherEntity.writeClientUpdateDataToBundle( bundle, Position3D::zero(),
		*pCache, distSqr );
#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	hasAddedReliableRelativePosition |=
		otherEntity.writeClientUpdateDataToBundle( bundle,
			referencePosition_, *pCache, distSqr );
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */


#ifdef DEBUG_BANDWIDTH
	if (bundle.size() - oldSize > CellAppConfig::entitySpamSize())
	{
		WARNING_MSG( "Witness::update: "
							"%u added %d bytes to bundle of %u\n",
			otherEntity.id(),
			bundle.size() - oldSize,
			entity_.id() );
	}
#endif // DEBUG_BANDWIDTH

	return hasAddedReliableRelativePosition;
}

/**
 *	This method is called regularly to send data to the witnesses associated
 *	with this entity.
 */
void Witness::update()
{
	SCOPED_PROFILE( CLIENT_UPDATE_PROFILE );
	AUTO_SCOPED_ENTITY_PROFILE( &entity_ );

	static ProfileVal profilePrepare( "updateClientPrepare" );
	static ProfileVal profileVehiclePrioritise( "updateClientVehiclePrioritise" );
	static ProfileVal profileLoop( "updateClientLoop" );
	static ProfileVal profileMakeHeap( "updateClientMakeHeap" );
	static ProfileVal profileClearPrioritised( "updateClientClearPrioritised" );
	static ProfileVal profilePush( "updateClientPush" );

	START_PROFILE( profilePrepare );

	Mercury::Bundle & bundle = this->bundle();

	const float throttle = CellApp::instance().emergencyThrottle();
	const int desiredPacketSize = int(maxPacketSize_ * throttle) -
			bandwidthDeficit_ + bundle.size();

	this->addSpaceDataChanges( bundle );
	bool * pIsReferencePositionReliable = NULL;

#if VOLATILE_POSITIONS_ARE_ABSOLUTE

	bool usePlayerAsReference = false;

#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	pIsReferencePositionReliable = this->addReferencePosition( bundle );
	bool usePlayerAsReference = pIsReferencePositionReliable == NULL;

#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	// To avoid juddering of the witness, we now need to send detailed position
	// updates for the witness. We only send if volatile and not controlled
	// by our client.
	bool * pIsPlayerPositionReliable =
		this->addDetailedPlayerPosition( bundle, usePlayerAsReference );

	// Get pIsReferencePositionReliable pointing at the isReliable member
	// of whatever message we're using to carry the reference position for
	// the rest of our position updates.
	if (pIsReferencePositionReliable == NULL)
	{
		pIsReferencePositionReliable = pIsPlayerPositionReliable;
	}
	else if (pIsPlayerPositionReliable != NULL)
	{
		// There's a real reference position message, so the player position
		// message is not being used as a reference position

		// This is the condition that we won't use a player's position message
		// as a reference position. See
		// ServerConnection::detailedPositionReceived and its callers in
		// lib/connection/server_connection.cpp
		MF_ASSERT( entity_.pVehicle() != NULL ||
			real_.controlledByRef().id == entity_.id() );
	}

	bool hasAddedReliableRelativePosition = false;

	STOP_PROFILE( profilePrepare );

	int numPrioritised = 0;

	START_PROFILE( profileVehiclePrioritise );
	EntityCache::Priority startingPriority = entityQueue_.empty() ? 0.f :
		entityQueue_.front()->priority();

	// send the vehicle stack first
	for (Entity * pVehicle = entity_.pVehicle(); pVehicle != NULL;
		pVehicle = pVehicle->pVehicle() )
	{
		EntityCache * pCache = aoiMap_.find( pVehicle->id() );
		if (!pCache)
		{
			ERROR_MSG( "Witness::update(): Witness %d trying "
					"to prioritise Entity %d outside its AoI\n",
				entity_.id(), pVehicle->id() );
			continue;
		}

		if (!pCache->isUpdatable() || pCache->isRequestPending() ||
				pCache->pEntity()->isDestroyed())
		{
			continue;
		}

		MF_ASSERT( !pCache->isPrioritised() );

		++numPrioritised;
		pCache->setPrioritised();

		hasAddedReliableRelativePosition |= this->sendQueueElement( pCache );
	}

	STOP_PROFILE( profileVehiclePrioritise );

	START_PROFILE( profileLoop );

	int loopCounter = 0;

	// This is the maximum amount of priority change that we go through in a
	// tick. Based on the default AoIUpdateScheme (distance/5 + 1) things up
	// to 45 metres away can be sent every frame.
	const float MAX_PRIORITY_DELTA =
		CellAppConfig::witnessUpdateMaxPriorityDelta();

	// We want to make sure that entities at a distance are never sent at 10Hz.
	// What we do is make sure that the change in priorities that we go over is
	// capped.
	// Note: We calculate the max priority from the front priority. We should
	// probably calculate from the previous maxPriority. Doing it the current
	// way, if you only have 1 entity in your AoI, it will be sent every frame.
	EntityCache::Priority maxPriority = entityQueue_.empty() ? 0.f :
		entityQueue_.front()->priority() + MAX_PRIORITY_DELTA;

	KnownEntityQueue::iterator queueBegin = entityQueue_.begin();
	KnownEntityQueue::iterator queueEnd = entityQueue_.end();

			// Entities in queue &&
	while ((queueBegin != queueEnd) &&
				// Priority change less than MAX_PRIORITY_DELTA &&
				(*queueBegin)->priority() < maxPriority &&
				// Packet still has space (includes 2 bytes for sequenceNumber))
				bundle.size() < desiredPacketSize - 2)
	{
		loopCounter++;

		// Pop the top entity. pop_heap actually keeps the entity in the vector
		// but puts it in the end. [queueBegin, queueEnd) is a heap and
		// [queueEnd, entityQueue_.end()) has the entities that have been popped
		EntityCache * pCache = entityQueue_.front();
		std::pop_heap( queueBegin, queueEnd--, PriorityCompare() );
		bool wasPrioritised = pCache->isPrioritised();
		
		// See if the entity is still in our AoI

		MF_ASSERT(!pCache->isRequestPending());

		if (pCache->pEntity()->isDestroyed() && !pCache->isGone())
		{
			MF_ASSERT(pCache->isManuallyAdded());
			this->removeFromAoI( const_cast<Entity *>( pCache->pEntity().get() ),
				 /* clearManuallyAdded */ true );
		}

		if (!pCache->isUpdatable())
		{
			this->handleStateChange( &pCache, queueEnd );
		}
		else if (!pCache->isPrioritised())
		{
			// The entity has not gone anywhere, so we will proceed with the update.
			hasAddedReliableRelativePosition |= this->sendQueueElement( pCache );

			pCache->updatePriority( entity_.position() );
		}

		// Prioritised, updatable entities were sent earlier.
		if (wasPrioritised)
		{
			if (pCache)
			{
				pCache->priority( startingPriority );
				// prioritised vehicles' priorities should be updated only after pop_heap
				pCache->updatePriority( entity_.position() );

				pCache->clearPrioritised();
			}
			--numPrioritised;
			MF_ASSERT( numPrioritised >= 0 );
		}
	}

	STOP_PROFILE_WITH_DATA( profileLoop, loopCounter );

	bandwidthDeficit_ =
		(bundle.size() > desiredPacketSize) ?
			bundle.size() - desiredPacketSize : 0;

	if (bandwidthDeficit_ > maxPacketSize_)
	{
		WARNING_MSG( "Witness::update: "
				"%u has a deficit of %d bytes (%.2f packets)\n",
			entity_.id(),
			bandwidthDeficit_,
			float(bandwidthDeficit_)/maxPacketSize_ );

		// TODO: We probably do not want this eventually.
		// Cap the maximum bandwidth deficit.
		bandwidthDeficit_ = maxPacketSize_;
	}

	// We need to push all of the sent entities back on the heap.

	if (queueEnd == entityQueue_.begin())
	{
		// Must have sent and popped all prioritised entities.
		MF_ASSERT( numPrioritised == 0 );

		START_PROFILE( profileMakeHeap );
		while (queueEnd != entityQueue_.end())
		{
			// TODO: Ask Murph about this... Shouldn't this then updatePriority
			// according to current distance?
			// As it is, if we empty the queue, next tick will send entities in
			// random order. We know we've just finished sending _everything_,
			// so we should send them again in order of closeness.
			(*queueEnd)->priority( 0.f );
			// i.e. now,
			// (*queueEnd)->updatePriority( entity_.position() );
			// TODO: Blame this file, perhaps.

			++queueEnd;
		}

		std::make_heap( entityQueue_.begin(), entityQueue_.end(), PriorityCompare() );
		STOP_PROFILE_WITH_DATA( profileMakeHeap, entityQueue_.size() );
	}
	else
	{
		EntityCache::Priority earliestPriority = entityQueue_.front()->priority();

		START_PROFILE( profileClearPrioritised );
		int count=0;
		for (Entity * pVehicle = entity_.pVehicle(); numPrioritised > 0 && pVehicle != NULL;
				pVehicle = pVehicle->pVehicle(), ++count )
		{
			EntityCache * pCache = aoiMap_.find( pVehicle->id() );
			if (!pCache)
			{
				// this case should be already logged above so no additional logs here
				continue;
			}

			if (pCache->isPrioritised())
			{
				pCache->clearPrioritised();
				--numPrioritised;
				MF_ASSERT( numPrioritised >= 0 );
			}
		}
		STOP_PROFILE_WITH_DATA( profileClearPrioritised, count );

		// Must have sent and cleared all prioritised entities.
		MF_ASSERT( numPrioritised == 0 );

		START_PROFILE( profilePush );
		count=0;
		while (queueEnd != entityQueue_.end())
		{
			// We want to do a check to make sure no entity gets sent in the
			// "past". If an entity should have be sent multiple times in a
			// frame it does not keep up with virtual time without this check.
			if ((*queueEnd)->priority() < earliestPriority)
			{
				(*queueEnd)->priority( earliestPriority );
			}

			count++;
			std::push_heap( queueBegin, ++queueEnd, PriorityCompare() );
		}
		STOP_PROFILE_WITH_DATA( profilePush, count );
	}

	// TODO:PM The following is debug code to check if we ever have a floating
	// point overflow. A solution to avoid this would be to use unsigned
	// arithmetic operations. The only limit then would be that the change in a
	// priority could not be bigger than half the maximum of an unsigned
	// integer.

	if (!entityQueue_.empty())
	{
		// TODO: This is implementation-dependant, and probably wrong.
		const EntityCache::Priority highestPriority =
			entityQueue_.back()->priority();

		if (highestPriority + 0.0001 <= highestPriority)
		{
			CRITICAL_MSG( "Witness::update: "
					"Priority underflow: cacheId = %u. id = %u priority = %f\n",
					entityQueue_.back()->pEntity()->id(),
					this->entity().id(),
					highestPriority );
		}
	}

	// Mark the reference position message's reliability as whether or not we
	// have sent any relative position messages that were reliable.
	if (pIsReferencePositionReliable != NULL)
	{
		*pIsReferencePositionReliable |= hasAddedReliableRelativePosition;
	}
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	else
	{
		// No reference position in this update. The player must be non-volatile
		// and not on a vehicle, as we'll use the most recent detailedPosition
		// as the reference position.
		MF_ASSERT( entity_.pVehicle() == NULL &&
			!entity_.volatileInfo().hasVolatile( 0.f ) );
	}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	if (pIsPlayerPositionReliable != NULL && *pIsPlayerPositionReliable)
	{
		this->onSendReliablePosition( entity_.localPosition(),
			entity_.localDirection() );
	}

	{
		AUTO_SCOPED_PROFILE( "updateClientSend" );

		this->flushToClient();
	}

	// Tell the proxy that anything else we send is from next tick
	BaseAppIntInterface::tickSyncArgs::start( this->bundle() ).tickByte =
		(uint8)(CellApp::instance().time() + 1);
}


/**
 * This method is called when our client has just been sent a reliable detailed
 * position for our entity.
 */
void Witness::onSendReliablePosition( Position3D position,
	Direction3D direction )
{
	lastSentReliableGameTime_ = CellApp::instance().time();
	lastSentReliablePosition_ = position;
	lastSentReliableDirection_ = direction;
}


/**
 *	This method returns true if we have rooted our RangeListNode somewhere.
 *	If true, pAoIRoot() is an EntityRangeListNode. If false, pAoIRoot() is
 *	a MobileRangeListNode.
 */
bool Witness::isAoIRooted() const
{
	return pAoIRoot_ != entity_.pRangeListNode();
}


/**
 *	This method sets our AoI root to the given X,Z position in the world.
 */
void Witness::setAoIRoot( float x, float z )
{
	if (this->isAoIRooted())
	{
		// Shuffle the existing AoIRoot around
		SCOPED_PROFILE( SHUFFLE_AOI_TRIGGERS_PROFILE );

		static_cast< MobileRangeListNode * >( pAoIRoot_ )->setPosition( x, z );

		return;
	}

	// Replace our existing non-rooted AoITrigger with a rooted AoITrigger

	RangeListNode * pNewAoIRoot = new MobileRangeListNode( x, z,
		RangeListNode::FLAG_NO_TRIGGERS,
		RangeListNode::FLAG_NO_TRIGGERS );

	// Disabling callbacks is not needed since no script should be triggered but
	// it's helpful for debugging.
	Entity::callbacksPermitted( false );

	// TODO: Remove this const_cast.
	const_cast< RangeList & >( entity().space().rangeList() ).add(
		pNewAoIRoot );

	Entity::callbacksPermitted( true );

	RangeListNode * pOldAoIRoot = this->replaceAoITrigger( pNewAoIRoot );

	MF_ASSERT( pOldAoIRoot == entity().pRangeListNode() );
	MF_ASSERT( this->isAoIRooted() );

}


/**
 *	This method sets our AoI root to be our entity
 */
void Witness::clearAoIRoot()
{
	if (!this->isAoIRooted())
	{
		return;
	}
	
	MobileRangeListNode * pOldAoIRoot =
		static_cast< MobileRangeListNode * >(
			this->replaceAoITrigger( entity().pRangeListNode() ) );

	MF_ASSERT( pAoIRoot_ == entity().pRangeListNode() );
	MF_ASSERT( !this->isAoIRooted() );

	pOldAoIRoot->remove();
	bw_safe_delete( pOldAoIRoot );
}


/**
 *	This method returns our current AoIRoot as a PyObject *
 */
PyObject * Witness::pyGet_aoiRoot()
{
	if (this->isAoIRooted())
	{
		const float x = this->pAoIRoot()->x();
		const float z = this->pAoIRoot()->z();
		PyObject * pPyRootPos = PyTuple_New( 2 );
		PyTuple_SET_ITEM( pPyRootPos, 0, Script::getData( x ) );
		PyTuple_SET_ITEM( pPyRootPos, 1, Script::getData( z ) );
		return pPyRootPos;
	}
	else
	{
		PyObject * pPyEntityRoot = (PyObject *)&entity_;
		Py_INCREF( pPyEntityRoot );
		return pPyEntityRoot;
	}
}


/**
 *	This method sets our current AoIRoot from a PyObject *
 *	Acceptable inputs are this entity, a 2-tuple of X,Z or
 *	a 3-tuple of X,Y,Z where Y is ignored.
 */
int Witness::pySet_aoiRoot( PyObject * value )
{
	if (Entity::Check( value ))
	{
		if (value != (PyObject *)&entity_)
		{
			PyErr_SetString( PyExc_ValueError,
				"This attribute cannot be set to an Entity "
					"this Witness is not attached to." );
			return -1;
		}
		this->clearAoIRoot();
		return 0;
	}

	if (PyVector<Vector2>::Check( value ) || 
		(PyTuple_Check( value ) && PyTuple_Size( value ) == 2))
	{
		Vector2 valueVector2;
		int retVal = Script::setData( value, valueVector2,
			"aoiRoot" );
		if (retVal == 0)
		{
			this->setAoIRoot( valueVector2.x,
				valueVector2.y );
		}
		return retVal;
	}

	if (PyVector<Vector3>::Check( value ) || 
		(PyTuple_Check( value ) && PyTuple_Size( value ) == 3))
	{
		Vector3 valueVector3;
		int retVal = Script::setData( value, valueVector3,
			"aoiRoot" );
		if (retVal == 0)
		{
			this->setAoIRoot( valueVector3.x,
				valueVector3.z );
		}
		return retVal;
	}

	PyErr_SetString( PyExc_ValueError,
		"This attribute can only be set to a Vector2, a Vector3, or "
			"the Entity this Witness is attached to." );
	return -1;
}


/**
 *	This method returns a reference to the next outgoing bundle destined
 *	for the proxy.
 */
Mercury::Bundle & Witness::bundle()
{
	return real_.channel().bundle();
}


/**
 *	This handles the sending of an ENTER_PENDING EntityCache, and its removal
 *	from the entity queue. 
 */
void Witness::handleEnterPending( Mercury::Bundle & bundle,
		KnownEntityQueue::iterator iEntityCache )
{
	this->sendEnter( bundle, *iEntityCache );
	*iEntityCache = entityQueue_.back();
	entityQueue_.pop_back();
}


/**
 *	This method sends the enterAoI message to the client.
 */
void Witness::sendEnter( Mercury::Bundle & bundle, EntityCache * pCache )
{
	size_t oldSize = bundle.size();
	const Entity * pEntity = pCache->pEntity().get();

	pCache->idAlias( this->allocateIDAlias( *pEntity ) );

	MF_ASSERT( !pCache->isRequestPending() );
	pCache->clearEnterPending();
	pCache->setRequestPending();

	pCache->vehicleChangeNum( pEntity->vehicleChangeNum() );

	// TODO: Need to have some sort of timer on the pending entities.
	// At the moment, we do not handle the case where the client does
	// not reply to this message.

	if (pEntity->pVehicle() != NULL)
	{
		BaseAppIntInterface::enterAoIOnVehicleArgs & rEnterAoIOnVehicle =
			BaseAppIntInterface::enterAoIOnVehicleArgs::start( bundle );

		rEnterAoIOnVehicle.id = pEntity->id();
		rEnterAoIOnVehicle.vehicleID = pEntity->pVehicle()->id();
		rEnterAoIOnVehicle.idAlias = pCache->idAlias();
	}
	else
	{
		BaseAppIntInterface::enterAoIArgs & rEnterAoI =
			BaseAppIntInterface::enterAoIArgs::start( bundle );

		rEnterAoI.id = pEntity->id();
		rEnterAoI.idAlias = pCache->idAlias();
	}
	pEntity->pType()->stats().enterAoICounter().
		countSentToOtherClients( bundle.size() - oldSize );
}


/**
 *	This method sends the createEntity message to the client.
 */
void Witness::sendCreate( Mercury::Bundle & bundle, EntityCache * pCache )
{
	size_t oldSize = bundle.size();
	pCache->clearCreatePending();

	if (pCache->isRefresh())
	{
		ERROR_MSG( "Witness::sendCreate: isRefresh = true\n" );
		pCache->clearRefresh();
	}

	MF_ASSERT( pCache->isUpdatable() );

	const Entity & entity = *pCache->pEntity();

	entity.writeVehicleChangeToBundle( bundle, *pCache );

	bool isVolatile = entity.volatileInfo().hasVolatile( 0.f );

	if (isVolatile)
	{
		bundle.startMessage( BaseAppIntInterface::createEntity );
	}
	else
	{
		bundle.startMessage( BaseAppIntInterface::createEntityDetailed );
	}

	{
		CompressionOStream compressedStream( bundle,
			this->entity().pType()->description().externalNetworkCompressionType() );

		compressedStream << entity.id() << entity.clientTypeID();
		compressedStream << entity.localPosition();

		if (isVolatile)
		{
			compressedStream << PackedYawPitchRoll< /* HALFPITCH */ false >(
				entity.localDirection().yaw, entity.localDirection().pitch,
				entity.localDirection().roll );
		}
		else
		{
			compressedStream << entity.localDirection().yaw;
			compressedStream << entity.localDirection().pitch;
			compressedStream << entity.localDirection().roll;
		}

		pCache->addOuterDetailLevel( compressedStream );
	}

	entity.pType()->stats().createEntityCounter().
		countSentToOtherClients( bundle.size() - oldSize );
}


/**
 *	This method sends the bundle to the client associated with this entity.
 */
void Witness::flushToClient()
{
	// Tell the BaseApp to send to the client.
	this->bundle().startMessage( BaseAppIntInterface::sendToClient );

	g_downstreamBytes += this->bundle().size();
	g_downstreamPackets += this->bundle().numDataUnits();
	++g_downstreamBundles;

	// Send bundle via the channel
	real_.channel().send();
}


/**
 *	This class is used to mark all entities in the Witness's AoI as isGone
 *	and isInAoIOffload so that the AoITrigger can be replaced quietly.
 *	This is exactly how Witness::readAoI handles onloading entities.
 */
class AoIPreparer : public EntityCacheMutator
{
public:
	void mutate( EntityCache & cache )
	{
		Entity * pEntity =
			const_cast< Entity * >( cache.pEntity().get() );
		MF_ASSERT( pEntity != NULL );
		MF_ASSERT( !pEntity->isInAoIOffload() );

		pEntity->isInAoIOffload( true );
		cache.setGone();
	}
};


/**
 *	This class is used to trigger callbacks and clean up after
 *	entities which were in AoI when the AoIPreparer was run, but
 *	are no longer in AoI under the newly-inserted AoITrigger.
 *	This is exactly how Witness::init handles onloading entities which
 *	no longer fall into our AoI.
 */
class AoICleaner : public EntityCacheMutator
{
public:
	AoICleaner( Entity & rOwner ) : rOwner_( rOwner ) {}

	void mutate( EntityCache & cache )
	{
		Entity * pEntity =
			const_cast< Entity * >( cache.pEntity().get() );
		MF_ASSERT( pEntity != NULL );
		if (!pEntity->isInAoIOffload())
		{
			return;
		}

		pEntity->isInAoIOffload( false );

		rOwner_.callback( "onLeftAoI", PyTuple_Pack( 1, pEntity ),
			"onLeftAoI", true );

		MF_ASSERT( cache.isGone() );
	}

	Entity & rOwner_;
};


/**
 * 	This method replaces our AoITrigger instance with one centred around
 * 	the given RangeListNode, and returns the centre of the previous
 * 	AoITrigger. It attempts to ensure that the onEnteredAoI and onLeftAoI
 * 	callbacks only trigger where necessary.
 *
 * 	@param pNewAoIRoot	A pointer to the RangeListNode to centre our
 * 						AoITrigger on
 *
 * 	@return				A pointer to the RangeListNode the old
 * 						AoITrigger was centred on
 */
RangeListNode * Witness::replaceAoITrigger( RangeListNode * pNewAoIRoot )
{
	Entity::callbacksPermitted( false );

	RangeListNode * pOldAoIRoot = pAoIRoot_;

	// Scan our aoiMap, mark everything as isInAoIOffload and isGone
	AoIPreparer preparer;
	aoiMap_.mutate( preparer );
	
	if (this->isAoIRooted())
	{
		MobileRangeListNode * pRoot =
			static_cast< MobileRangeListNode * >( pAoIRoot_ );
		pRoot->delTrigger( pAoITrigger_ );
	}
	else
	{
		this->entity().delTrigger( pAoITrigger_ );
	}

	// Potentially changes this->isAoIRooted()
	pAoIRoot_ = pNewAoIRoot;

	// AoITrigger does not contract when it is destroyed, so no
	// onLeftAoI callbacks are triggered.
	bw_safe_delete( pAoITrigger_ );

	{
		SCOPED_PROFILE( SHUFFLE_AOI_TRIGGERS_PROFILE );

		// AoITrigger will expand when created, so now all entities
		// which are in the new AoI have been (re-)added.

		// Entities that were already known do not have their
		// onEnteredAoI callback triggered
		pAoITrigger_ = new AoITrigger( *this, pAoIRoot_, aoiRadius_ );
	}

	if (this->isAoIRooted())
	{
		MobileRangeListNode * pRoot =
			static_cast< MobileRangeListNode * >( pAoIRoot_ );
		pRoot->addTrigger( pAoITrigger_ );
	}
	else
	{
		entity().addTrigger( pAoITrigger_ );
	}

	// Anything still marked as isInAoIOffload needs to have its
	// onLeftAoI callback triggered.
	AoICleaner cleaner( this->entity() );
	aoiMap_.mutate( cleaner );

	Entity::callbacksPermitted( true );

	return pOldAoIRoot;
}


/**
 *	This method adds the input entity to the collection of seen entities.
 *	It is called by addToAoI, or later in requestEntityUpdate.
 */
void Witness::addToSeen( EntityCache * pCache )
{
	EntityCache::Priority initialPriority = 0.0;


	// We want to give the entity the minimum current priority. We need to
	// be careful here. 
	if (!entityQueue_.empty())
	{
		initialPriority = entityQueue_.front()->priority();
	}

	pCache->priority( initialPriority );

// #define DELAY_ENTER_AOI
#ifdef DELAY_ENTER_AOI
	const bool shouldUpdatePriority = !pCache->isEnterPending();

	if (shouldUpdatePriority && !pCache->isGone())
	{
		pCache->updatePriority( entity_.position() );
	}
#endif

	entityQueue_.push_back( pCache );
	std::push_heap( entityQueue_.begin(), entityQueue_.end(), PriorityCompare() );
}


/**
 *	This method allocates a new id alias for the input entity. It may allocate
 *	NO_ID_ALIAS.
 */
IDAlias Witness::allocateIDAlias( const Entity & entity )
{
	// Only give an ID alias to those entities who have volatile data.
	// TODO: Consider whether this should be done on the entity's volatileInfo
	// or the entity type's volatileInfo.
	if (entity.volatileInfo().hasVolatile( 0.f ) &&
			numFreeAliases_ != 0)
	{
		numFreeAliases_--;
		return freeAliases_[ numFreeAliases_ ];
	}

	return NO_ID_ALIAS;
}


/**
 *	This method removes the entity pointed to by the input iterator from this
 *	object's seen collection. It calls onLeaveAoI.
 *
 *	@param bundle	The bundle to put the leaveAoI message on.
 *	@param iter		An iterator pointing to the entity cache of the entity being
 *					removed.
 *	@param id		This is the id of the entity. This should only be passed in
 *					if the entity in the cache is NULL. This can occur when an
 *					entity is offloaded and a referenced entity is not on the
 *					new cell.
 */
void Witness::deleteFromSeen( Mercury::Bundle & bundle,
	KnownEntityQueue::iterator iter )
{
	EntityCache * pCache = *iter;

	this->deleteFromClient( bundle, pCache );
	this->deleteFromAoI( iter );
}


/**
 *	This method informs the client that an entity has left its AoI.
 */
void Witness::deleteFromClient( Mercury::Bundle & bundle,
	EntityCache * pCache )
{
	pCache->clearRefresh();

	EntityID id = pCache->pEntity()->id();

	if (!pCache->isEnterPending())
	{
		pCache->addLeaveAoIMessage( bundle, id );
	}

	this->onLeaveAoI( pCache, id );

	// Reset client related state
	pCache->onEntityRemovedFromClient();
}


/**
 *	This method removes and deletes an EntityCache from the AoI.
 */
void Witness::deleteFromAoI( KnownEntityQueue::iterator iter )
{
	EntityCache * pCache = *iter;

	// We want to remove this entity from the vector. To do this, we move the
	// last element to the one we want to delete and pop the back.
	*iter = entityQueue_.back();
	entityQueue_.pop_back();

	// Now remove it from the AoI map (if it's in there)
	aoiMap_.del( pCache );
}


/**
 *	This helper method is used to send data to the client associated with this
 *	object.
 */
bool Witness::sendToClient( EntityID entityID, Mercury::MessageID msgID,
		MemoryOStream & stream, int msgSize )
{
	Mercury::Bundle & bundle = this->bundle();

	int oldSize = bundle.size();

	if (!this->selectEntity( bundle, entityID ))
	{
		return false;
	}

	bundle.startMessage( BaseAppIntInterface::sendMessageToClient );
	bundle << msgID;
	MF_ASSERT( msgSize < 0 || msgSize == stream.size() );
 	bundle << (int16) msgSize;
	bundle.transfer( stream, stream.size() );

	int messageSize = bundle.size() - oldSize;

	bandwidthDeficit_ += messageSize;

	return true;
}


/**
 *	This helper method is used to send data to the proxy associated with this
 *	object.
 */
void Witness::sendToProxy( Mercury::MessageID msgID, MemoryOStream & stream )
{
	Mercury::Bundle & bundle = this->bundle();

	bundle.startMessage(
		BaseAppIntInterface::gMinder.interfaceElement( msgID ) );
	bundle.transfer( stream, stream.size() );
}


/**
 *	This virtual method is used to adjust the amount of data that is sent to a
 *	particular witness.
 */
void Witness::setWitnessCapacity( EntityID witnessID, int bps )
{
	(void)witnessID;
	// TODO:PM At the moment, the only witness is the proxy.
	MF_ASSERT( witnessID == this->entity().id() );

	int updateHertz = CellAppConfig::updateHertz();

	if (updateHertz != 0)
	{
		maxPacketSize_ = ServerUtil::bpsToPacketSize( bps, updateHertz );
	}
}


/**
 *	This virtual method is used to handle a request from the client to update
 *	the information associated with a particular entity.
 */
void Witness::requestEntityUpdate( EntityID id,
		EventNumber * pEventNumbers, int size )
{
	EntityCache * pCache = aoiMap_.find( id );

	// see if we have the entity at all
	if (pCache == NULL)
	{
		if (id != entity_.id())
		{
			// XXX: I think this can also happen if the entity entered and
			// then left the AoI before we received the requestEntityUpdate
			// message.
			DEBUG_MSG( "Witness::requestEntityUpdate: "
				"Pending entity %u is no longer in AoI of %u "
				"(should be rare and only after offload).\n",
				id, this->entity().id() );
			// It should be rare that pEntity is NULL. This only occurs when the
			// PendingEntity was offloaded and the entity did not exist on the
			// new cell (in which case we've sent the leaveAoI message already)
		}
		else
		{
			ERROR_MSG( "Witness::requestEntityUpdate: "
				"Entity %u requested an update of itself! Bad client.\n", id );
			// just ignore it if it's trying to do this silly thing
		}
		return;
	}

	// see if it is still pending
	if (!pCache->isRequestPending())
	{
		ERROR_MSG( "Witness::requestEntityUpdate: "
				"Request for %u that is not pending in AoI of %u.\n",
			id, this->entity().id() );
		return;
	}

	pCache->clearRequestPending();
	pCache->setCreatePending();

#if 0
	// see if it has since left our AoI
	if (pCache->isGone())
	{
		INFO_MSG( "Witness::requestEntityUpdate: "
			"Pending entity %lu has since left the AoI of %lu.\n",
			id, this->entity().id() );
	}
#endif

	// make sure that the client is not try to stuff us up
	if (size > pCache->numLoDLevels())
	{
		ERROR_MSG( "CHEAT: Witness::requestEntityUpdate: "
			"Client %u sent %d LoD event stamps when max is %d\n",
			entity_.id(), size, pCache->numLoDLevels() );
		size = pCache->numLoDLevels();
	}

	// ok we have the all clear, let's add it to the seen list then
	pCache->lodEventNumbers( pEventNumbers, size );
	this->addToSeen( pCache );
}


/**
 *	This method does the work that needs to be done when an entity leaves this
 *	entity's Area of Interest.
 */
void Witness::onLeaveAoI( EntityCache * pCache, EntityID id )
{
	if (pCache->idAlias() != NO_ID_ALIAS)
	{
		freeAliases_[ numFreeAliases_ ] = pCache->idAlias();
		numFreeAliases_++;
	}
}


/**
 *	This method sets the AoI radius for this entity.
 */
void Witness::setAoIRadius( float radius, float hyst )
{
	SCOPED_PROFILE( SHUFFLE_AOI_TRIGGERS_PROFILE );

	radius = std::max( 0.1f, radius );

	aoiHyst_      = hyst;
	aoiRadius_    = radius;

	if (aoiRadius_ > CellAppConfig::maxAoIRadius())
	{
		WARNING_MSG( "Witness::setAoIRadius: Clamping %u's AoI radius (%.1f) "
			"to the maximum allowed value (%.1f)\n",
			entity_.id(), aoiRadius_, CellAppConfig::maxAoIRadius() );

		aoiRadius_ = CellAppConfig::maxAoIRadius();
	}

	// Disabling callbacks is not needed since no script should be triggered but
	// it's helpful for debugging.
	Entity::callbacksPermitted( false );

	pAoITrigger_->setRange( aoiRadius_ );

	// the range of any other triggers will get updated in the normal
	// way then they are next processed by our update loop
	if (this->isAoIRooted())
	{
		MobileRangeListNode * pRoot =
			static_cast< MobileRangeListNode * >( pAoIRoot_ );
		pRoot->modTrigger( pAoITrigger_ );
	}
	else
	{
		entity_.modTrigger( pAoITrigger_ );
	}

	Entity::callbacksPermitted( true );
}


/**
 *	This method dumps this entity's AoI.
 */
void Witness::dumpAoI()
{
#define DUMPAOI_MSG BW::DebugMsg( "dumpAoI" ).write

	DUMPAOI_MSG( "Dumping AoI for entity %u. Seen = %" PRIzu " "
			"AoIMap = %u AoIRadius = %6.3f AoIHyst = %6.3f\n",
		this->entity().id(),
		entityQueue_.size(),
		aoiMap_.size(),
		aoiRadius_,
		aoiHyst_ );

#ifdef SORTED_DUMP
	KnownEntityQueue::iterator qBegin = entityQueue_.begin();
	KnownEntityQueue::iterator qEnd = entityQueue_.end();

	while (qBegin != qEnd)
	{
		pop_heap( qBegin, qEnd--, PriorityCompare() );
	}
#endif

	KnownEntityQueue::iterator iter = entityQueue_.begin();

	while (iter != entityQueue_.end())
	{
		const Entity & e = *(*iter)->pEntity();
		DUMPAOI_MSG( "%4u: volatile %4d/%4d. event %4d/%4d. priority %c%lf\n",
				e.id(),
				int((*iter)->lastVolatileUpdateNumber()),
				int(e.volatileUpdateNumber()),
				int((*iter)->lastEventNumber()),
				int(e.lastEventNumber()),
				((*iter)->isPrioritised()?'!':' '),
				double((*iter)->priority()) );

		iter++;
	}

#ifdef SORTED_DUMP
	make_heap( entityQueue_.begin(), entityQueue_.end(), PriorityCompare() );
#endif
#undef DUMPAOI_MSG
}


/**
 *	This method adds the input entity to this entity's Area of Interest.
 */
void Witness::addToAoI( Entity * pEntity, bool setManuallyAdded )
{
	if (pEntity->isDestroyed())
	{
		// TODO: It might be nice to have a different value that we use here. It
		// would make debugging easier.
		MF_ASSERT( !this->entity().isInAoIOffload() );
		// We use this flag to indicate that the current dead entity has been
		// triggered artificially. We remember this so that we can respond to
		// the corresponding removeFromAoI correctly.
		this->entity().isInAoIOffload( true );

		return;
	}

	if (!pEntity->pType()->description().canBeOnClient())
	{
		return;
	}

	// see if the entity is already in our AoI
	EntityCache * pCache = aoiMap_.find( *pEntity );

	bool wasInAoIOffload = pEntity->isInAoIOffload();

	if (wasInAoIOffload)
	{
		pEntity->isInAoIOffload( false );
		// TODO: Set cache pointer as result of isInAoIOffload,
		// so we don't need to look it up when onloading.
		MF_ASSERT( pCache != NULL );

		// we want to do the same processing as for if it was gone
		MF_ASSERT( pCache->isGone() );
	}

	if (pCache != NULL)
	{
		if (pCache->isGone())
		{
			pCache->reuse();
		}
		else
		{
			if (setManuallyAdded)
			{
				pCache->setManuallyAdded();
			}
			else
			{
				pCache->setAddedByTrigger();
			}
			return;
		}
	}
	else
	{
		pCache = aoiMap_.add( *pEntity );
		pCache->setEnterPending();
		this->addToSeen( pCache );
	}

	if (setManuallyAdded)
	{
		pCache->setManuallyAdded();
	}
	else
	{
		pCache->setAddedByTrigger();
	}

	// Don't re-call onEnteredAoI during an offload
	if (!wasInAoIOffload)
	{
		if (this->shouldAddReplayAoIUpdates())
		{
			this->entity().cell().pReplayData()->addEntityAoIChange( 
				this->entity().id(), *(pCache->pEntity()), 
				/* hasEnteredAoI */ true );
		}

		// Set isAlwaysDetailed to the entity's type default, and let
		// onEnteredAoI override it.
		pCache->isAlwaysDetailed(
			pEntity->pType()->description().shouldSendDetailedPosition() );

		this->entity().callback( "onEnteredAoI",
			Py_BuildValue( "(O)", pEntity ),
			"onEnteredAoI", true );
	}

	// we should always have a cache entry for this entity by here
	MF_ASSERT( pCache != NULL );
}


/**
 *	This method removes the input entity from this entity's Area of Interest.
 *
 *	It is not actually removed immediately. This method is called when an AoI
 *	trigger is untriggered. It adds this to a leftAoI set and the normal
 *	priority queue updating is used to actually remove it from the list.
 */
void Witness::removeFromAoI( Entity * pEntity, bool clearManuallyAdded )
{
	// Check if this is a remove that is matched with an ignored addToAoI call.
	if (this->entity().isInAoIOffload())
	{
		MF_ASSERT( pEntity->isDestroyed() );
		this->entity().isInAoIOffload( false );

		return;
	}

	if (!pEntity->pType()->description().canBeOnClient())
	{
		return;
	}

	// find the entity in our AoI
	EntityCache * pCache = aoiMap_.find( *pEntity );
	if (pCache == NULL)
	{
		this->entity().space().debugRangeList();

		CRITICAL_MSG( "Witness::removeFromAoI: "
				"Entity %u not in AoI of entity %u!\n",
			pEntity->id(), entity_.id() );
		return;
	}

	if (clearManuallyAdded)
	{
		pCache->clearManuallyAdded();
		if (pCache->isAddedByTrigger())
		{
			return;
		}
	}
	else
	{
		pCache->clearAddedByTrigger();
		if (pCache->isManuallyAdded())
		{
			return;
		}
	}

	MF_ASSERT( !pCache->isGone() );
	pCache->setGone();

	if (this->shouldAddReplayAoIUpdates())
	{
		this->entity().cell().pReplayData()->addEntityAoIChange( 
			this->entity().id(), *(pCache->pEntity()), 
			/* hasEnteredAoI */ false );
	}

	// Also see the calls in Witness::init() for offloading
	this->entity().callback( "onLeftAoI", PyTuple_Pack( 1, pEntity ),
		"onLeftAoI", true );
}


/**
 *	This method informs us that our entity is now at a new position.
 *	We may use this opportunity to update our velocity record.
 *	and check if we're making too much noise.
 */
void Witness::newPosition( const Vector3 & position )
{
	GameTime now = CellApp::instance().time();
	uint32 dur = now - noiseCheckTime_;
	uint32 sinceNoise  = now - noisePropagatedTime_;
	if ((dur > 1) && !noiseMade_)
	{
		const Vector3 & v = real_.velocity();
		if (v.y < -NoiseConfig::verticalSpeed())
		{
			noiseMade_ = true;
		}
		else
		{
			float horizSpeedSqr = ServerUtil::sqr( v.x ) +
				ServerUtil::sqr( v.z );
			if (horizSpeedSqr > NoiseConfig::horizontalSpeedSqr())
			{
				noiseMade_ = true;
			}
		}
		noiseCheckTime_ = now;
	}

	if (sinceNoise > 10 && noiseMade_)
	{
		Entity::nominateRealEntity( this->entity() );
		this->entity().makeNoise( stealthFactor_, 0 );
		Entity::nominateRealEntityPop();

		noiseMade_ = false;
		noisePropagatedTime_ = now;
	}
}


#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
/**
 *	This method converts the entity's position into a reference position. This
 *	just adjusts the position to be aligned to a larger grid so that calculated
 *	positions do not move around as the reference position changes.
 */
void Witness::calculateReferencePosition()
{
	referencePosition_ = ::BW_NAMESPACE calculateReferencePosition(
		entity_.position() );
}


/**
 *	This method updates the position reference based on a client update. When
 *	sending volatile entity positions to the client, the positions are sent as
 *	12-bits floats (in x and z) and are relative to this position reference
 *	point.
 *
 *	@param seqNum The sequence number associated with this reference position.
 */
void Witness::updateReferencePosition( uint8 seqNum )
{
	this->calculateReferencePosition();
	referenceSeqNum_ = seqNum;
	hasReferencePosition_ = true;
}


/**
 *	This method cancels the position reference. When
 *	sending volatile entity positions to the client, the positions are sent as
 *	3 bytes (one uint8 for each of x, y and z), and are relative to this
 *	position reference point. The position space thus is described as a 256m3
 *	cube.
 */
void Witness::cancelReferencePosition()
{
	hasReferencePosition_ = false;
	referencePosition_.set( 0.f, 0.f, 0.f );
	referenceSeqNum_ = 0;
}
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */


/**
 *	This method is used to dump debug information.
 */
void Witness::debugDump()
{
	DEBUG_MSG( "stealthFactor = %6.1f\n", stealthFactor_ );
}


// -----------------------------------------------------------------------------
// Section: Witness's Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( Witness )

PY_BEGIN_METHODS( Witness )
	// Internal python method, only expose for testing.
	//PY_METHOD( unitTest )

	/*~ function Entity dumpAoI
	*  @components{ cell }
	*  dumpAoI sends details of the Entity's area of interest to the cell's DEBUG
	*  message channel with a category of "dumpAoI". A description of the
	*  workings of the AoI system can be found in the Entity class documentation.
	*
	*  A sample message follows:
	*  @{
	*  451.8 DEBUG: Dumping AoI for entity 1000. Seen = 2 AoIMap = 2 AoIRadius = 500.000 AoIHyst =  5.000
	*  451.8 DEBUG: 16777218: volatile  129/ 130. event   45/  45. priority !0.000000
	*  451.8 DEBUG: 16778216: volatile 65535/   0. event    1/   1. priority  0.000000
	*  @}
	*  The first line of this message tells us:
	*  <ul>
	*  <li>the data is for entity #1000</li>
	*  <li>the client is receiving updates for 2 entities in its AoI</li>
	*  <li>it has 2 entities in its AoI</li>
	*  <li>the AoI radius is 500.000</li>
	*  <li>the AoI hysteresis area extends 5.000 beyond the AoI radius</li>
	*  </ul>
	*  The following set of lines provide information for each of the entities
	*  that the Entity is watching. In this example line 2 tells us:
	*  <ul>
	*  <li>the Entity is watching entity #16777218</li>
	*  <li>the last volatile information update that the Entity's client received
	*  for #16777218 was its update #129, and the entity is up to update #130</li>
	*  <li>the last event that the client received for #16777218 was its event #45
	*  and the entity is update event #45</li>
	*  <li>in the Entity's AoI entity #16777218 currently has an update priority
	*  of 0.000000 and is prioritised</li>
	*  </ul>
	*  In this example line 3 tells us:
	*  <ul>
	*  <li>the Entity is watching entity #16778216</li>
	*  <li>the Entity's client has never received a volatile update for #16778216
	*  and the entity has generated no volatile updtes</li>
	*  <li>the last event that the client received for #16778216 was its event #1
	*  and the entity is update event #1</li>
	*  <li>in the Entity's AoI entity #16777218 currently has an update priority
	*  of 0.000000 and is not prioritised</li>
	*  </ul>
	*/
	PY_METHOD( dumpAoI )

	/*~ function Entity setAoIRadius
	*  @components{ cell }
	*  setAoIRadius specifies the size of the Entity's area of interest.
	*
	*  This function is only available on the entity that has an associated
	*  witness.
	*
	*  By default an Entity's AoI area extends 500 metres along both the x and
	*  z axis of the world, and its hysteresis area extends another 5 metre
	*  further.
	*
	*  Note: If you set this to be greater than the bw.xml configuration option
	*  'cellApp/maxAoIRadius', the AoI radius will be clamped to this value.
	*
	*  You can set the default AoI radius by setting the bw.xml configuration
	*  option 'cellApp/defaultAoIRadius'.
	*
	*  @param radius radius specifies the size of the AoI area, as a float.
	*  @param hyst=5 hyst specifies the size of the hysteresis area beyond the
	*  AoI area, as a float.
	*  @return None
	*/
	PY_METHOD( setAoIRadius )

	/*~ function Entity withholdFromClient
	 *	@components{ cell }
	 *
	 *  withholdFromClient allows an entity in an entity's Area of Interest to
	 *  be withheld from being sent to the client.
	 *
	 *  This is often used with Entity.onEnteredAoI and Entity.entitiesInAoI.
	 *
	 *  Example:
	 *  @{
	 *  # Identify and withhold entities that should be withheld.
	 *  def onEnteredAoI( self, entity ):
	 *      if entity.shouldBeWithheldFrom( self ):
	 *          self.withholdFromClient( entity, True )
	 *
	 *  # No longer withhold entities we now have the ability to send.
	 *  def adjustForExtraPowers( self ):
	 *      for entity in self.entitiesInAoI( withheld = True ):
	 *          if not entity.shouldBeWithheldFrom( self ):
	 *              self.withholdFromClient( entity, False )
	 *  @}
	 *
	 *  @param entityOrID An entity or entity ID.
	 *	@param isWithheld A boolean indicating whether the entity should be
	 *    withheld. That is, it is not sent to the client.
	 *
	 *  @return None. If the entity does not exist, a ValueError is raised.
	 */
	PY_METHOD( withholdFromClient )

	/*~ function Entity isWithheldFromClient
	 *	@components{ cell }
	 *
	 *  This method returns whether an entity in the AoI is currently being
	 *  withheld from being sent to the client. This is modified with
	 *  Entity.withholdFromClient.
	 *
	 *  @param entityOrID An entity or entity ID to check
	 *
	 *  @return True if currently withheld, otherwise false. If the entity does
	 *  not exist, a ValueError is raised.
	 */
	PY_METHOD( isWithheldFromClient )

	/*~ function Entity entitiesInAoI
	 *	@components{ cell }
	 *
	 *  This method returns a list of the entities in this entity's Area of
	 *  Interest.
	 *
	 *  Examples:
	 *  @{
	 *  self.entitiesInAoI() # All entities in AoI
	 *
	 *  self.entitiesInAoI( withheld = True ) # All entities in AoI being withheld
	 *
	 *  self.entitiesInAoI( withheld = False ) # All entities in AoI not being withheld
	 *  @}
	 *
	 *  @param withheld The optional keyword argument indicates whether only the
	 *  withheld entities or only the entities not withheld should be included.
	 *  If this is not passed, all entities are included.
	 */
	PY_METHOD( entitiesInAoI )

	/*~ function Entity entitiesInManualAoI
	 *	@components{ cell }
	 *
	 *  This method returns a list of the entities in this entity's Manual Area
	 *	of Interest.
	 *
	 *  Examples:
	 *  @{
	 *  self.entitiesInManualAoI() # All entities in manual AoI
	 *  @}
	 */
	PY_METHOD( entitiesInManualAoI )

	/*~ function Entity isInAoI
	 *	@components{ cell }
	 *
	 *  This method returns whether an entity is in the current entity's Area of
	 *  Interest.
	 *
	 *  @param entityOrID An entity or entity ID to check.
	 */
	PY_METHOD( isInAoI )

	/*~ function Entity setPositionDetailed
	 *	@components{ cell }
	 *
	 *	setPositionDetailed causes an entity in this entity's Area of
	 *	Interest to have its position and direction sent to the client as a
	 *	detailed update any time it would have sent a volatile position update.
	 *
	 *	A detailed position uses more bandwidth, but is always fully accurate
	 *	irrespective of distance. This is the same accuracy as used by
	 *	non-volatile position and direction updates.
	 *
	 *	Note that detailed positions are never snapped to the terrain, even if
	 *	the entity's onGround value is True.
	 *
	 *	Frequency of position update is still governed by the currently set Area
	 *	of Interest update scheme.
	 *
	 *	This value is reset according to the ShouldSendDetailedVolatilePosition
	 *	property of the entity's type if the entity leaves and then re-enters
	 *	an AoI. If used, this would commonly appear in onEnteredAoI.
	 *
	 *	For example,
	 *	@{
	 *	self.setPositionDetailed( otherEntity, True )
	 *	@}
	 *
	 *	@param entity The entity (or entity id) that is in the AoI.
	 *	@param isPositionDetailed A boolean indicating whether position updates
	 *		for the entity should be sent as detailed or not.
	 *
	 *	@return None. If the entity is not in AoI, a ValueError is raised.
	 *
	 *	@see Entity.isPositionDetailed
	 */
	PY_METHOD( setPositionDetailed )

	/*~ function Entity isPositionDetailed
	 *	@components{ cell }
	 *
	 *	isPositionDetailed returns whether position updates for this entity
	 *	are sent to the client as detailed position and direction updates.
	 *
	 *	@param entity The entity (or entity id) that is in the AoI.
	 *
	 *	@return Boolean. If the entity is not in AoI, a ValueError is raised.
	 *
	 *	@see Entity.setPositionDetailed
	 */
	PY_METHOD( isPositionDetailed )

	/*~ function Entity updateManualAoI
	 *	@components{ cell }
	 *
	 *	This method sets the manual AoI entities. Those entities marked as
	 *	manually added that are not present in the new sequence will be
	 *	removed from the AoI.
	 *
	 *  @param entityList 	A sequence where each element is either an entity
	 * 						ID or an entity reference to be added (or retained)
	 * 						in the AoI.
	 *	@param ignoreNonManualAoIEntities
	 *						If False (the default), then an exception is raised
	 *						for entities in the list of a type not marked as
	 *						for manual AoI. If True, no exception is raised,
	 *						but a warning message is logged for each such
	 *						entity.
	 */
	PY_METHOD( updateManualAoI )

	/*~ function Entity addToManualAoI
	 *	@components{ cell }
	 *
	 *	This method adds the given manual-AoI entity to the AoI.
	 *
	 *	@param entity 	An entity ID or entity reference of the entity to
	 *					remove from the AoI. An exception is raised if the
	 *					entity is not a valid entity ID or entity reference, or
	 *					if the entity is already in the AoI.
	 */
	PY_METHOD( addToManualAoI )

	/*~ function Entity removeFromManualAoI
	 *	@components{ cell }
	 *
	 *	This method removes the given manual-AoI entity from the AoI.
	 *
	 *	@param entity 	An entity ID or entity reference of the entity to
	 *					remove from the AoI. An exception is raised if the
	 *					entity is not a valid entity ID or entity reference, or
	 *					if the entity is not currently in the AoI.
	 */
	PY_METHOD( removeFromManualAoI )

	/*~ function Entity setAoIUpdateScheme
	 *	@components{ cell }
	 *
	 *	setAoIUpdateScheme allows an entity in this entity's Area of Interest to
	 *	have a specialised scheme for its update rate in that AoI.
	 *
	 *	When an entity enters another's AoI, the initial update scheme is taken
	 *	from the viewed entity's Entity.aoiUpdateScheme property.
	 *
	 *	This value is reset if the entity leaves and then re-enters an AoI. If
	 *	used, this would commonly appear in onEnteredAoI.
	 *
	 *	For example,
	 *	@{
	 *	self.setAoIUpdateScheme( otherEntity, "sniper" )
	 *	@}
	 *
	 *	@param entity The entity (or entity id) that is in the AoI.
	 *	@param scheme The name of the scheme to use. These are specified in the
	 *		cellApp/aoiUpdateSchemes section of bw.xml
	 */
	PY_METHOD( setAoIUpdateScheme )

	/*~ function Entity getAoIUpdateScheme
	 *	@components{ cell }
	 *
	 *	getAoIUpdateScheme returns the scheme used by an entity in this entity's
	 *	Area of Interest.
	 *
	 *	@param entity The entity (or entity id) that is in the AoI.
	 */
	PY_METHOD( getAoIUpdateScheme )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Witness )
	/*~ attribute Entity bandwidthPerUpdate
	 *	@components{ cell }
	 *
	 *	bandwidthPerUpdate is the maximum packet size per update which is
	 *	sent to the client. This attribute is only available on the entity
	 *	that has an associated witness.
	 *
	 *	This attribute is directly linked to Entity.bpsToClient
	 *
	 *	@see Entity.bpsToClient
	 *	@type int32
	 */
	PY_ATTRIBUTE( bandwidthPerUpdate )

	/*~ attribute Entity bpsToClient
	 *	@components{ cell }
	 *
	 *	bpsToClient is the maximum number of bits per second which is sent to the
	 *	the client. This attribute is only available on the entity that has an
	 *	associated witness.
	 *
	 *	This attribute is directly linked to Entity.bandwidthPerUpdate
	 *
	 *	@see Entity.bandwidthPerUpdate
	 *	@type int32
	 */
	PY_ATTRIBUTE( bpsToClient )

	/*~ attribute Entity stealthFactor
	 *	@components{ cell }
	 *
	 *	stealthFactor is the value that is used to determine how much noise
	 *	this entity makes (from a stealth perspective) due to movement.
	 *
	 *	This attribute is only available on the entity that has an associated
	 *	witness.
	 *
	 *	Whenever an entity with witness moves at a speed faster than
	 *	cellApp/noise/horizontalSpeed (from bw.xml) along the horizontal plane
	 *	or below a velocity of -cellApp/noise/verticalSpeed (from bw.xml) 
	 *	along the vertical axis, it will inform other entities of the noise
	 *	it is making by calling "makeNoise" every 10 ticks, with a noiseLevel
	 *	equal to the stealth factor and an event type of 0.
	 *
	 *	The value that this sets does not have any other effect on the stealth
	 *	system.
	 *	@type float
	 */
	PY_ATTRIBUTE( stealthFactor )

	/*~ attribute Entity aoiRoot
	 *	@components{ cell }
	 *
	 *	aoiRoot is either this entity, or a 2-tuple of X,Z or a 3-tuple of X,Y,Z
	 *	around which the AoI of this entity is centred.
	 *
	 *	This attribute is only available on the entity that has an associated
	 *	witness.
	 *
	 *	The 3-tuple format is provided for setting convenience; it always returns
	 *	either an Entity or a 2-tuple (X,Z)
	 *	@type Entity or 2-tuple (X,Z)
	 */
	PY_ATTRIBUTE( aoiRoot )

PY_END_ATTRIBUTES()

/**
 *	Python getter for Entity.bpsToClient
 */
int Witness::bpsToClient( )
{
	return ServerUtil::packetSizeToBPS( maxPacketSize_,
			CellAppConfig::updateHertz() );
}


/**
 *	Python setter for Entity.bpsToClient
 */
int Witness::pySet_bpsToClient( PyObject * value )
{
	int newValue;
	int retVal = Script::setData( value, newValue, "bandwidthPerUpdate" );

	if ( retVal != 0 )
	{
		return retVal;
	}

	int maxPacketSize = ServerUtil::bpsToPacketSize( newValue,
			CellAppConfig::updateHertz() );

	if (maxPacketSize > 0)
	{
		maxPacketSize_ = maxPacketSize;
		return 0;
	}
	else
	{
		PyErr_Format(PyExc_ValueError, 
			"Packet size would not be greater than zero.");
		return -1;
	}
}


/**
 * @internal
 *	Do some unit tests
 */
void Witness::unitTest()
{
	DEBUG_MSG( "unitTest begins for entity %u\n", entity_.id() );

	DEBUG_MSG( "self refcount before %" PRIzd "\n", entity_.ob_refcnt );

	// add a whole lot of entity caches and remove them
	for (int i = 0; i < 65536; i++)
	{
		EntityCache * pEC = aoiMap_.add( entity_ );
		aoiMap_.del( pEC );
	}

	DEBUG_MSG( "self refcount after %" PRIzd "\n", entity_.ob_refcnt );

	DEBUG_MSG( "unitTest complete.\n" );
}


/**
 *	This static method adds the watchers associated with this class.
 */
void Witness::addWatchers()
{
	MF_WATCH( "stats/numWitnesses", g_numWitnesses );
	MF_WATCH( "stats/totalWitnessesEver", g_numWitnessesEver );

	MF_WATCH( "stats/downstreamBytes", g_downstreamBytes );

	MF_WATCH( "stats/downstreamBundles", g_downstreamBundles );

	MF_WATCH( "stats/downstreamPackets", g_downstreamPackets );

	EntityCacheMap::addWatchers();
}


/**
 *	This static helper function will find an entity from the input Python
 *	object. This object may be the entity or the entity's id.
 *
 *	@param pEntityOrID An entity or an entity id.
 *
 *	@return If the entity could not be found, NULL is returned and the Python
 *		exception state set.
 */
Entity * Witness::findEntityFromPyArg( PyObject * pEntityOrID )
{
	Entity * pEntity = NULL;

	if (Entity::Check( pEntityOrID ))
	{
		pEntity = static_cast< Entity * >( pEntityOrID );
	}
	else if (PyInt_Check( pEntityOrID ))
	{
		long entityID = PyInt_AsLong( pEntityOrID );
		pEntity = CellApp::instance().findEntity( entityID );

		if (pEntity == NULL)
		{
			PyErr_Format( PyExc_KeyError, "%d", int( entityID ) );
		}
	}
	else
	{
		PyErr_SetString( PyExc_TypeError,
				"Expected an entity reference or entity ID" );
	}

	return pEntity;
}


/**
 *	This helper method returns the EntityCache associated with the given Python
 *	object. This may be an entity or an entity id.
 *
 *	@return On failure, the Python exception state is set and NULL returned.
 */
EntityCache * Witness::findEntityCacheFromPyArg( PyObject * pEntityOrID ) const
{
	Entity * pEntity = Witness::findEntityFromPyArg( pEntityOrID );

	if (pEntity == NULL)
	{
		// Exception state set by findEntityFromPyArg
		return NULL;
	}

	EntityCache * pCache = aoiMap_.find( *pEntity );

	if (pCache == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Entity %d is not in the AoI of %d\n",
				pEntity->id(), this->entity().id() );
	}

	return pCache;
}


/**
 *	This method modifies whether an entity is withheld from being sent to a
 *	client.
 *
 *	@return On failure, the Python exception state is set and false is returned.
 */
bool Witness::withholdFromClient( PyObjectPtr pEntityOrID, bool isWithheld )
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return false;
	}

	if (isWithheld)
	{
		pCache->setWithheld();
	}
	else
	{
		pCache->clearWithheld();
	}

	if (this->shouldAddReplayAoIUpdates())
	{
		this->entity().cell().pReplayData()->addEntityAoIChange( 
			this->entity().id(), *(pCache->pEntity()), !isWithheld );
	}

	return true;
}


/**
 *	This method returns whether an entity in the current AoI is being withheld
 *	from the client. This is modified with Witness::withholdFromClient.
 */
PyObject * Witness::isWithheldFromClient( PyObjectPtr pEntityOrID ) const
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return NULL;
	}

	return Script::getData( pCache->isWithheld() );
}


/**
 *	This class is using to update the manual AoI list. We visit and check each
 *	entity in the Witness' AoI and adding new entities for those that are
 *	manually updated.
 */
class ManualAoIVisitor : public EntityCacheVisitor
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param witness 	The witness, whose AoI we are visiting.
	 */
	ManualAoIVisitor( Witness & witness ) :
		witness_( witness )
	{}


	/*
	 *	Override from EntityCacheVisitor.
	 */
	virtual void visit( const EntityCache & cache )
	{
		Entity * pEntity = const_cast< Entity * >( cache.pEntity().get() );

		EntitySet::iterator iter = manualAoIEntities_.find( pEntity );

		if (iter != manualAoIEntities_.end())
		{
			// No need to re-add it existing entity.
			manualAoIEntities_.erase( iter );
		}
		else if (!cache.isGone() &&
				pEntity->pType()->description().isManualAoI())
		{
			// This was previously manual, and now is not part of the list.
			witness_.removeFromAoI( pEntity, /* clearManuallyAdded */ true );
		}
	}


	/**
	 *	This method adds a manual-AoI entity.
	 */
	void addManualAoIEntity( Entity * pEntity )
	{
		manualAoIEntities_.insert( pEntity );
	}


	/**
	 *	Add the collected manual-AoI entities that still need to be added to the
	 *	AoI.
	 */
	void addNewManualAoIEntities()
	{
		for (EntitySet::iterator iter = manualAoIEntities_.begin();
				iter != manualAoIEntities_.end();
				++iter )
		{
			witness_.addToAoI( *iter, /* setManuallyAdded */ true );

			DEBUG_MSG( "ManualAoIVisitor::processNewEntity: "
					"For witness %d, added new manual-AoI entity %d\n",
				witness_.entity().id(), (*iter)->id() );
		}
	}


private:
	typedef BW::set< Entity * > EntitySet;

	EntitySet manualAoIEntities_;
	Witness & witness_;
};


/**
 *	This method sets the manual AoI entities. Those entities marked as manually
 *	added that are not on the new list will be removed.
 *
 *	@param list 	The list of entity IDs or entity references to add to the
 *					AoI.
 *	@param ignoreNonManualAoIEntities
 *					If false then an exception is raised for entities in the
 *					list of a type not marked as for manual AoI. If true, no
 *					exception is raised, but a warning message is logged for
 *					each such entity.
 *
 *	@return true on success, false if an exception was raised.
 */
bool Witness::updateManualAoI( ScriptSequence list,
		bool ignoreNonManualAoIEntities )
{
	ManualAoIVisitor manualAoIVisitor( *this );

	// First pass: validate the entity/ID list

	for (ScriptSequence::size_type i = 0; i < list.size(); ++i)
	{
		ScriptObject item = list.getItem( i, ScriptErrorRetain() );

		if (!item.exists())
		{
			return false;
		}

		Entity * pEntity = Witness::findEntityFromPyArg( item.get() );

		if (NULL == pEntity)
		{
			return false;
		}

		if (!pEntity->pType()->description().isManualAoI())
		{
			if (ignoreNonManualAoIEntities)
			{
				WARNING_MSG( "Witness::updateManualAoI: "
						"%s %d is not marked as being a manual-AoI entity "
						"type\n",
					pEntity->pType()->name(), pEntity->id() );
			}
			else
			{
				PyErr_Format( PyExc_TypeError, "%s %d is not marked as being a "
						"manual-AoI entity type",
					pEntity->pType()->name(), pEntity->id() );
				return false;
			}
		}
	}

	// Second pass after validation
	for (ScriptSequence::size_type i = 0; i < list.size(); ++i)
	{
		ScriptObject item = list.getItem( i, ScriptErrorClear() );

		Entity * pEntity = Witness::findEntityFromPyArg( item.get() );

		if (pEntity->pType()->description().isManualAoI())
		{
			manualAoIVisitor.addManualAoIEntity( pEntity );
		}
	}

	aoiMap_.visit( manualAoIVisitor );

	manualAoIVisitor.addNewManualAoIEntities();

	return true;
}


/**
 *	This method adds a manual-AoI entity to the AoI.
 *  If the IsManualAoI configuration option 
 *  is set to false in an entity definition file and
 *  the addToManualAoI entity will be correctly added
 *  to the AoI.
 *
 *	@param pEntityOrID The entityID or entity reference of the entity to add.
 */
bool Witness::addToManualAoI( PyObjectPtr pEntityOrID )
{
	Entity * pEntity = Witness::findEntityFromPyArg( pEntityOrID.get() );

	if (pEntity == NULL)
	{
		return false;
	}

	if (pEntity->isDestroyed())
	{
		PyErr_Format( PyExc_ValueError,
			"Cannot add a destroyed entity (%d) to the manual-AoI",
			int(pEntity->id()) );
		return false;
	}

	EntityCache * pCache = aoiMap_.find( *pEntity );

	if (pCache && !pCache->isGone() && pCache->isManuallyAdded())
	{
		PyErr_Format( PyExc_ValueError, "%s %d is already in the AoI",
			pEntity->pType()->name(), pEntity->id() );
		return false;
	}

	this->addToAoI( pEntity, /* setManuallyAdded */ true );

	return true;
}


/**
 *	This method removes a manual-AoI entity from the AoI.
 *  If the IsManualAoI configuration option 
 *  is set to false, and addToManualAoI method is called
 *  the entity will be correctly added to the AoI.
 *  When the removeFromManualAoI method is called for
 *  such kind of entities it will remove it from 
 *  the manual AoI and the entity will
 *  receive automatic AoI management.
 *
 *	@param pEntityOrID 	The entity ID or entity reference of the entity to
 *						remove.
 */
bool Witness::removeFromManualAoI( PyObjectPtr pEntityOrID )
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );
	if (pCache == NULL)
	{
		return false;
	}

	Entity * pEntity = const_cast< Entity * >( pCache->pEntity().get() );

	if (pCache->isGone())
	{
		PyErr_Format( PyExc_TypeError, "%s %d is not in the AoI",
			pEntity->pType()->name(), int( pEntity->id() ) );
		return false;
	}

	if (!pCache->isManuallyAdded())
	{
		PyErr_Format( PyExc_TypeError, "%s %d is not marked as being a "
				"manual-AoI entity type",
			pEntity->pType()->name(), int( pEntity->id() ) );
		return false;
	}

	this->removeFromAoI( pEntity, /* clearManuallyAdded */ true );

	return true;
}


/**
 *	This method modifies whether an entity is being sent to a client with
 *	detailed volatile position updates.
 *
 *	@return On failure, the Python exception state is set and false is returned.
 */
bool Witness::setPositionDetailed( PyObjectPtr pEntityOrID,
	bool isPositionDetailed )
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return false;
	}
	pCache->isAlwaysDetailed( isPositionDetailed );

	return true;
}


/**
 *	This method returns whether an entity in the current AoI is being sent to
 *	the client with detailed position. This is modified with
 *	Witness::setDetailedPosition.
 */
PyObject * Witness::isPositionDetailed( PyObjectPtr pEntityOrID ) const
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return NULL;
	}

	return Script::getData( pCache->isAlwaysDetailed() );
}


namespace
{

/**
 *	This class is a base for Witness' AoI visitor implementations that create
 *	a Python list containing the entities in a Witness' AoI.
 */
class BaseAoICollector : public EntityCacheVisitor
{
public:

	// Returns a new reference.
	ScriptList getList() const
	{
		return pList_;
	}

protected:

	/**
	 *	Constructor.
	 */
	BaseAoICollector() : pList_( ScriptList::create() )
	{
	}

	// Add entity to the list.
	void addToList( const EntityCache & cache )
	{
		const EntityConstPtr pEntity = cache.pEntity();

		if (!pEntity->isDestroyed())
		{
			pList_.append( ScriptObject( pEntity ) );
		}
	}

private:
	ScriptList pList_;
};


/**
 *	This class is used to create a Python list containing the entities in a
 *	Witness' AoI.
 *	@param PRED	Type of the predicate functor. The predicate should have the
 *				following signature:
 *				@code
 *					bool operator()( const EntityCache & cache ) const
 *				@endcode
 */
template <typename PRED>
class AoICollector : public BaseAoICollector
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param predicate	A functor to test which entities should be added to
	 *						the list.
	 */
	AoICollector( const PRED & predicate ) : predicate_ ( predicate )
	{
	}

	// Override from EntityCacheVisitor.
	void visit( const EntityCache & cache ) /* override */
	{
		if (predicate_( cache ))
		{
			this->addToList( cache );
		}
	}

private:
	const PRED predicate_;
};


/**
 *	This class is a functor to be used with @ref AoICollector. The predicate
 *	does not filter any entries.
 */
class IncludeAllEntities
{
public:
	bool operator() ( const EntityCache & /*cache*/ ) const
	{
		return true;
	}
};


/**
 *	This class is a functor to be used with @ref AoICollector. The predicate
 *	only includes entities with @a isWithheld() property equal to the one
 *	provided to the functor constructor.
 */
class FilterWithheldEntities
{
public:
	FilterWithheldEntities( bool onlyWithheld ) : onlyWithheld_( onlyWithheld )
	{
	}

	bool operator() ( const EntityCache & cache ) const
	{
		return cache.isWithheld() == onlyWithheld_;
	}

private:
	const bool onlyWithheld_;
};


/**
 *	This class is a functor to be used with @ref AoICollector. The predicate
 *	only includes entities with manual AoI control.
 */
class FilterManualEntities
{
public:
	bool operator()( const EntityCache & cache ) const
	{
		return cache.isManuallyAdded();
	}
};


} // anonymous namespace


/**
 *	This method constructs a Python list of entities within AoI that satisfy the
 *	predicate.
 *	@param predicate	A functor to test which entities should be added to
 *						the list (@see AoICollector).
 *	@return				Python list of entities.
 */
template <typename PRED>
ScriptList Witness::entitiesInAoI( PRED predicate ) const
{
	AoICollector<PRED> visitor( predicate );
	aoiMap_.visit( visitor );
	return visitor.getList();
}


/**
 *	This method returns a Python list containing entities from this Witness'
 *	AoI.
 */
PyObject * Witness::entitiesInAoI( bool includeAll, bool onlyWithheld ) const
{
	ScriptList ret;
	if (includeAll)
	{
		ret = this->entitiesInAoI( IncludeAllEntities() );
	}
	else
	{
		ret = this->entitiesInAoI( FilterWithheldEntities( onlyWithheld ) );
	}
	return ret.newRef();
}


/**
 *	This method implements the Entity.isInAoI script method.
 */
bool Witness::isInAoI( PyObjectPtr pEntityOrID ) const
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		PyErr_Clear();
		return false;
	}

	return true;
}


/**
 *	This method implements the Entity.entitiesInAoI method.
 */
PyObject * Witness::py_entitiesInAoI( PyObject * args, PyObject * kwargs )
{
	const char * keywords[] = { "withheld", NULL };
	PyObject * pArg = NULL;

	if (!PyArg_ParseTupleAndKeywords( args, kwargs,
				"|O:entitiesInAoI", (char**)keywords, &pArg ))
	{
		return NULL;
	}

	bool includeAll = (pArg == NULL);
	bool onlyWithheld = pArg && PyObject_IsTrue( pArg );

	return this->entitiesInAoI( includeAll, onlyWithheld );
}


/**
 *	This method implements the Entity.entitiesInManualAoI method.
 */
ScriptList Witness::entitiesInManualAoI() const
{
	return this->entitiesInAoI( FilterManualEntities() );
}


/**
 *	This method sets the AoI update scheme for a specific entity in our AoI.
 */
bool Witness::setAoIUpdateScheme( PyObjectPtr pEntityOrID,
		BW::string schemeName )
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return false;
	}

	AoIUpdateSchemeID id;

	if (!AoIUpdateSchemes::getIDFromName( schemeName, id ))
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid scheme name '%s'", schemeName.c_str() );
		return false;
	}

	pCache->updateSchemeID( id );

	return true;
}


/**
 *	This method gets the AoI update scheme for a specific entity in our AoI.
 */
PyObject * Witness::getAoIUpdateScheme( PyObjectPtr pEntityOrID )
{
	EntityCache * pCache = this->findEntityCacheFromPyArg( pEntityOrID.get() );

	if (pCache == NULL)
	{
		return NULL;
	}

	BW::string name = "Unspecified";

	if (!AoIUpdateSchemes::getNameFromID( pCache->updateSchemeID(), name ))
	{
		WARNING_MSG( "Witness::getAoIUpdateScheme: "
				"Entity %d has invalid scheme id (%d)\n",
				pCache->pEntity()->id(), int( pCache->updateSchemeID() ) );
	}

	return Script::getData( name );
}


/**
 *	This method determines whether we should be adding AoI updates for replay.
 */
bool Witness::shouldAddReplayAoIUpdates() const
{
	return (shouldAddReplayAoIUpdates_ && 
		entity_.cell().pReplayData() && 
		(entity_.cell().pReplayData()->recordingSpaceEntryID() ==
			real_.recordingSpaceEntryID() ));
}


// -----------------------------------------------------------------------------
// Section: AoITrigger
// -----------------------------------------------------------------------------

/**
 * This method is called when an entity enters (triggers) the AoI.
 * It forwards the call to the entity.
 *
 * @param entity The entity who triggered this trigger/entered AoI.
 */
void AoITrigger::triggerEnter( Entity & entity )
{
	if ((&entity != &owner_.entity()) &&
			!entity.pType()->description().isManualAoI())
	{
		owner_.addToAoI( &entity, /* setManuallyAdded */ false );
	}
}


/**
 * This method is called when an entity leaves (untriggers) the AoI.
 * It forwards the call to the entity
 *
 * @param entity The entity who untriggered this trigger/left AoI.
 */
void AoITrigger::triggerLeave( Entity & entity )
{
	if ((&entity != &owner_.entity()) &&
			(!entity.pType()->description().isManualAoI()))
	{
		owner_.removeFromAoI( &entity, /* clearManuallyAdded */ false );
	}
}


/**
 *	This method returns the identifier for the AoITriggerNode
 *
 *	@return the string identifier of the node
 */
BW::string AoITrigger::debugString() const
{
	char buf[128];
	bw_snprintf( buf, sizeof(buf), "AoI of %u around %s",
		owner_.entity().id(),
		this->pCentralNode()->debugString().c_str() );

	return buf;
}

BW_END_NAMESPACE

// witness.cpp
