#ifndef PY_AVATAR_DROP_FILTER_HPP
#define PY_AVATAR_DROP_FILTER_HPP

#include "avatar_drop_filter.hpp"
#include "py_avatar_filter.hpp"

#include "pyscript/script.hpp"
#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

class GroundNormalProvider;
typedef WeakPyPtr< GroundNormalProvider > WeakGroundNormalProviderPtr;
class Vector4Provider;
typedef SmartPointer< Vector4Provider > Vector4ProviderPtr;

/*~ class BigWorld.AvatarDropFilter
 *
 *	This class inherits from AvatarFilter.  It is nearly exactly the same
 *	as its parent, except that a findDropPoint is used to place each input
 *	point on the ground.
 *
 *	A new AvatarDropFilter is created using the BigWorld.AvatarDropFilter
 *	function.
 */
/**
 *	This is a Python object to manage an AvatarDropFilter instance, matching the
 *	lifecycle of a Python object to the (shorter) lifecycle of a MovementFilter.
 */
class PyAvatarDropFilter : public PyAvatarFilter
{
	Py_Header( PyAvatarDropFilter, PyAvatarFilter )

	PY_WEAK_REFERENCABLE( PyAvatarDropFilter )

public:
	PyAvatarDropFilter( PyTypeObject * pType = &s_type_ );
	virtual ~PyAvatarDropFilter() {}

	bool alignToGround() const;
	void alignToGround( bool newValue );

	Vector4ProviderPtr groundNormalProvider();

	// Python Interface
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PyAvatarDropFilter, END );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, alignToGround, alignToGround );
	PY_RO_ATTRIBUTE_DECLARE( groundNormalProvider(), groundNormal );

	// Implementation of PyFilter
	virtual AvatarDropFilter * pAttachedFilter();
	virtual const AvatarDropFilter * pAttachedFilter() const;

protected:
	// Implementation of PyFilter
	virtual AvatarDropFilter * getNewFilter();
	virtual void onLosingAttachedFilter();

private:
	WeakGroundNormalProviderPtr wpGroundNormalProvider_;

	// This attribute is a cache of the value in AvatarDropFilter, but
	// it is only used if this->pAttachedFilter() is NULL
	bool alignToGround_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyAvatarDropFilter );

BW_END_NAMESPACE

#endif // PY_AVATAR_DROP_FILTER_HPP
