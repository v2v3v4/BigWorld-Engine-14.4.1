#include "pch.hpp"

#include "space_data_mapping.hpp"

#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method clears all entries
 */
void SpaceDataMapping::clear()
{
	dataEntries_.clear();
}

/**
 *	This method adds a data entry and corresponding data
 */
 bool SpaceDataMapping::addDataEntry( const SpaceEntryID & spaceEntryID,
										uint16 key,
										const BW::string & data )
{
	BW_GUARD;
	if (dataEntries_.find( spaceEntryID ) != dataEntries_.end())
	{
		return false;
	}

	// ok it's novel, so add it
	dataEntries_[spaceEntryID] = DataValue( key, data );
	return true;
}


/**
 *	This method deletes a data entry by key
 */
bool SpaceDataMapping::delDataEntry( const SpaceEntryID & spaceEntryID )
{
	BW_GUARD;
	size_t numErased = dataEntries_.erase( spaceEntryID );
	return numErased > 0;
}


/**
 *	This method deletes a data entry by key and returns the old value
 */
bool SpaceDataMapping::delDataEntry( const SpaceEntryID & spaceEntryID,
		DataValue & oldValue )
{
	DataEntryMap::iterator it;
	it = dataEntries_.find( spaceEntryID );
	if (it == dataEntries_.end())
	{
		oldValue = DataValue();
		return false;
	}
	oldValue = it->second;
	dataEntries_.erase( it );
	return true;
}


/**
 *	Retrieve the given exact entry.
 *	Return a not valid DataValue if not found.
 */
const SpaceDataMapping::DataValue SpaceDataMapping::dataRetrieveSpecific(
	const SpaceEntryID & spaceEntryID ) const
{
	BW_GUARD;
	const_iterator it;
	it = dataEntries_.find( spaceEntryID );
	if (it == dataEntries_.end())
	{
		return DataValue();
	}
	return it->second;
}


/**
 *	Retrieve the first data entry for that key in this chunk space.
 */
const BW::string * SpaceDataMapping::dataRetrieveFirst( uint16 key ) const
{
	BW_GUARD;
	const_iterator it;
	for (it = dataEntries_.begin(); it != dataEntries_.end(); ++it)
	{
		if (it->second.key() == key)
		{
			return &it->second.data();
		}
	}
	return NULL;
}

BW_END_NAMESPACE

// space_data_mapping.cpp
