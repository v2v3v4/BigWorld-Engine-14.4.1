#ifndef REMOTE_ENTITY_METHOD_HPP
#define REMOTE_ENTITY_METHOD_HPP

#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class MethodDescription;
class PyEntityMailBox;


/**
 *  This class implements a simple helper Python type. Objects of this type are
 *  used to represent methods that the base can call on another script object
 *  (e.g. the cell, the client, or another base).
 */
class RemoteEntityMethod : public PyObjectPlus
{
	Py_Header( RemoteEntityMethod, PyObjectPlus )

public:
	RemoteEntityMethod( PyEntityMailBox * pMailBox,
			const MethodDescription * pMethodDescription,
			PyTypeObject * pType = &s_type_ ) :
		PyObjectPlus( pType ),
		pMailBox_( pMailBox ),
		pMethodDescription_( pMethodDescription )
	{
	}
	~RemoteEntityMethod() { }

	PY_KEYWORD_METHOD_DECLARE( pyCall )

	ScriptDict convertReturnValuesToDict( ScriptTuple pArgs ) const;
	PY_AUTO_METHOD_DECLARE( RETDATA,
			convertReturnValuesToDict, ARG( ScriptTuple, END ) );

	ScriptObject argumentTypes() const;
	ScriptObject returnValueTypes() const;

	PY_RO_ATTRIBUTE_DECLARE( argumentTypes(), argumentTypes );
	PY_RO_ATTRIBUTE_DECLARE( returnValueTypes(), returnValueTypes );

private:
	SmartPointer< PyEntityMailBox > pMailBox_;
	const MethodDescription * pMethodDescription_;
};

BW_END_NAMESPACE

#endif // REMOTE_ENTITY_METHOD_HPP

