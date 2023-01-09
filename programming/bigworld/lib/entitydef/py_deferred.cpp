#include "pch.hpp"

#include "py_deferred.hpp"
#include "method_description.hpp"

#include "network/nub_exception.hpp"
#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

namespace
{

// Stores a pointer to Twisted's Deferred class
PyObjectPtr s_classDeferred;


/**
 *	This class is used to free the static Python state kept by this file.
 */
class FiniDeferred : public Script::FiniTimeJob
{
public:
	virtual void fini()
	{
		s_classDeferred = NULL;
	}
};

FiniDeferred s_finiInstance;

}


// -----------------------------------------------------------------------------
// Section: ReturnValuesHandler
// -----------------------------------------------------------------------------

/**
 *	This method initialises the static state used by PyDeferred.
 */
bool PyDeferred::staticInit()
{
	static bool hasFailed = false;

	if (hasFailed)
	{
		return false;
	}

	// Okay to call multiple times.
	if (s_classDeferred)
	{
		return true;
	}

	PyObject * pModule = PyImport_ImportModule( "twisted.internet.defer" );

	if (!pModule)
	{
		ERROR_MSG( "PyDeferred::staticInit: "
				"Failed to import twisted.internet.defer\n" );
		PyErr_Print();

		hasFailed = true;

		return false;
	}

	s_classDeferred = PyObjectPtr(
						PyObject_GetAttrString( pModule, "Deferred" ),
						PyObjectPtr::STEAL_REFERENCE );
	Py_DECREF( pModule );

	if (!s_classDeferred)
	{
		ERROR_MSG( "PyDeferred::staticInit: "
				"Failed to get Deferred from module twisted.internet.defer\n" );
		PyErr_Print();

		hasFailed = true;

		return false;
	}

	return true;
}


/*
 *	This helper function creates a Twisted Deferred instance.
 */
PyDeferred::PyDeferred() :
	pObject_( s_classDeferred ?
				PyObject_CallFunctionObjArgs( s_classDeferred.get(), NULL ) :
				NULL,
			PyObjectPtr::STEAL_REFERENCE )
{
	// ReplyMessageHandler::staticInit must have been called successfully first.
	MF_ASSERT( s_classDeferred );
}


/**
 *
 */
bool PyDeferred::callback( PyObjectPtr pArg )
{ 
	return this->callMethod( "callback", pArg );
}


/**
 *	This method calls the errback with an exception.
 */
bool PyDeferred::errback( PyObjectPtr pArg )
{
	return this->callMethod( "errback", pArg );
}


/**
 *	This method calls the errback with an exception.
 */
bool PyDeferred::errback( const char * excType, const char * msg )
{
	return this->errback(
		MethodDescription::createErrorObject( excType, msg ) );
}


/**
 *
 */
bool PyDeferred::mercuryErrback( const Mercury::NubException & exception )
{
	return this->errback( "BWMercuryError",
			Mercury::reasonToString( exception.reason() ) );
}


/**
 *	This method is a helper for calling a method on the Deferred object.
 */
bool PyDeferred::callMethod( const char * methodName, PyObjectPtr values )
{
	PyObjectPtr pResult(
		PyObject_CallMethod( pObject_.get(), (char *)methodName, "(O)",
			values.get() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (!pResult)
	{
		WARNING_MSG( "PyDeferred::callMethod: "
					"Deferred.%s failed. "
					"See SCRIPT output for more information.\n",
				methodName );
		PyErr_Print();

		return false;
	}

	return true;
}

BW_END_NAMESPACE

// py_deferred.cpp
