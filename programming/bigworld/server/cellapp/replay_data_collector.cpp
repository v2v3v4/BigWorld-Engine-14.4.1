#include "script/first_include.hpp"
#include "script/py_script_object.hpp"

#include "replay_data_collector.hpp"

#include "cellapp.hpp"
#include "entity.hpp"
#include "real_entity.hpp"
#include "witness.hpp"

#include "connection/replay_data.hpp"

#include "entitydef/data_description.hpp"
#include "entitydef/entity_description.hpp"
#include "entitydef/method_description.hpp"
#include "entitydef/property_change.hpp"

#include "network/compression_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 *
 *	@param name 					The name of the replay data. The actual
 *									file name will be derived from this.
 *	@param shouldRecordAoIEvents 	If true, AoI events are recorded,
 *									otherwise, they are not.
 					
 */
ReplayDataCollector::ReplayDataCollector( const BW::string & name,
			bool shouldRecordAoIEvents ) :
		name_( name ),
		shouldRecordAoIEvents_( shouldRecordAoIEvents ),
		data_(),
		recordingSpaceEntryID_(),
		movedEntities_(),
		bufferedEntityData_()
{}


/**
 *	Destructor.
 */
ReplayDataCollector::~ReplayDataCollector()
{
	if (!bufferedEntityData_.empty())
	{
		DEBUG_MSG( "ReplayDataCollector::~ReplayDataCollector: "
				"Discarding %zu entities' buffered data\n",
			bufferedEntityData_.size() );

		BufferedEntityData::iterator iter =
			bufferedEntityData_.begin();
		while (iter != bufferedEntityData_.end())
		{
			bw_safe_delete( iter->second );
			++iter;
		}
	}
}


/**
 *	This method adds the given entity's entire property and volatile state.
 */
void ReplayDataCollector::addEntityState( const Entity & entity )
{
	if (entity.pReal()->recordingSpaceEntryID() == recordingSpaceEntryID_)
	{
		return;
	}

	entity.pReal()->recordingSpaceEntryID( recordingSpaceEntryID_ );

	if (!entity.pType()->description().canBeOnClient())
	{
		return;
	}

	data_ << ReplayData::BlockType( ReplayData::BLOCK_TYPE_ENTITY_CREATE );
	data_ << entity.vehicleID();
	{
		CompressionOStream compressedStream( data_, 
			entity.pType()->description().externalNetworkCompressionType() );

		compressedStream << entity.id() << entity.clientTypeID();
		compressedStream << entity.localPosition();
		compressedStream << entity.localDirection().yaw;
		compressedStream << entity.localDirection().pitch;
		compressedStream << entity.localDirection().roll;

		const EntityDescription & entityDesc = entity.pType()->description();

		// Send all the properties across, in the same format as what
		// Witness::sendCreate() would generate.
		uint propertyCount = entityDesc.clientServerPropertyCount();
		int numToSend = 0;

		for (uint i = 0; i < propertyCount; ++i)
		{
			const DataDescription * pDataDesc =
				entityDesc.clientServerProperty( i );

			if (pDataDesc->isExposedForReplay())
			{
				if (pDataDesc->isBaseData() && !pDataDesc->getItemFrom(
					entity.exposedForReplayClientProperties() ))
				{
					// Exposed for replay base property, but the value is
					// not available yet
					continue;
				}
				++numToSend;
			}
		}

		MF_ASSERT( numToSend < 256 );

		// Send the properties.
		compressedStream << uint8(numToSend);

		if (numToSend == 0)
		{
			return;
		}


		for (uint i = 0; i < propertyCount; ++i)
		{
			const DataDescription * pDataDesc = 
				entityDesc.clientServerProperty( i );

			if (pDataDesc->isExposedForReplay())
			{
				ScriptObject value;
				if (pDataDesc->isBaseData())
				{
					value = pDataDesc->getItemFrom(
							entity.exposedForReplayClientProperties() );

					if (!value)
					{
						continue;
					}
				}
				else
				{
					value = entity.propertyByDataDescription( pDataDesc );
				}

				compressedStream << uint8(i);

				ScriptDataSource source( value );
				pDataDesc->addToStream( source, compressedStream,
					/* isPersistentOnly */ false );
			}
		}
	}

	if (entity.pReal()->pWitness() != NULL)
	{
		this->addEntityPlayerStateChange( entity );
	}

	BufferedEntityData::iterator iter = 
		bufferedEntityData_.find( entity.id() );
	if (iter != bufferedEntityData_.end())
	{
		data_.transfer( *(iter->second),
			iter->second->size() );
		bw_safe_delete( iter->second );
		bufferedEntityData_.erase( iter );
	}
}


/**
 *	This method adds the volatile data of the given entity.
 */
void ReplayDataCollector::queueEntityVolatile( const Entity & entity )
{
	if (!entity.pType()->description().isClientType())
	{
		return;
	}

	movedEntities_.insert( std::make_pair( entity.id(), &entity ) );
}


