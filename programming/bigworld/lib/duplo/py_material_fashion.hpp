#ifndef PY_MATERIAL_FASHION_HPP
#define PY_MATERIAL_FASHION_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "cstdmf/smartpointer.hpp"

#include "duplo/pyfashion.hpp"

BW_BEGIN_NAMESPACE

/*~ class BigWorld.PyMaterialFashion
 *
 *	This class is the abstract base class for objects that change the 
 *	properties of a model at render time. In particular, PyDye.
 */
/**
 *	This class is the base class for Python objects that wrap
 *	a MaterialFashion.
 */
class PyMaterialFashion : public PyFashion
{
	Py_Header( PyMaterialFashion, PyFashion )

public:
	PyMaterialFashion( PyTypeObject * pType = &PyMaterialFashion::s_type_ );

	/**
	 *	This method returns a smart pointer to the underlying fashion.
	 *	@return the wrapped MaterialFashion.
	 */
	virtual const MaterialFashionPtr materialFashion() const = 0;
	virtual MaterialFashionPtr materialFashion() = 0;

protected:
	virtual ~PyMaterialFashion() {}
};

BW_END_NAMESPACE

#endif // PY_MATERIAL_FASHION_HPP
