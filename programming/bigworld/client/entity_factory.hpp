#ifndef ENTITY_FACTORY_HPP
#define ENTITY_FACTORY_HPP

#include "connection_model/bw_entity_factory.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class BWConnection;

/**
 *	This class is an implementation of BWEntityFactory to produce Entity
 *	instances.
 */
class EntityFactory : public BWEntityFactory
{
private:
	BWEntity * doCreate( EntityTypeID entityTypeID,
		BWConnection * pConnection );
};

BW_END_NAMESPACE

#endif // ENTITY_FACTORY_HPP
