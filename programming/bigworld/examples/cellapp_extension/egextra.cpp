#include "script/first_include.hpp"

#include "egextra.hpp"
#include "egcontroller.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( EgExtra )

PY_BEGIN_METHODS( EgExtra )
	PY_METHOD( helloWorld )
	PY_METHOD( addEgController )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( EgExtra )
PY_END_ATTRIBUTES()

const EgExtra::Instance< EgExtra >
		EgExtra::instance( EgExtra::s_getMethodDefs(),
			EgExtra::s_getAttributeDefs() );

EgExtra::EgExtra( Entity& e ) : EntityExtra( e )
{
}

EgExtra::~EgExtra()
{
}

void EgExtra::helloWorld()
{
	DEBUG_MSG( "egextra: hello world\n" );
}

PyObject * EgExtra::addEgController( int userArg )
{
	ControllerPtr pController = new EgController();
	ControllerID controllerID = entity_.addController( pController, userArg );
	return Script::getData( controllerID );
}

BW_END_NAMESPACE

// egextra.cpp
