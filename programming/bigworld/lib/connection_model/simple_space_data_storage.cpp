#include "pch.hpp"

#include "simple_space_data_storage.hpp"

#include "network/space_data_mappings.hpp"

BW_BEGIN_NAMESPACE

int SimpleSpaceDataStorage_Token = 0;

/**
 *	Constructor
 */
 SimpleSpaceDataStorage::SimpleSpaceDataStorage(
		SpaceDataMappingsPtr pMappings ) :
	pMappings_( pMappings )
{
}

/**
 *	This virtual method prepares storage for space data for the given
 *	SpaceID.
 *
 *	@param spaceID			The SpaceID to prepare storage for
 *
 *	@return	true unless the space was already known, in which case it may
 *			have stale data present.
 */
bool SimpleSpaceDataStorage::addMapping( SpaceID spaceID )
{
	if (pMappings_->mapping( spaceID ) != NULL)
	{
		return false;
	}
	pMappings_->addSpace( spaceID, /* isLocal */ false );
	return true;
}


/**
 *	This virtual method forgets all the data for the given SpaceID.
 *
 *	@param spaceID			The SpaceID to forget the data for
 */
void SimpleSpaceDataStorage::clearMapping( SpaceID spaceID )
{
	// Ignore failure to clear, may already have been cleared through
	// SimpleSpaceDataStorage::clearAllMappings()
	pMappings_->delSpace( spaceID );
}


/**
 *	This virtual method retrieves a SpaceDataMapping for the given spaceID.
 *
 *	@param spaceID			The SpaceID to retrieve the mapping for.
 *
 *	@return	A SpaceDataMapping pointer, or NULL if no mapping exists for this
 *			SpaceID.
 */
SpaceDataMapping * SimpleSpaceDataStorage::getMapping( SpaceID spaceID ) const
{
	return pMappings_->mapping( spaceID );
}


/**
 *	This virtual method clears all mappings managed through this interface.
 */
void SimpleSpaceDataStorage::clearAllMappings()
{
	pMappings_->clear( /* remoteOnly */ true );
}


BW_END_NAMESPACE

// simple_space_data_storage.cpp
