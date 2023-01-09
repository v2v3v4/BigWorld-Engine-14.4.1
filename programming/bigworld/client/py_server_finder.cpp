#include "py_server_finder.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

namespace
{

PyObjectPtr getAttr( PyObjectPtr pObject, const char * attrName )
{
	PyObjectPtr pAttr( PyObject_GetAttrString( pObject.get(), attrName ),
		PyObjectPtr::STEAL_REFERENCE );

	if (!pAttr)
	{
		PyErr_Clear();
	}

	return pAttr;
}
}

// -----------------------------------------------------------------------------
// Section: FindServerDoneCaller
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
FindServerDoneCaller::FindServerDoneCaller( ScriptObject pCallback ) :
	pCallback_( pCallback )
{
}


/**
 *	Destructor. This calls the callback when there are no more references to
 *	this object.
 */
FindServerDoneCaller::~FindServerDoneCaller()
{
	Script::call( pCallback_.newRef(), NULL );
}


// -----------------------------------------------------------------------------
// Section: ProbeReplyHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PyServerProbeHandler::PyServerProbeHandler( ScriptObject pCallback,
		const ServerInfo & serverInfo,
		FindServerDoneCallerPtr pDoneCaller ) :
	pDict_( PyDict_New(), ScriptObject::STEAL_REFERENCE ),
	pCallback_( pCallback ),
	pDoneCaller_( pDoneCaller )
{
	pDict_.setItem( "address",
		ScriptObject::createFrom( serverInfo.address() ).get() );

	pDict_.setItem( "serverString",
		ScriptObject::createFrom( serverInfo.address().c_str() ).get() );

	pDict_.setItem( "uid",
		ScriptObject::createFrom( serverInfo.uid() ).get() );
}


/**
 *	This method is called when there is a key/value pair received from the
 *	server.
 */
void PyServerProbeHandler::onKeyValue( const BW::string & key,
	const BW::string & value )
{
	pDict_.setItemString( key.c_str(),
		ScriptObject::createFrom( value ).get() );
}


/**
 *	This method is called when there has been a successful response.
 */
void PyServerProbeHandler::onSuccess()
{
	Script::call( pCallback_.newRef(),
		PyTuple_Pack( 1, pDict_.get(), NULL ) );
}


// -----------------------------------------------------------------------------
// Section: PyServerFinder
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PyServerFinder::PyServerFinder( PyObjectPtr pFoundCallback,
		PyObjectPtr pDetailsCallback, PyObjectPtr pDoneCallback ):
	pFoundCallback_( pFoundCallback ),
	pDetailsCallback_( pDetailsCallback ),
	pDoneCaller_()
{
	if (pDoneCallback)
	{
		// Reference counted object that calls onFindServersDone
		pDoneCaller_ = new FindServerDoneCaller( pDoneCallback );
	}
}


/*
 *	Override from ServerFinder.
 */
void PyServerFinder::onServerFound( const ServerInfo & serverInfo )
{
	PyObjectPtr pFoundServer( Script::getData( serverInfo.address() ),
					PyObjectPtr::STEAL_REFERENCE );
	Script::call( pFoundCallback_.newRef(),
		PyTuple_Pack( 1, pFoundServer.get(), NULL ) );

	if (pDetailsCallback_)
	{
		this->sendProbe( serverInfo.address(),
			new PyServerProbeHandler( pDetailsCallback_,
				serverInfo, pDoneCaller_ ) );
	}
}


// -----------------------------------------------------------------------------
// Section: Python interface
// -----------------------------------------------------------------------------

/**
 *	This method is resposed to Python script. It attempts to find all LoginApp
 *	processes on the local network.
 */
bool findServers( PyObjectPtr pCallback )
{
	PyObjectPtr pFoundCallback = getAttr( pCallback, "onFoundServer" );
	PyObjectPtr pDetailsCallback = getAttr( pCallback, "onFoundDetails" );
	PyObjectPtr pCompleteCallback = getAttr( pCallback, "onFindServersDone" );

	if ((!pFoundCallback || !PyCallable_Check( pFoundCallback.get())) &&
		(!pDetailsCallback || !PyCallable_Check( pDetailsCallback.get() )))
	{
		PyErr_SetString( PyExc_ValueError,
			"Callback object must have a 'onFoundServer' or "
				"'onFoundDetails' method" );

		return false;
	}

	ServerFinder * pFinder = new PyServerFinder( pFoundCallback,
				pDetailsCallback, pCompleteCallback );

	pFinder->findServers(
		ConnectionControl::serverConnection()->networkInterface() );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, findServers,
	ARG( PyObjectPtr, END ), BigWorld );

BW_END_NAMESPACE

// py_server_finder.cpp
