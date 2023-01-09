#include "pch.hpp"

#include "remote_entity_method.hpp"

#include "mailbox_base.hpp"
#include "method_description.hpp"

DECLARE_DEBUG_COMPONENT2( "EntityDef", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RemoteEntityMethod
// -----------------------------------------------------------------------------

/*~ class NoModule.RemoteEntityMethod
 *	@components{ base, cell }
 *
 *	An instance of RemoteEntityMethod is return from an entity mailbox to allow
 *	the calling of a method on a remote machine
 */
PY_TYPEOBJECT_WITH_CALL( RemoteEntityMethod )

PY_BEGIN_METHODS( RemoteEntityMethod )
	PY_METHOD( convertReturnValuesToDict )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RemoteEntityMethod )
	PY_ATTRIBUTE( argumentTypes )
	PY_ATTRIBUTE( returnValueTypes )
PY_END_ATTRIBUTES()


/**
 *	This method is called when a script wants to call this method
 *	on a remote script handler (entity/base).
 */
PyObject * RemoteEntityMethod::pyCall( PyObject * args, PyObject * kwargs )
{
	ScriptTuple pArgs = ScriptTuple( args,
		ScriptTuple::FROM_BORROWED_REFERENCE );

	if (kwargs)
	{
		pArgs = pMethodDescription_->convertKeywordArgs( pArgs,
			ScriptDict( kwargs, ScriptObject::FROM_BORROWED_REFERENCE ) );

		if (!pArgs)
		{
			return NULL;
		}
	}
	return pMailBox_->callMethod( pMethodDescription_, pArgs );
}


/**
 *	This method creates a dictionary from a tuple of return values.
 */
ScriptDict RemoteEntityMethod::convertReturnValuesToDict(
		ScriptTuple pArgs ) const
{
	return pMethodDescription_->createDictFromTuple( pArgs );
}


/*~ attribute RemoteEntityMethod.argumentTypes
 *	@components{ base, cell }
 *
 *	This attribute is a tuple that describes the type of each argument that an
 *	entity method expects. Each element in the tuple is a 2-tuple containing the
 *	argument's name and a string describing the type of that argument.
 */
/**
 *	This method returns a tuple that describes the type of each argument that
 *	entity method expects. Each element in the tuple is a 2-tuple of the
 *	argument name and a string describing the type of that argument.
 */
ScriptObject RemoteEntityMethod::argumentTypes() const
{
	return pMethodDescription_->argumentTypesAsScript();
}


/*~ attribute RemoteEntityMethod.returnValueTypes
 *	@components{ base, cell }
 *
 *	This attribute is a tuple that describes the type of each return value that
 *	an entity method expects. Each element in the tuple is a 2-tuple containing
 *	the	return value's name and a string describing the type of that return
 *	value.
 */
/**
 *	This method returns a tuple that describes the type of each return value
 *	that entity method expects. Each element in the tuple is a 2-tuple of the
 *	return value name and a string describing the type of that return value.
 *
 */
ScriptObject RemoteEntityMethod::returnValueTypes() const
{
	return pMethodDescription_->returnValueTypesAsScript();
}

BW_END_NAMESPACE

// remote_entity_method.cpp

