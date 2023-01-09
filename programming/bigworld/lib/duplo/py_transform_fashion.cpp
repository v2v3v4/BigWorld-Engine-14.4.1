#include "pch.hpp"
#include "py_transform_fashion.hpp"

namespace BW
{

PY_TYPEOBJECT( PyTransformFashion )

PY_BEGIN_METHODS( PyTransformFashion )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyTransformFashion )
PY_END_ATTRIBUTES()


PyTransformFashion::PyTransformFashion( PyTypeObject * pType ) :
	PyFashion( pType )
{
}


/**
 *	This method checks if this is an animation to be applied first
 *	or whether it should to be applied after animating (e.g., a Tracker
 *	or IKConstraintSystem).
 *	@return true if the transform is to be applied in the first pass.
 */
bool PyTransformFashion::isPreTransform() const
{
	BW_GUARD;
	return true;
}


/**
 *	This method check if this animation is being applied this frame or
 *	if it has been turned off. Eg. due to lodding.
 *	@return true if the transform is actively being applied.
 */
bool PyTransformFashion::isAffectingTransformThisFrame() const
{
	BW_GUARD;
	return true;
}

} // namespace BW