/**
 *	This method adds volatile data for queued entities.
 */
void ReplayDataCollector::addVolatileDataForQueuedEntities()
{
	MovedEntities::const_iterator iMovedEntity = movedEntities_.begin();

	while (iMovedEntity != movedEntities_.end())
	{
		const Entity & entity = *(iMovedEntity->second);

		if (entity.isOnGround())
		{
			data_ << 
				ReplayData::BlockType( 
					ReplayData::BLOCK_TYPE_ENTITY_VOLATILE_ON_GROUND ) << 
				entity.id() <<
				entity.localPosition().x << 
				entity.localPosition().z << 
				entity.localDirection() <<
				entity.vehicleID();
		}
		else
		{
			data_ << ReplayData::BlockType( 
					ReplayData::BLOCK_TYPE_ENTITY_VOLATILE ) << 
				entity.id() << 
				entity.localPosition() << 
				entity.localDirection() << 
				entity.vehicleID();
		}

		++iMovedEntity;
	}
}


/**
 *	This method adds the method data for the given entity.
 */
void ReplayDataCollector::addEntityMethod( const Entity & entity, 
		const MethodDescription & methodDescription, BinaryIStream & data )
{
	if (!entity.pType()->description().canBeOnClient())
	{
		data.finish();
		return;
	}

	BinaryOStream * pStream = this->getStreamForEntity( entity );

	*pStream << ReplayData::BlockType( ReplayData::BLOCK_TYPE_ENTITY_METHOD ) << 
		entity.id() << methodDescription.exposedMsgID();

	*pStream << int32(data.remainingLength());
	pStream->transfer( data, data.remainingLength() );
}


/**
 *	This method adds the property data for the given entity.
 */
void ReplayDataCollector::addEntityProperty( const Entity & entity,
		const DataDescription & dataDescription,
		PropertyChange & change )
{
	if (!entity.pType()->description().canBeOnClient())
	{
		// Forbid entities that don't have a client script from being replayed.
		return;
	}

	if (!dataDescription.isExposedForReplay())
	{
		return;
	}

	if (entity.pReal()->recordingSpaceEntryID() != recordingSpaceEntryID_)
	{
		// Hasn't been added yet, this could be because a property is being set
		// in __init__. Ignore, because addEntityState() will be called soon.
		return;
	}

	PropertyOwnerBase * propertyOwner;

	// TODO: Not const cast
	if (!(const_cast< Entity & >(entity)).getTopLevelOwner( change, 
			propertyOwner ))
	{
		ERROR_MSG( "ReplayDataCollector::addEntityProperty: "
				"failed to get top level owner for %s %d property %s\n",
			entity.pType()->name(),
			entity.id(),
			dataDescription.name().c_str() );

		return;
	}

	data_ << ReplayData::BlockType( 
			ReplayData::BLOCK_TYPE_ENTITY_PROPERTY_CHANGE ) <<
		entity.id();

	data_ << uint8( change.isSlice() );

	MemoryOStream data;
	change.addToExternalStream( data,
		dataDescription.clientServerFullIndex(),
		entity.pType()->propCountClientServer() );
	data_ << int32( data.remainingLength() );
	data_.transfer( data, data.remainingLength() );
}

/**
 *	This method adds the exposed for replay base properties data
 *	for the given entity.
 */
void ReplayDataCollector::addEntityProperties( const Entity & entity,
		ScriptDict properties )
{
	const EntityDescription & desc = entity.pType()->description();

	const uint propertyCount = desc.clientServerPropertyCount();
	for (uint i = 0; i < propertyCount; ++i)
	{
		const DataDescription * pDataDesc = desc.clientServerProperty( i );
		ScriptObject value = pDataDesc->getItemFrom( properties );

		if (value)
		{
			MF_ASSERT( pDataDesc->isExposedForReplay() );
			SinglePropertyChange change( pDataDesc->clientServerFullIndex(),
					propertyCount, *pDataDesc->dataType() );

			change.setValue( value );

			data_ << ReplayData::BlockType(
					ReplayData::BLOCK_TYPE_ENTITY_PROPERTY_CHANGE ) <<
				entity.id();

			data_ << uint8( change.isSlice() );

			MemoryOStream data;
			change.addToExternalStream( data,
				pDataDesc->clientServerFullIndex(),
				propertyCount );
			data_ << int32( data.remainingLength() );
			data_.transfer( data, data.remainingLength() );
		}
	}
}


namespace // (anonymous)
{

/**
 *	This class collects IDs of entities that are in a player's AoI.
 */
class AoIEntityIDCollector : public EntityCacheVisitor
{
public:
	typedef BW::vector< EntityID > Collection;

	/**
	 *	Constructor.
	 *
	 *	@param player 	The player entity.
	 */
	AoIEntityIDCollector( const Entity & player ) :
			EntityCacheVisitor(),
			player_( player ),
			entityIDs_()
	{}

