#include "script/first_include.hpp"

#include "real_caller.hpp"

#include "entity.hpp"

#include "entitydef/method_description.hpp"

#include "entitydef_script/delegate_entity_method.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RealCaller
// -----------------------------------------------------------------------------

/**
 *	This method is called when a script wants to call this method
 *	on a ghost entity.
 */
PyObject * RealCaller::pyCall( PyObject * pArgs )
{
	ScriptTuple args( pArgs, ScriptTuple::FROM_BORROWED_REFERENCE );

	if (!pMethodDescription_->areValidArgs( /* exposedExplicit */ true, args,
		/* generateException */ true ))
	{
		return NULL;
	}

	if (pEntity_->isDestroyed())
	{
		PyErr_Format( PyExc_TypeError,
			"Cannot call %s on a destroyed entity (%d)",
			pMethodDescription_->name().c_str(), int(pEntity_->id()) );
		return NULL;
	}

	if (pEntity_->isRealToScript())
	{
		WARNING_MSG( "RealCaller::pyCall: "
				"Calling %s on %u which is now real\n",
			pMethodDescription_->name().c_str(), pEntity_->id() );

		if (pEntity_->pEntityDelegate() && 
			pMethodDescription_->isComponentised())
		{
			DelegateEntityMethod delegateMethod(
				pEntity_->pEntityDelegate(), pMethodDescription_, pEntity_->id() );

			return delegateMethod.pyCall( pArgs, NULL );
		}
		else
		{
			ScriptObject entity( pEntity_ );
			ScriptObject res = 
				entity.callMethod( pMethodDescription_->name().c_str(),
				ScriptArgs( args ), ScriptErrorPrint( "RealCaller::pyCall: " ) );
			return res.newRef();
		}
	}
	else if (!pEntity_->sendMessageToReal( pMethodDescription_, args ))
	{
		PyErr_Format( PyExc_SystemError,
			"Unexpected error calling method %s",
			pMethodDescription_->name().c_str() );
		return NULL;
	}

	Py_RETURN_NONE;
}

PY_TYPEOBJECT_WITH_CALL( RealCaller )

PY_BEGIN_METHODS( RealCaller )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RealCaller )
PY_END_ATTRIBUTES()

BW_END_NAMESPACE

// real_caller.cpp
