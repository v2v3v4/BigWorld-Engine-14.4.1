#ifndef PY_BASES_HPP
#define PY_BASES_HPP

#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class Bases;

/**
 *	This class is used to expose the collection of bases to scripting.
 */
class PyBases : public PyObjectPlus
{
	Py_Header( PyBases, PyObjectPlus )

public:
	PyBases( const Bases & bases, PyTypeObject * pType = &PyBases::s_type_ );

	PyObject * 			subscript( PyObject * entityID );
	int					length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )
	PY_METHOD_DECLARE( py_get )

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

private:
	PyObject * findInstanceWithType( const char * typeName ) const;

	const Bases & bases_;
};

#ifdef CODE_INLINE
// #include "py_bases.ipp"
#endif

BW_END_NAMESPACE

#endif // PY_BASES_HPP
