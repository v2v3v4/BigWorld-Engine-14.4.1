#ifndef SIMPLE_CLIENT_ENTITY_HPP
#define SIMPLE_CLIENT_ENTITY_HPP

#include "entitydef/entity_description.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This namespace contains functions for dealing with simple client entities
 *	implemented by a Python object, with methods and properties in the
 *	natural way.
 */
namespace SimpleClientEntity
{
	bool propertyEvent( ScriptObject pEntity, const EntityDescription & edesc,
		int propertyID, BinaryIStream & data, bool shouldUseCallback );

	bool resetPropertiesEvent( ScriptObject pEntity,
		const EntityDescription & edesc, BinaryIStream & data );

	bool nestedPropertyEvent( ScriptObject pEntity,
		const EntityDescription & edesc,
		BinaryIStream & data, bool shouldUseCallback, bool isSlice );

	bool methodEvent( ScriptObject pEntity, const EntityDescription & edesc,
		int methodID, BinaryIStream & data );

};

BW_END_NAMESPACE

#endif // SIMPLE_CLIENT_ENTITY_HPP

