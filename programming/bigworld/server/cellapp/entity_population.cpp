#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "entity_population.hpp"

#ifndef CODE_INLINE
#include "entity_population.ipp"
#endif

#include "cellapp.hpp"
#include "entity.hpp"
#include "cell_app_channel.hpp"
#include "baseapp/baseapp_int_interface.hpp"
#include "space.hpp"

#include "network/bundle.hpp"
#include "network/bundle_sending_map.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityPopulation
// -----------------------------------------------------------------------------

/**
 *	This method adds an entity to the population.
 */
void EntityPopulation::add( Entity & entity )
{
	// It used to be the case that notifyObservers was called in here but it
	// was too early. We don't really need the EntityPopulation to be associated
	// with the observers.

	// Add ourselves to the population
	MF_VERIFY( this->insert( std::make_pair( entity.id(), &entity ) ).second );

	// Make sure that it is not in the cached real address maps.
	this->forgetRealChannel( entity.id() );
}


/**
 *	This method removes an entity from the population.
 */
void EntityPopulation::remove( Entity & entity )
{
	MF_VERIFY( this->erase( entity.id() ) != 0 );
}


/**
 *	This method adds an observer on the population. This is an object that wants
 *	to be told be a particular entity joins the population.
 *
 *	@param id			The id of the entity to watch out for.
 *	@param pObserver	The object that will be told when the entity arrives.
 *
 *	@note	Once the observer has been told that the entity has arrived, it is
 *			removed from the list of observers.
 */
void EntityPopulation::addObserver( EntityID id, Observer * pObserver ) const
{
	observers_.insert( std::make_pair( id, pObserver ) );
}


/**
 *	This method removes an observer on the population.
 *
 *	@param id			The id of the entity to watch out for.
 *	@param pObserver	The object that will be told when the entity arrives.
 */
bool EntityPopulation::removeObserver( EntityID id, Observer * pObserver ) const
{
	typedef Observers::iterator Iter;
	std::pair<Iter, Iter> range = observers_.equal_range( id );

	for (Iter iter = range.first; iter != range.second; ++iter)
	{
		if (iter->second == pObserver)
		{
			observers_.erase( iter );

			return true;
		}
	}

	WARNING_MSG( "EntityPopulation::removeObserver: "
						"Unable to remove for %u\n", id );

	return false;
}


/**
 *	This method notifies observers of a new entity.
 */
void EntityPopulation::notifyObservers( Entity & entity ) const
{
	typedef Observers::const_iterator Iter;
	std::pair<Iter, Iter> range = observers_.equal_range( entity.id() );

	if (range.first != range.second)
	{
		for (Iter iter = range.first; iter != range.second; ++iter)
		{
			iter->second->onEntityAdded( entity );
		}
		observers_.erase( entity.id() );
	}
}


// -----------------------------------------------------------------------------
// Section: RealAddress
// -----------------------------------------------------------------------------

/**
 *	This method is called when a ghost entity is destroyed. It remembers where
 *	it thought the real entity was. This is used if a message arrives for this
 *	entity. The message will be forwarded to this address.
 */
void EntityPopulation::rememberRealChannel( EntityID id,
	CellAppChannel & channel )
{
	currChannels_[ id ] = &channel;
}


/**
 *	This method adds the baseapp channel from a recently offloaded real
 */
void EntityPopulation::rememberBaseChannel( Entity & entity, const Mercury::Address & addr ) const
{
	if (entity.baseAddr() != Mercury::Address::NONE)
	{
		RecentTeleportData newData = {entity.baseAddr(), entity.space().id(), 
							entity.id()};
		currRecentTeleportData_.insert( std::pair< Mercury::Address, RecentTeleportData > ( addr, newData ) );
	}
}


/**
 *	This method frees the cached real addresses. It should be called
 *	periodically to allow freeing of the associated memory.
 */
void EntityPopulation::expireRealChannels() const
{
	prevChannels_.clear();
	currChannels_.swap( prevChannels_ );

	prevRecentTeleportData_.clear();
	currRecentTeleportData_.swap( prevRecentTeleportData_ );
}


/**
 *	This method attempts to find a possible location for entity with the input
 *	id.
 */
CellAppChannel * EntityPopulation::findRealChannel( EntityID id ) const
{
	RealChannels::iterator iter = currChannels_.find( id );

	if (iter != currChannels_.end())
	{
		return iter->second;
	}

	iter = prevChannels_.find( id );

	if (iter != prevChannels_.end())
	{
		return iter->second;
	}

	return NULL;
}


/**
 *	This method informs base entities that their cell entity may be on a dead
 *	CellApp. Entities that have recently teleported to the dead CellApp inform
 *	the base entity that this is where they believe the real cell entity is. If
 *	the base entity thought the cell entity was still here, it believes this
 *	message and updates the location of the real.
 */
void EntityPopulation::notifyBaseRangeOfCellAppDeath( 
	BaseAddrs & container,
	const Mercury::Address & addr,
	Mercury::BundleSendingMap & bundles,
	AckCellAppDeathHelper * pAckHelper) const
{
	std::pair<BaseAddrs::iterator, BaseAddrs::iterator> range = 
		container.equal_range( addr );
	BaseAddrs::iterator iter;
	for (iter = range.first; iter != range.second; iter++)
	{
		EntityID entityID = iter->second.entityID;

		if ( this->find( entityID ) != this->end() )
		{
			continue;
		}

		CellAppChannel * pCellChannel = this->findRealChannel( entityID );
		if (pCellChannel && pCellChannel->addr() == addr)
		{
			Mercury::Bundle & bundle = bundles[ iter->second.baseAddr ];

			BaseAppIntInterface::setClientArgs setClientArgs =
				{ entityID };
			bundle << setClientArgs;

			bundle.startRequest( BaseAppIntInterface::emergencySetCurrentCell,
								 pAckHelper,
								 reinterpret_cast< void * >( entityID ));

			bundle << iter->second.spaceID;
			bundle << addr;

			// Remember that we're expecting another reply.
			pAckHelper->addBadGhost();
		}
	}
}


/**
 *	This method is called on CellApp death.  It begins where handleCellAppDeath
 *	left off, any recently offloaded entities who's ghosts have already been 
 *	destroyed (probably through teleport) will 
 */
void EntityPopulation::notifyBasesOfCellAppDeath( 
	const Mercury::Address & addr,
	Mercury::BundleSendingMap & bundles,
	AckCellAppDeathHelper * pAckHelper ) const
{
	// Iterate through both lists
	this->notifyBaseRangeOfCellAppDeath(prevRecentTeleportData_, addr, bundles,
										pAckHelper);
	this->notifyBaseRangeOfCellAppDeath(currRecentTeleportData_, addr, bundles,
										pAckHelper);
}


/**
 *  This method is called on CellApp death.  We remove any reference to the
 *  CellAppChannel to the dead app.
 */
void EntityPopulation::handleCellAppDeath( const Mercury::Address & addr )
{
	this->forgetRealChannel( currChannels_, addr );
	this->forgetRealChannel( prevChannels_, addr );
}


/**
 *  This method will remove all mappings from the given collection that refer to
 *  the dead channel.
 */
void EntityPopulation::forgetRealChannel( RealChannels & channels,
	const Mercury::Address & addr )
{
	RealChannels::iterator iter = channels.begin();

	while (iter != channels.end())
	{
		RealChannels::iterator oldIter = iter++;
		CellAppChannel * pChannel = oldIter->second;

		if (pChannel->addr() == addr)
		{
			channels.erase( oldIter );
		}
	}
}

BW_END_NAMESPACE

// entity_population.cpp
