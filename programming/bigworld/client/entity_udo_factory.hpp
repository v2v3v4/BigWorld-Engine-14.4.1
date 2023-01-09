#ifndef CLIENT_ENTITY_UDO_FACTORY_HPP
#define CLIENT_ENTITY_UDO_FACTORY_HPP

#include "compiled_space/entity_udo_factory.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class Entity;
class EntityType;

class EntityUDOFactory : public CompiledSpace::IEntityUDOFactory
{
public:
	static Entity* createEntityWithType( SpaceID spaceID,
		const EntityType* pType, const Matrix& transform,
		const DataSectionPtr &pPropertiesDS );
	
	static void destroyEntityByID( EntityID id );
	static void destroyBWEntity( BWEntity * pEntity );

	
	// IEntityUDOFactory interface
	virtual EntityID createEntity( SpaceID spaceID, const DataSectionPtr& ds );
	virtual void destroyEntity( EntityID id );

	virtual UDOHandle createUDO( const DataSectionPtr& ds );
	virtual void destroyUDO( UDOHandle udo );

private:
	struct UDOHolder;
};

BW_END_NAMESPACE

#endif // CLIENT_ENTITY_UDO_FACTORY_HPP
