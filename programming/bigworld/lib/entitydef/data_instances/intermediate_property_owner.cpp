#include "pch.hpp"

#include "intermediate_property_owner.hpp"

#include "entitydef/property_change.hpp"


BW_BEGIN_NAMESPACE

/**
 *	One of our properties is telling us it's been changed internally.
 */
bool IntermediatePropertyOwner::getTopLevelOwner( PropertyChange & change,
		PropertyOwnerBase *& rpTopLevelOwner )
{
	if (this->shouldPropagate())
	{
		change.isNestedChange( true );
		bool result = pOwner_->getTopLevelOwner( change, rpTopLevelOwner );
		if (rpTopLevelOwner != pOwner_)
		{
			change.addToPath( ownerRef_, pOwner_->getNumOwnedProperties() );
		}
		else
		{
			change.rootIndex( ownerRef_ );
		}
		return result;
	}

	return true;
}

BW_END_NAMESPACE

// intermediate_property_owner.cpp
