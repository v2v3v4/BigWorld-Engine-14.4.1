#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "entity_cache.hpp"

#include "cellapp.hpp"
#include "entity.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "entitydef/script_data_source.hpp"
#include "entitydef/script_data_sink.hpp"

#include "network/bundle.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity_cache.ipp"
#endif

namespace
{
uint32 g_numInAoI = 0;
uint32 g_numInAoIEver = 0;
}


// -----------------------------------------------------------------------------
// Section: EntityCacheMap
// -----------------------------------------------------------------------------

/**
 *	Destructor.
 */
EntityCacheMap::~EntityCacheMap()
{
	g_numInAoI -= this->size();
}


/**
 *	Add the given entity into the entity cache map
 */
EntityCache * EntityCacheMap::add( const Entity & e )
{
	++g_numInAoI;
	++g_numInAoIEver;
	EntityCache eCache( &e );
	Implementor::iterator iter = set_.insert( eCache ).first;

	return const_cast< EntityCache *>( &*iter );
}

/**
 *	Delete the given entity cache from the entity cache map
 *	(the entity cache itself will be deleted by this operation)
 */
void EntityCacheMap::del( EntityCache * ec )
{
	--g_numInAoI;

	MF_ASSERT( ec );
	MF_ASSERT( ec->pEntity() );
	// set_.erase( EntityCacheMap::toIterator( ec ) );
	set_.erase( *ec );
}

/**
 *	Find the given entity in the entity cache map
 */
EntityCache * EntityCacheMap::find( const Entity & e ) const
{
	EntityCache ec( &e );
	Implementor::const_iterator found = set_.find( ec );
	if (found == set_.end())
	{
		return NULL;
	}

	 return const_cast< EntityCache * >( &*found );
}


/**
 *	Find the given entity in the entity cache map
 */
EntityCache * EntityCacheMap::find( EntityID id ) const
{
	Entity * pEntity = CellApp::instance().findEntity( id );
	return pEntity ? this->find( *pEntity ) : NULL;
}

/**
 *	Write out all our entries to the given stream.
 *	Our size has already been written out.
 */
void EntityCacheMap::writeToStream( BinaryOStream & stream ) const
{
	Implementor::iterator it = set_.begin();
	Implementor::iterator nd = set_.end();
	for (; it != nd; ++it)
	{
		stream << *it;
	}
}


/**
 *	This method is used to visit each EntityCache in this map.
 */
void EntityCacheMap::visit( EntityCacheVisitor & visitor ) const
{
	Implementor::iterator iter = set_.begin();

	while (iter != set_.end())
	{
		visitor.visit( *iter );

		++iter;
	}
}


/**
 *	This method is used to mutate each EntityCache in this map.
 */
void EntityCacheMap::mutate( EntityCacheMutator & mutator )
{
	Implementor::iterator iter = set_.begin();

	while (iter != set_.end())
	{
		EntityConstPtr pEntity = iter->pEntity();

		EntityCache & rCache = const_cast< EntityCache & >( *iter );
		mutator.mutate( rCache );

		// Ensure that mutator didn't change iter->pEntity
		MF_ASSERT( pEntity == iter->pEntity() );

		++iter;
	}
}


/**
 *	This static method adds the watchers associated with this class.
 */
void EntityCacheMap::addWatchers()
{
	MF_WATCH( "stats/numInAoI", g_numInAoI );
	MF_WATCH( "stats/totalInAoIEver", g_numInAoIEver );
}


// -----------------------------------------------------------------------------
// Section: EntityCache
// -----------------------------------------------------------------------------

/**
 *	This method is used to update the detail level associated with this cache.
 *	If the detail is increased, the necessary information is added to the input
 *	bundle that will be sent to the viewing client.
 *
 *	@return	True if we added data to the bundle, False otherwise.
 */
bool EntityCache::updateDetailLevel( Mercury::Bundle & bundle,
	float lodPriority, bool hasSelectedEntity )
{
	bool hasWrittenToStream = false;

	// Update the LoD level and add any necessary info to the bundle.
	const EntityDescription & entityDesc =
		this->pEntity()->pType()->description();
	const DataLoDLevels & lodLevels = entityDesc.lodLevels();

	while (lodLevels.needsMoreDetail( detailLevel_, lodPriority ))
	{
		detailLevel_--;

		hasWrittenToStream |= this->addChangedProperties( bundle, &bundle,
			/* shouldSelectEntity */ !hasSelectedEntity );

		hasSelectedEntity |= hasWrittenToStream;
	}

	while (lodLevels.needsLessDetail( detailLevel_, lodPriority ))
	{
		this->lodEventNumber( detailLevel_, this->lastEventNumber() );

		detailLevel_++;
	}

	return hasWrittenToStream;
}


