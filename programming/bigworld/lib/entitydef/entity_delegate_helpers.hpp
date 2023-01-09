#ifndef ENTITY_DELEGATE_HELPERS_HPP
#define ENTITY_DELEGATE_HELPERS_HPP

#include "delegate_interface/entity_delegate.hpp"

BW_BEGIN_NAMESPACE

ScriptDict createDictWithAllProperties( 
		const EntityDescription & entityDesription,
		const ScriptObject & entity,
		IEntityDelegate * pEntityDelegate, 
		int dataDomains );

bool populateDelegateWithDict( 
		const EntityDescription & entityDesription,
		IEntityDelegate * pEntityDelegate, 
		const ScriptDict & dict,
		int dataDomains );

BW_END_NAMESPACE

#endif // ENTITY_DELEGATE_HELPERS_HPP
