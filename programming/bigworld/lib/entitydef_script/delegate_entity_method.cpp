#include "pch.hpp"

#include "delegate_entity_method.hpp"
#include "entitydef/method_description.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT_WITH_CALL( DelegateEntityMethod )

PY_BEGIN_ATTRIBUTES( DelegateEntityMethod )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( DelegateEntityMethod )
PY_END_METHODS()

DelegateEntityMethod::DelegateEntityMethod( 
			const IEntityDelegatePtr & delegate,
			const MethodDescription * pMethodDescription,
			EntityID sourceID,
			PyTypeObject * pType ) : PyObjectPlus( pType ),
	delegate_( delegate ),
	pMethodDescription_( pMethodDescription ),
	sourceID_( sourceID )
{
	MF_ASSERT( pMethodDescription && pMethodDescription->isComponentised() );
}

PyObject * DelegateEntityMethod::pyCall( PyObject * args, PyObject * kwargs )
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

	if (!pMethodDescription_->areValidArgs( true, pArgs, true ))
	{
		return NULL;
	}
	
	MemoryOStream stream;
	
	EntityID sourceEntityID = NULL_ENTITY_ID;
	
	// Add the arguments to the stream.
	if (pMethodDescription_->component() == MethodDescription::CLIENT)
	{
		ScriptDataSource source( pArgs );
		if (!pMethodDescription_->addToClientStream( source, stream,
				sourceID_ ))
		{
			return NULL;
		}
	}
	else
	{
		ScriptTuple remainingArgs =
			pMethodDescription_->extractSourceEntityID( pArgs, sourceEntityID );

		ScriptDataSource source( remainingArgs );
		if (!pMethodDescription_->addToServerStream( source, stream,
				sourceEntityID ))
		{
			return NULL;
		}
	}
	
	if (!delegate_->handleMethodCall( *pMethodDescription_,
		stream, sourceEntityID ))
	{
		PyErr_SetString( PyExc_ValueError,
						"Failed to call delegate method" );
		return NULL;
	}
	
	Py_RETURN_NONE;
}

BW_END_NAMESPACE
