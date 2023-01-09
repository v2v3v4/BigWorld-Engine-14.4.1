#include "script/first_include.hpp"

#include "dump_controllers_extra.hpp"

#include "cellapp/controllers.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( DumpControllersExtra )

PY_BEGIN_METHODS( DumpControllersExtra )

	/*	function Entity visitControllers
	 *	@components{ cell }
	 *
	 *	The visitControllers method takes a callable object which will be
	 *	called as each controller associated with the Entity is visited.
	 *
	 *
	 *	The callable object will receive 2 arguments, being the name and id
	 *	of an individual controller. The callable object must also return
	 *	True to continue visiting, or False if visiting should terminate early.
	 *	An example callback function is provided below:
	 *
	 *		def controllerCB( name, id ):
	 *			print "Controller:", name, id
	 *			return True
	 *
	 *		Entity.visitControllers( controllerCB )
	 *
	 *	@params A callable object accepting 2 arguments (name/id) and
	 *			returning a boolean as to whether to continue visiting.
	 *
	 *	@returns None
	 */
	PY_METHOD( visitControllers )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( DumpControllersExtra )
PY_END_ATTRIBUTES()

const DumpControllersExtra::Instance< DumpControllersExtra >
	DumpControllersExtra::instance( DumpControllersExtra::s_getMethodDefs(),
			DumpControllersExtra::s_getAttributeDefs() );


/*
 *
 * ControllerCallbackVisitor
 *
 */

/**
 *	Helper visitor class to allow the extra to callback on each controller.
 */
class ControllerCallbackVisitor : public ControllersVisitor
{
public:
	ControllerCallbackVisitor( PyObjectPtr pCallback );

	bool hasErrorOccurred()		{ return errOccurred_; }

private:
	virtual bool visit( ControllerPtr pController );

	PyObjectPtr pCallback_;

	bool errOccurred_;
};


ControllerCallbackVisitor::ControllerCallbackVisitor( PyObjectPtr pCallback ) :
	pCallback_( pCallback ),
	errOccurred_( false )
{
}


bool ControllerCallbackVisitor::visit( ControllerPtr pController )
{
	PyObjectPtr pName( Script::getData( pController->typeName() ),
		PyObjectPtr::STEAL_REFERENCE );
	PyObjectPtr pID( Script::getData( pController->exclusiveID() ),
		PyObjectPtr::STEAL_REFERENCE );
	if (!pName || !pID)
	{
		ERROR_MSG( "ControllerCallbackVisitor::visit: "
			"Unable to convert values\n" );

		errOccurred_ = true;
		return false;
	}


	Py_INCREF( pCallback_.get() );

	PyObject * pResult = Script::ask( pCallback_.get(),
		PyTuple_Pack( 2, pName.get(), pID.get() ),
		"visitControllers" );

	if (!pResult)
	{
		ERROR_MSG( "ControllerCallbackVisitor::visit: "
			"Failed to call visitor callback, terminating visit.\n" );
		errOccurred_ = true;
		return false;
	}

	bool shouldContinue = PyObject_IsTrue( pResult );

	Py_DECREF( pResult );

	return shouldContinue;
}



/*
 *
 * DumpControllersExtra
 *
 */

/**
 *	Constructor.
 */
DumpControllersExtra::DumpControllersExtra( Entity & e ) :
	EntityExtra( e )
{
}


/**
 *	Destructor.
 */
DumpControllersExtra::~DumpControllersExtra()
{
}


bool DumpControllersExtra::visitControllers( PyObjectPtr pCallback )
{
	if (!PyCallable_Check( pCallback.get() ))
	{
		PyErr_Format( PyExc_TypeError, "Entity.visitControllers: "
				"Callback arg provided is not a non-callable object." );
		return false;
	}

	ControllerCallbackVisitor visitor( pCallback );

	entity_.visitControllers( visitor );

	return !visitor.hasErrorOccurred();
}

BW_END_NAMESPACE

// dump_controllers_extra.cpp
