#ifndef SPACE_DATA_STORAGE_HPP
#define SPACE_DATA_STORAGE_HPP

#include "space/client_space.hpp"

#include "connection_model/bw_space_data_storage.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/smartpointer.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

typedef BW::map< SpaceID, ClientSpacePtr > ClientSpaceMap;

/**
 *	This class is an implementation of BWSpaceDataStorage to wrap the
 *	static SpaceManager.
 */
class SpaceDataStorage : public BWSpaceDataStorage
{
private:
	// BWSpaceDataStorage implementation
	bool addMapping( SpaceID spaceID );
	void clearMapping( SpaceID spaceID );
	SpaceDataMapping * getMapping( SpaceID spaceID ) const;
	void clearAllMappings();

	ClientSpaceMap knownSpaces_;
};

BW_END_NAMESPACE

#endif // SPACE_DATA_STORAGE_HPP
