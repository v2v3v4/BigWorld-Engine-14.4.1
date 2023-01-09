#ifndef ENTITY_DESCRIPTION_DEBUG_HPP
#define ENTITY_DESCRIPTION_DEBUG_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class EntityDescription;
class EntityDescriptionMap;

/**
 *	This is a simple namespace that contains functions to help debug
 *	EntityDescription related issues.
 */
namespace EntityDescriptionDebug
{
void dump( const EntityDescription & desc, int detailLevel = 100 );
void dump( const EntityDescriptionMap & desc, int detailLevel = 100 );
}

BW_END_NAMESPACE

#endif // ENTITY_DESCRIPTION_DEBUG_HPP
