#ifndef PY_AOI_ENTITIES_HPP
#define PY_AOI_ENTITIES_HPP

#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

class Entity;
typedef SmartPointer< Entity > EntityPtr;
class AoIEntityVisitor;

/**
 *	This class adapts the AoI entities collection for a witness entity to the 
 *	Python dict interface.
 */
class PyAoIEntities : public PyObjectPlus
{
	Py_Header( PyAoIEntities, PyObjectPlus )

public:
	PyAoIEntities( Entity * pWitness );

	~PyAoIEntities();

	size_t numInAoI() const;

	bool isValid() const;
	Entity * getAoIEntity( EntityID entityID );

	// Python method declarations
	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	// Python mapping implementation methods.
	static Py_ssize_t pyGetLength( PyObject * self );
	static PyObject * pyGetFieldByKey( PyObject * self, PyObject * key );
	
	// Python iterator implementation methods.
	static PyObject * pyGetIter( PyObject * self );
	static PyObject * pyIterNext( PyObject * iteration );
	PyObject * pyRepr();

private:
	bool visitAoI( PyObject * args, AoIEntityVisitor & visitor );

	EntityPtr pWitness_;
};


BW_END_NAMESPACE

#endif // PY_AOI_ENTITIES_HPP