/**
 *	This method adds the new property values of the current detail level to the
 *	input stream.
 *
 *	@param stream The stream to add the changes to.
 *	@param pBundleForHeader If not NULL, a message is started on this bundle
 *		if any properties are added.
 *	@param shouldSelectEntity If true, before a new message is started on the
 *
 *	@return	True if we added any property values, false if there were no new
 *			property values at this level of detail.
 */
bool EntityCache::addChangedProperties( BinaryOStream & stream,
		Mercury::Bundle * pBundleForHeader, bool shouldSelectEntity )
{
	const EntityDescription & entityDesc =
		this->pEntity()->pType()->description();
	int numProperties = entityDesc.clientServerPropertyCount();
	EventNumber lodEventNumber = this->lodEventNumber( detailLevel_ );

	int numToSend = 0;

	for (int i = 0; i < numProperties; i++)
	{
		const DataDescription * pDataDesc =
			entityDesc.clientServerProperty( i );

		if (pDataDesc->detailLevel() == detailLevel_ &&
				this->pEntity()->propertyEventStamps().get( *pDataDesc ) >
					lodEventNumber )
		{
			++numToSend;
		}
	}

	if (numToSend == 0)
	{
		return false;
	}

	MF_ASSERT( numToSend < 256 );

	if (pBundleForHeader != NULL)
	{
		if (shouldSelectEntity)
		{
			this->addEntitySelectMessage( *pBundleForHeader );
		}
		pBundleForHeader->startMessage( BaseAppIntInterface::updateEntity );
	}


	stream << uint8( numToSend );

	for (int i = 0; i < numProperties; i++)
	{
		const DataDescription * pDataDesc =
			entityDesc.clientServerProperty( i );

		if (pDataDesc->detailLevel() == detailLevel_ &&
				this->pEntity()->propertyEventStamps().get( *pDataDesc ) >
					lodEventNumber )
		{
			ScriptObject pValue = this->pEntity()->propertyByDataDescription(
				pDataDesc );

			if (!pValue)
			{
				CRITICAL_MSG( "EntityCache::addChangedProperties: "
						"Could not get attribute %s when increasing the detail level of %u.\n",
					pDataDesc->name().c_str(),
					this->pEntity()->id() );
				ScriptDataSink sink;
				MF_VERIFY( pDataDesc->dataType()->getDefaultValue( sink ) );
				pValue = sink.finalise();
			}

			--numToSend;
			stream << uint8(i);

			ScriptDataSource source( pValue );
			pDataDesc->addToStream( source, stream,
				/* isPersistentOnly */ false );
		}
	}

	MF_ASSERT( numToSend == 0 );

	return true;
}


/**
 *	This method adds the properties that have changed in the outer detail level
 *	to the input stream.
 */
void EntityCache::addOuterDetailLevel( BinaryOStream & stream )
{
	MF_ASSERT( detailLevel_ == this->numLoDLevels() );
	detailLevel_ = this->numLoDLevels() - 1;
	lastEventNumber_ = this->pEntity()->lastEventNumber();

	this->addChangedProperties( stream );
}


/**
 *	This method writes a message to the given bundle indicating which entity
 *	the following history events are destined for.
 *
 *	@param bundle	The bundle to put the information on.
 */
void EntityCache::addEntitySelectMessage( Mercury::Bundle & bundle ) const
{
	MF_ASSERT( this->pEntity() != NULL );
	MF_ASSERT( this->isUpdatable() );

	const bool hasAlias = (this->idAlias() != NO_ID_ALIAS);

	if (hasAlias)
	{
		BaseAppIntInterface::selectAliasedEntityArgs::start(
				bundle ).idAlias = this->idAlias();
	}
	else
	{
		BaseAppIntInterface::selectEntityArgs::start( bundle ).id =
			this->pEntity()->id();
	}
}


/**
 *	This method adds a leaveAoI message to the input bundle.
 *
 *	This method is safe to call on a dummy cache
 *
 *	@param bundle	The bundle to add the message to.
 *	@param id	The ID of pEntity(), or if pEntity() is NULL,
 *					the ID of the entity this cache referred to
 */
void EntityCache::addLeaveAoIMessage( Mercury::Bundle & bundle,
	   EntityID id ) const
{
	MF_ASSERT( !this->pEntity() || this->pEntity()->id() == id );

	// The leaveAoI message contains the id of the entity leaving the AoI and
	// then the sequence of event numbers that are the LoD level stamps.

	// TODO: At the moment, we send all of the stamps but we only really need to
	// send stamps for those levels that we actually entered.

	bundle.startMessage( BaseAppIntInterface::leaveAoI );
	bundle << id;

	if (this->pEntity())
	{
		const int size = this->numLoDLevels();

		// For detail levels higher (more detailed) than the current one, we
		// send the stored value.
		for (int i = 0; i < detailLevel_; i++)
		{
			bundle << lodEventNumbers_[i];
		}

		// For the current and lower detail levels, we know that they are all at
		// the current event number. (Only the current one is set and the others
		// are implied).
		for (int i = detailLevel_; i < size; i++)
		{
			bundle << this->lastEventNumber();
		}
	}
}


