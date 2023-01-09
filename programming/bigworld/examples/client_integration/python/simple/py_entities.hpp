#ifndef PY_ENTITIES_HPP
#define PY_ENTITIES_HPP

#include "connection_model/bw_entities.hpp"

#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to expose the collection of entities to scripting.
 */
class PyEntities : public PyObjectPlus
{
	Py_Header( PyEntities, PyObjectPlus )

public:
	PyEntities( const BWEntities & entities,
		PyTypeObject * pType = &PyEntities::s_type_ );
	~PyEntities();

	PyObject * 			subscript( PyObject * entityID );
	int					length();

	PY_METHOD_DECLARE(py_has_key)
	PY_METHOD_DECLARE(py_keys)
	PY_METHOD_DECLARE(py_values)
	PY_METHOD_DECLARE(py_items)

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

private:
	PyEntities( const PyEntities & );
	PyEntities& operator=( const PyEntities & );

	typedef PyObject * (*GetFunc)( const BWEntities::const_iterator & item );
	PyObject * makeList( GetFunc objectFunc );

	const BWEntities & entities_;
};


#ifdef CODE_INLINE
#include "py_entities.ipp"
#endif

BW_END_NAMESPACE


#endif // PY_ENTITIES_HPP
