#ifndef PYFASHION_HPP
#define PYFASHION_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class PyModel;
typedef SmartPointer<class MaterialFashion> MaterialFashionPtr;
typedef SmartPointer<class TransformFashion> TransformFashionPtr;

/*~ class BigWorld.PyFashion
 *
 *	This class is the abstract base class for objects that change the 
 *	appearance of a model. In particular, PyTransformFashion
 *	and PyMaterialFashion inherit from it.
 */
/**
 *	This class is the base class for python objects that wrap (or can
 *	otherwise provide) a fashion.
 */
class PyFashion : public PyObjectPlus
{
	Py_Header( PyFashion, PyObjectPlus )

public:
	PyFashion( PyTypeObject * pType ) :
		PyObjectPlus( pType ) { }

	/// This method ticks this fashion
	virtual void tick( float dTime, float lod ) { }

	/**
	 *	This method makes a copy of the fashion for the given model,
	 *	and returns a new reference to the copy. If no copy is necessary,
	 *	then another reference to the same fashion may be returned.
	 *	If NULL is returned, an exception must be set.
	 *	This method can also be used as an 'attach' method for fashions
	 *	which prefer to implement attach/detach semantics.
	 *
	 *	@note Do not store a reference to the PyModel, as it would be
	 *	circular.
	 */
	virtual PyFashion * makeCopy( PyModel * pModel, const char * attrName );

	/**
	 *	This method tells us that our model is no longer interested in us,
	 *	and isn't going to call us anymore.
	 */
	virtual void disowned() { }

protected:
	virtual ~PyFashion() {}
};

BW_END_NAMESPACE

#endif // PYFASHION_HPP
