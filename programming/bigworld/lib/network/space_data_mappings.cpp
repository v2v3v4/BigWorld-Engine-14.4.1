#include "pch.hpp"

#include "space_data_mappings.hpp"

#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

int SpaceDataMappings_Token = 0;

/**
 *	This method registers a spaceID and creates a data mapping for it.
 */
bool SpaceDataMappings::addSpace( SpaceID spaceID, bool isLocal /* = false */ )
{
	BW_GUARD;

	if (spaceMap_.find( spaceID ) != spaceMap_.end())
	{
		return false;
	}
	spaceMap_[ spaceID ] = SpaceMappingEntry( SpaceDataMapping(), isLocal );
	return true;
}


/**
 *	This method deregisters a spaceID
 */
bool SpaceDataMappings::delSpace( SpaceID spaceID )
{
	BW_GUARD;

	SpaceMap::iterator it = spaceMap_.find( spaceID );
	if (it == spaceMap_.end())
	{
		return false;
	}
	spaceMap_.erase( it );
	return true;
}


SpaceDataMapping * SpaceDataMappings::mapping( SpaceID spaceID )
{
	SpaceMap::iterator it = spaceMap_.find( spaceID );
	if (it == spaceMap_.end())
	{
		return NULL;
	}
	return &it->second.mapping_;
}


void SpaceDataMappings::clear( bool remoteOnly )
{
	for (SpaceMap::iterator it = spaceMap_.begin();
			it != spaceMap_.end();)
	{
		if (!remoteOnly || !it->second.isLocal_)
		{
			SpaceMap::iterator delIt = it++;
			spaceMap_.erase( delIt );
		}
		else
		{
			++it;
		}
	}
}

BW_END_NAMESPACE

// space_data_mappings.cpp