/**
 *	This method is called if an entity is reintroduced to an AoI before it has
 *	had a chance to be removed.
 */
void EntityCache::reuse()
{
	MF_ASSERT( this->isGone() );
	this->clearGone();

	if (this->lastEventNumber() < pEntity_->eventHistory().lastTrimmedEventNumber()
		&& this->isUpdatable())
	{
		// In this case, we have missed events. Refresh the entity
		// properties by throwing it out of the AoI and make it re-enter
		// immediately after.
		INFO_MSG( "EntityCache::reuse: Client has last received event %d, "
				"entity is at event %d but only has history since event %d. "
				"Not reusing cache.\n",
			this->lastEventNumber(),
			pEntity_->lastEventNumber(),
			pEntity_->eventHistory().lastTrimmedEventNumber() );

		this->setRefresh();
	}
}


/**
 *	This method sets the event numbers associated with some cache levels. The
 *	input is an array of 'size' event numbers. These are used to set the top
 *	'size' cache event numbers.
 */
void EntityCache::lodEventNumbers( EventNumber * pEventNumbers, int size )
{
	const int offset = this->numLoDLevels() - size;

	MF_ASSERT( offset >= 0 );

	for (int i = 0; i < size; i++)
	{
		lodEventNumbers_[i + offset] = pEventNumbers[i];
	}
}


// -----------------------------------------------------------------------------
// Section: Streaming operators
// -----------------------------------------------------------------------------

/**
 *	Streaming operator for EntityCache.
 */
BinaryIStream & operator>>( BinaryIStream & stream,
		EntityCache & entityCache )
{
	stream >>
		entityCache.flags_ >>
		entityCache.updateSchemeID_ >>
		entityCache.vehicleChangeNum_ >>
		entityCache.priority_ >>
		entityCache.lastEventNumber_ >>
		entityCache.idAlias_ >>
		entityCache.detailLevel_;

	uint8 cacheSize;
	stream >> cacheSize;

	MF_ASSERT( !entityCache.pEntity() ||
			entityCache.numLoDLevels() == cacheSize );

	for (int i = 0; i < cacheSize - 1; i++)
	{
		stream >> entityCache.lodEventNumbers_[i];
	}

	return stream;
}


/**
 *	Streaming operator for EntityCache.
 */
BinaryOStream & operator<<( BinaryOStream & stream,
		const EntityCache & entityCache )
{
	EntityConstPtr pEntity = entityCache.pEntity();
	stream << pEntity->id();

	EntityCache::VehicleChangeNum vehicleChangeNumState =
		EntityCache::VEHICLE_CHANGE_NUM_OLD;

	if (pEntity->vehicleChangeNum() == entityCache.vehicleChangeNum())
	{
		vehicleChangeNumState =
			pEntity->pVehicle() ?
				EntityCache::VEHICLE_CHANGE_NUM_HAS_VEHICLE :
				EntityCache::VEHICLE_CHANGE_NUM_HAS_NO_VEHICLE;
	}

	stream <<
		entityCache.flags_ <<
		entityCache.updateSchemeID_ <<
		// Stream on the state of the vehicleChangeNum_ instead of the current
		// value. This is used to set an appropriate value on the reading side.
		vehicleChangeNumState << // entityCache.vehicleChangeNum_ <<
		// NOTE: It's been explicitly decided it's not worthwhile streaming
		// lastPriorityDelta_. This could cause a rapid priority increase if an
		// offload occurs while adjusting to a large priority delta change.
		entityCache.priority_ <<
		entityCache.lastEventNumber_ <<
		entityCache.idAlias_ <<
		entityCache.detailLevel_;

	// Only have to stream on the size because if the receiving cell does not
	// have this entity, it does not know the type of the entity and so does
	// not know the cache size.
	// TODO: Could add the type instead.

	int size = entityCache.numLoDLevels();

	stream << uint8(size);

	for (int i = 0; i < size - 1; i++)
	{
		stream << entityCache.lodEventNumbers_[i];
	}

	if (vehicleChangeNumState == EntityCache::VEHICLE_CHANGE_NUM_HAS_VEHICLE)
	{
		// This is read off in Witness::Witness.
		stream << pEntity->pVehicle()->id();
	}

	return stream;
}

BW_END_NAMESPACE

// entity_cache.cpp

