#ifndef ENTITY_EXTENSION_FACTORY_BASE_HPP
#define ENTITY_EXTENSION_FACTORY_BASE_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class EntityExtension;

/**
 *	This class is the base class for a factory used to create EntityExtension
 *	instances.
 */
class EntityExtensionFactoryBase
{
public:
	EntityExtensionFactoryBase() : slot_( -1 ) {}
	void setSlot( int slot )	{ slot_ = slot; }

	int slot() const			{ return slot_; }

	EntityExtension * getFrom( const BWEntity & entity ) const;

protected:
	int slot_;
};

BW_END_NAMESPACE

#endif // ENTITY_EXTENSION_FACTORY_BASE_HPP
