#ifndef COMPILED_SPACE_ENTITY_UDO_FACTORY_HPP
#define COMPILED_SPACE_ENTITY_UDO_FACTORY_HPP

#include "space/space_lib.hpp"
#include "resmgr/datasection.hpp"

namespace BW {
namespace CompiledSpace {

class IEntityUDOFactory
{
public:
	typedef int32 EntityID;
	typedef uintptr UDOHandle;

public:
	virtual ~IEntityUDOFactory() {}

	virtual EntityID createEntity( SpaceID spaceID, const DataSectionPtr& ds ) = 0;
	virtual void destroyEntity( EntityID entityID ) = 0;

	virtual UDOHandle createUDO( const DataSectionPtr& ds ) = 0;
	virtual void destroyUDO( UDOHandle udo ) = 0;
};

} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_ENTITY_UDO_FACTORY_HPP