	/** 
	 *	Destructor.
	 */
	virtual ~AoIEntityIDCollector() {}


	/* 
	 *	Override from EntityCacheVisitor. 
	 */
	virtual void visit( const EntityCache & cache )
	{
		if (!cache.pEntity() || cache.isWithheld() ||
				// Teleported entities will still have entities from source
				// space initially - skip these.
				(cache.pEntity()->spaceID() != player_.spaceID()))
		{
			return;
		}

		entityIDs_.push_back( cache.pEntity()->id() );
	}

	/**
	 *	This method returns the IDs of entities that are outside the AoI
	 *	radius, but considered part of the AoI (e.g. non-zero AppealRadius).
	 */
	const Collection & entityIDs() const 
	{ 
		return entityIDs_;
	}
	
private:
	const Entity & player_;
	Collection entityIDs_;
};


} // end namespace (anonymous)


/**
 *	This method adds a player state change for the given entity.
 *
 *	@param entity 	The entity transitioning player state.
 */
void ReplayDataCollector::addEntityPlayerStateChange( const Entity & entity )
{
	if (!shouldRecordAoIEvents_)
	{
		return;
	}

	data_ << ReplayData::BlockType( 
		ReplayData::BLOCK_TYPE_ENTITY_PLAYER_STATE_CHANGE ) << entity.id();

	MF_ASSERT( entity.pReal() != NULL );

	if (entity.pReal()->pWitness() == NULL)
	{
		data_ << uint8( 0 );
		return;
	}

	data_ << uint8( 1 );

	AoIEntityIDCollector idCollector( entity );
	entity.pReal()->pWitness()->visitAoI( idCollector );

	data_ << uint32( idCollector.entityIDs().size() );

	AoIEntityIDCollector::Collection::const_iterator iEntityID = 
		idCollector.entityIDs().begin();

	while (iEntityID != idCollector.entityIDs().end())
	{
		data_ << *iEntityID;
		++iEntityID;
	}
}


/**
 *	This method adds a change to the withholding state for an entity.
 */
void ReplayDataCollector::addEntityAoIChange( EntityID playerID, 
		const Entity & entity, bool hasEnteredAoI )
{
	if (!shouldRecordAoIEvents_)
	{
		return;
	}

	BinaryOStream * pStream = &data_;

	if (hasEnteredAoI)
	{
		Entity * pPlayer = CellApp::instance().findEntity( playerID );
		MF_ASSERT( pPlayer && pPlayer->pReal() );
		// Buffer entry events potentially. For exit events, these are recorded
		// before the entity deletion event, so no need to buffer.
		pStream = this->getStreamForEntity( *pPlayer );
	}

	*pStream << ReplayData::BlockType( 
			ReplayData::BLOCK_TYPE_PLAYER_AOI_CHANGE ) <<
		playerID << entity.id() << uint8( hasEnteredAoI );
}


/**
 *	This method is called to add an entity deletion.
 */
void ReplayDataCollector::deleteEntity( EntityID entityID )
{
	// TODO: It's possible for an entity to destroy itself in __init__, so this
	// may need to be buffered. Entity creation code should still call
	// addEntityState() though in that case.

	data_ << ReplayData::BlockType( ReplayData::BLOCK_TYPE_ENTITY_DELETE ) <<
		entityID;

	movedEntities_.erase( entityID );
}


/**
 *	This method adds space data.
 */
void ReplayDataCollector::addSpaceData( const SpaceEntryID & spaceEntryID, 
		uint16 key, const BW::string & value )
{
	data_ << ReplayData::BlockType( ReplayData::BLOCK_TYPE_SPACE_DATA ) <<
		spaceEntryID << key << value;
}


/**
 *	This method clears the tick data.
 */
void ReplayDataCollector::clear()
{
	data_.reset();
	movedEntities_.clear();
}


/**
 *	This method adds the message to indicate the final tick.
 */
void ReplayDataCollector::addFinish()
{
	data_ << ReplayData::BlockType( ReplayData::BLOCK_TYPE_FINAL );
}


/**
 *	This method returns an appropriate stream for the given entity, depending
 *	on whether it needs to be buffered before a entity creation message added
 *	in addEntityState().
 */
BinaryOStream * ReplayDataCollector::getStreamForEntity( const Entity & entity )
{
	MF_ASSERT( entity.pReal() != NULL );

	if (entity.pReal()->recordingSpaceEntryID() == recordingSpaceEntryID_)
	{
		return &data_;
	}

	// We need to be buffering.

	BinaryOStream * pStream = NULL;

	BufferedEntityData::const_iterator iter =
		bufferedEntityData_.find( entity.id() );

	if (iter == bufferedEntityData_.end())
	{
		MemoryOStream * pBuffer = new MemoryOStream();
		bufferedEntityData_[ entity.id() ] = pBuffer;
		pStream = pBuffer;
	}
	else
	{
		pStream = iter->second;
	}

	return pStream;
}


BW_END_NAMESPACE


// replay_data_collector.cpp
