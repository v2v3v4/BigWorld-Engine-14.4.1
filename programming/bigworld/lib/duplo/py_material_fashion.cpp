#include "pch.hpp"
#include "py_material_fashion.hpp"

namespace BW
{

PY_TYPEOBJECT( PyMaterialFashion )

PY_BEGIN_METHODS( PyMaterialFashion )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMaterialFashion )
PY_END_ATTRIBUTES()


PyMaterialFashion::PyMaterialFashion( PyTypeObject * pType ) :
	PyFashion( pType )
{
}

} // namespace BW
