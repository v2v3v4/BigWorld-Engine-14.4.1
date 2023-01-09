#ifndef ENTITY_EXTENSION_FACTORY_MANAGER_HPP
#define ENTITY_EXTENSION_FACTORY_MANAGER_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class EntityExtensionFactoryBase;

/**
 *	This class is handles a collection of EntityExtensionFactoryBase instances.
 */
class EntityExtensionFactoryManager
{
public:
	void add( EntityExtensionFactoryBase * pFactory );

	size_t size() const	{ return factories_.size(); }
	EntityExtensionFactoryBase * operator[]( int i ) { return factories_[i]; }

private:
	BW::vector< EntityExtensionFactoryBase * > factories_;
};

BW_END_NAMESPACE

#endif // ENTITY_EXTENSION_FACTORY_MANAGER_HPP
