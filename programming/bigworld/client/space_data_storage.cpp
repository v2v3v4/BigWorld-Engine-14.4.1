#include "pch.hpp"

#include "entity_manager.hpp"
#include "space_data_storage.hpp"

#include "space/client_space.hpp"
#include "space/space_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This virtual method prepares storage for space data for the given
 *	SpaceID.
 *
 *	@param spaceID			The SpaceID to prepare storage for
 *
 *	@return	true unless the space was already known, in which case it may
 *			have stale data present.
 */
bool SpaceDataStorage::addMapping( SpaceID spaceID )
{
	if (knownSpaces_.find( spaceID ) != knownSpaces_.end())
	{
		return false;
	}

	knownSpaces_[ spaceID ] = SpaceManager::instance().createSpace( spaceID );

	return true;
}


/**
 *	This virtual method forgets all the data for the given SpaceID.
 *
 *	@param spaceID			The SpaceID to forget the data for
 */
void SpaceDataStorage::clearMapping( SpaceID spaceID )
{
	ClientSpaceMap::iterator iter = knownSpaces_.find( spaceID );

	if (iter == knownSpaces_.end())
	{
		return;
	}

	iter->second->clear();
	knownSpaces_.erase( iter );
}


/**
 *	This virtual method retrieves a SpaceDataMapping for the given spaceID.
 *
 *	@param spaceID			The SpaceID to retrieve the mapping for.
 *
 *	@return	A SpaceDataMapping pointer, or NULL if no mapping exists for this
 *			SpaceID.
 */
SpaceDataMapping * SpaceDataStorage::getMapping( SpaceID spaceID ) const
{
	ClientSpaceMap::const_iterator iClientSpaces = knownSpaces_.find( spaceID );
	if (iClientSpaces == knownSpaces_.end())
	{
		return NULL;
	}

	return &(iClientSpaces->second->spaceData());
}


/**
 *	This virtual method clears all mappings managed through this interface.
 */
void SpaceDataStorage::clearAllMappings()
{
	ClientSpaceMap::iterator iter = knownSpaces_.begin();

	while (iter != knownSpaces_.end())
	{
		iter->second->clear();
		++iter;
	}

	knownSpaces_.clear();
}



BW_END_NAMESPACE

// space_data_storage.cpp
