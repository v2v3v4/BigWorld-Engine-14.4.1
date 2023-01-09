#ifndef REAL_CALLER_HPP
#define REAL_CALLER_HPP

#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class MethodDescription;

typedef SmartPointer< Entity >                   EntityPtr;

/**
 *	This class implements a simple helper Python type. Objects of this type are
 *	used to represent methods that the client can call on the server.
 */
class RealCaller : public PyObjectPlus
{
	Py_Header( RealCaller, PyObjectPlus )

public:
	RealCaller( Entity * pEntity,
			const MethodDescription * pMethodDescription,
			PyTypeObject * pType = &RealCaller::s_type_ ) :
		PyObjectPlus( pType ),
		pEntity_( pEntity ),
		pMethodDescription_( pMethodDescription )
	{
	}

	PY_METHOD_DECLARE( pyCall )

private:
	EntityPtr pEntity_;
	const MethodDescription * pMethodDescription_;
};

BW_END_NAMESPACE

#endif // REAL_CALLER_HPP

