#ifndef SPACE_DATA_STORAGE_HPP
#define SPACE_DATA_STORAGE_HPP

#include "bwentity/bwentity_api.hpp"

#include "bw_space_data_storage.hpp"

#include "cstdmf/smartpointer.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class SpaceDataMappings;
typedef SmartPointer< SpaceDataMappings > SpaceDataMappingsPtr;

/**
 *	This class wraps a SpaceDataMappings instance for the BWSpaceDataStorage
 *	interface.
 */
class SimpleSpaceDataStorage : public BWSpaceDataStorage
{
public:
	BWENTITY_API SimpleSpaceDataStorage( SpaceDataMappingsPtr pMappings );
	BWENTITY_API virtual ~SimpleSpaceDataStorage() {};
private:
	// BWSpaceDataStorage implementation
	BWENTITY_API virtual bool addMapping( SpaceID spaceID );
	BWENTITY_API virtual void clearMapping( SpaceID spaceID );
	BWENTITY_API virtual SpaceDataMapping * getMapping( SpaceID spaceID ) const;
	BWENTITY_API virtual void clearAllMappings();

	SpaceDataMappingsPtr pMappings_;
};

BW_END_NAMESPACE

#endif // SPACE_DATA_STORAGE_HPP
