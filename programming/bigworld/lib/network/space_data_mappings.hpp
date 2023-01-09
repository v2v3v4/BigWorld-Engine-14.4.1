#ifndef SPACE_DATA_MAPPINGS_HPP
#define SPACE_DATA_MAPPINGS_HPP

#include "cstdmf/bw_safe_allocatable.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/smartpointer.hpp"

#include "bwentity/bwentity_api.hpp"

#include "network/space_data_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class represents a collection of SpaceDataMappings
 */
class SpaceDataMappings : public SafeReferenceCount, public SafeAllocatable
{
public:
	BWENTITY_API SpaceDataMappings() {}

	BWENTITY_API void clear( bool remoteOnly );

	BWENTITY_API bool addSpace( SpaceID spaceID, bool isLocal = false );
	BWENTITY_API bool delSpace( SpaceID spaceID );
	
	BWENTITY_API SpaceDataMapping * mapping( SpaceID spaceID );

private:
	SpaceDataMappings( SpaceDataMappings & );
	void operator=( SpaceDataMappings & );

	class SpaceMappingEntry
	{
	public:
		SpaceMappingEntry() : isLocal_( false ) {}

		SpaceMappingEntry( const SpaceDataMapping & mapping, bool isLocal ) :
			mapping_( mapping ), isLocal_( isLocal ) {}

		SpaceDataMapping mapping_;
		bool isLocal_;
	};

	typedef BW::map< SpaceID, SpaceMappingEntry > SpaceMap;
	SpaceMap spaceMap_;
};

typedef SmartPointer< SpaceDataMappings >  SpaceDataMappingsPtr;

BW_END_NAMESPACE

#endif // SPACE_DATA_MAPPINGS_HPP
