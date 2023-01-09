#include "pch.hpp"

#include "space_manager.hpp"
#include "client_space.hpp"
#include "client_space_factory.hpp"
#include "entity_embodiment.hpp"

// TODO: Remove this dependency
// By moving space data mappings into Space
#include "network/space_data_mappings.hpp"

#include "romp/py_omni_light.hpp"
#include "romp/py_spot_light.hpp"

BW_BEGIN_NAMESPACE

SpaceManager & SpaceManager::instance()
{
	static SpaceManager spaceManager;
	return spaceManager;
}


SpaceManager::SpaceManager()
{

}


SpaceManager::~SpaceManager()
{
}


void SpaceManager::init()
{

}


/**
 *	This method finalises the SpaceManager by clearing all spaces.
 */
void SpaceManager::fini()
{
	if (!spaces_.empty())
	{
		WARNING_MSG( "SpaceManager::fini: Spaces not cleared\n" );
	}
	this->clearSpaces();

	if (!localSpaces_.empty())
	{
		WARNING_MSG( "SpaceManager::fini: Local spaces not cleared\n" );
	}
	this->clearLocalSpaces();

	pFactory_.reset();
}


bool SpaceManager::isReady()
{
	return factory() != NULL;
}


void SpaceManager::factory( IClientSpaceFactory * pFactory )
{
	MF_ASSERT( pFactory != NULL );
	pFactory_.reset( pFactory );
}


IClientSpaceFactory * SpaceManager::factory()
{
	return pFactory_.get();
}


void SpaceManager::tick( float dTime )
{
	for (SpaceMap::iterator it = spaces_.begin(); 
		it != spaces_.end(); ++it)
	{
		it->second->tick( dTime );
	}

	for (SpaceMap::iterator it = localSpaces_.begin(); 
		it != localSpaces_.end(); ++it)
	{
		it->second->tick( dTime );
	}
}


void SpaceManager::updateAnimations( float dTime )
{
	for (SpaceMap::iterator it = spaces_.begin(); 
		it != spaces_.end(); ++it)
	{
		it->second->updateAnimations( dTime );
	}

	for (SpaceMap::iterator it = localSpaces_.begin(); 
		it != localSpaces_.end(); ++it)
	{
		it->second->updateAnimations( dTime );
	}
}


void SpaceManager::addSpace( ClientSpace * pSpace )
{
	if (space( pSpace->id() ) != NULL)
	{
		return;
	}

	SpaceMap & map =
		pSpace->id() >= LOCAL_SPACE_START ? localSpaces_ : spaces_;

	map.insert( std::make_pair( pSpace->id(), pSpace ) );

	onSpaceCreated.invoke( this, pSpace );
}


void SpaceManager::delSpace( ClientSpace * pSpace )
{
	onSpaceDestroyed.invoke( this, pSpace );

	SpaceMap & map =
		pSpace->id() >= LOCAL_SPACE_START ? localSpaces_ : spaces_;

	SpaceMap::iterator found = map.find( pSpace->id() );

	MF_ASSERT_DEV( found != map.end() );
	MF_ASSERT_DEV( pSpace->refCount() == 0 );

	if( found != map.end() )
	{
		map.erase( found );
	}
}


ClientSpace * SpaceManager::space( SpaceID spaceID )
{
	SpaceMap & map = spaceID >= LOCAL_SPACE_START ? localSpaces_ : spaces_;

	SpaceMap::iterator it = map.find( spaceID );
	if (it != map.end())
	{
		return it->second;
	}
	else
	{
		return NULL;
	}
}


ClientSpace * SpaceManager::createSpace( SpaceID spaceID )
{
	MF_ASSERT( space( spaceID ) == NULL );

	if ( spaceID == NULL_SPACE_ID )
	{
		ERROR_MSG("Somebody tried to create space with NULL Space ID %d\n",
			NULL_SPACE_ID );
		return NULL;
	}

	MF_ASSERT( pFactory_.get() );

	// Create the space
	ClientSpace * pSpace = pFactory_->createSpace( spaceID );
	MF_ASSERT( pSpace );

	addSpace( pSpace );
	return pSpace;
}


ClientSpace * SpaceManager::findOrCreateSpace( SpaceID spaceID )
{
	ClientSpace * pSpace = space( spaceID );
	if (pSpace)
	{
		return pSpace;
	}
	else
	{
		return createSpace( spaceID );	
	}
}


/**
 *	This method clears all non local spaces.
 */
void SpaceManager::clearSpaces()
{
	this->clearSpaceMap( spaces_ );
}


/**
 *	This method clears all local spaces.
 */
void SpaceManager::clearLocalSpaces()
{
	this->clearSpaceMap( localSpaces_ );
}


/**
 *	This method clears and deletes elements of a SpaceMap.
 */
void SpaceManager::clearSpaceMap( SpaceMap & map )
{
	if (map.empty())
	{
		return;
	}

	// Generate a list of spaces to remove first.
	// Then remove them.
	size_t numSpaces = map.size();
	size_t i = 0;
	SpaceID* spaceIDs = new SpaceID[ numSpaces ];
	for (SpaceMap::iterator it = map.begin(); it != map.end(); it++)
	{
		spaceIDs[i++] = it->first;
	}

	for (i = 0; i < numSpaces; i++)
	{
		map[spaceIDs[i]]->clear();
	}

	bw_safe_delete_array(spaceIDs);
}


IEntityEmbodimentPtr SpaceManager::createEntityEmbodiment( 
	const ScriptObject& object )
{
	IClientSpaceFactory* pFactory = this->factory();
	MF_ASSERT( pFactory );

	return pFactory->createEntityEmbodiment( object );
}


IOmniLightEmbodiment * SpaceManager::createOmniLightEmbodiment(
	const PyOmniLight & pyOmniLight )
{
	IClientSpaceFactory* pFactory = this->factory();
	MF_ASSERT( pFactory );

	return pFactory->createOmniLightEmbodiment( pyOmniLight );
}


ISpotLightEmbodiment * SpaceManager::createSpotLightEmbodiment(
	const PySpotLight & pySpotLight )
{
	IClientSpaceFactory* pFactory = this->factory();
	MF_ASSERT( pFactory );

	return pFactory->createSpotLightEmbodiment( pySpotLight );
}


SpaceManager::SpaceCreatedEvent::EventDelegateList& 
	SpaceManager::spaceCreated()
{
	return onSpaceCreated.delegates();
}


SpaceManager::SpaceDestroyedEvent::EventDelegateList& 
	SpaceManager::spaceDestroyed()
{
	return onSpaceDestroyed.delegates();
}


bool SpaceManager::isLocalSpace( SpaceID spaceID )
{
	return spaceID >= SpaceManager::LOCAL_SPACE_START;
}

BW_END_NAMESPACE
