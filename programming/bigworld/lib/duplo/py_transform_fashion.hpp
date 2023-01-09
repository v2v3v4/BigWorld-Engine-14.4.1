#ifndef PY_TRANSFORM_FASHION_HPP
#define PY_TRANSFORM_FASHION_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "cstdmf/smartpointer.hpp"

#include "duplo/pyfashion.hpp"

BW_BEGIN_NAMESPACE

/*~ class BigWorld.PyTransformFashion
 *
 *	This class is the abstract base class for objects that change the 
 *	animations of a model. In particular, ActionQueuer and Tracker all inherit from it.
 */
/**
 *	This class is the base class for Python objects that wrap
 *	a TransformFashion.
 */
class PyTransformFashion : public PyFashion
{
	Py_Header( PyTransformFashion, PyFashion )

public:
	PyTransformFashion( PyTypeObject * pType = &PyTransformFashion::s_type_ );

	/**
	 *	This method returns a smart pointer to the underlying fashion.
	 *	@return the wrapped TransformFashion.
	 */
	virtual const TransformFashionPtr transformFashion() const = 0;
	virtual TransformFashionPtr transformFashion() = 0;

	virtual bool isPreTransform() const;
	virtual bool isAffectingTransformThisFrame() const;

protected:
	virtual ~PyTransformFashion() {}
};

BW_END_NAMESPACE

#endif // PY_TRANSFORM_FASHION_HPP
