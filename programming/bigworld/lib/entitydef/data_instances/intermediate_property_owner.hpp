#ifndef INTERMEDIATE_PROPERTY_OWNER_HPP
#define INTERMEDIATE_PROPERTY_OWNER_HPP

#include "entitydef/property_owner.hpp"

BW_BEGIN_NAMESPACE

/**
 *
 */
class IntermediatePropertyOwner : public PropertyOwner
{
public:
	IntermediatePropertyOwner( PyTypeObject * pPyType ) :
		PropertyOwner( pPyType ),
		pOwner_( NULL ),
		ownerRef_( 0 )
	{
	}

	void setOwner( PropertyOwnerBase * pNewOwner, int ownerRef )
	{
		pOwner_ = pNewOwner;
		ownerRef_ = ownerRef;
	}

	void disowned()		{ pOwner_ = NULL; }
	bool hasOwner()		{ return pOwner_ != NULL; }

	// Overrides from PropertyOwner
	virtual bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner );

protected:
	bool shouldPropagate() const
	{
		// After finalisation we may not be detached properly from our owner
		// (see FixedDictDataType::detach()) if we are wrapped by a custom type
		// that keeps a reference to us. That's why we should avoid propagating
		// a change to our owner which may already be destroyed.

		 return (pOwner_ != NULL) && Py_IsInitialized();
	}

	PropertyOwnerBase * pOwner_;
	int ownerRef_;
};

BW_END_NAMESPACE

#endif // INTERMEDIATE_PROPERTY_OWNER_HPP
