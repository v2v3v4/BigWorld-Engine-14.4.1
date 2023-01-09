#include "script/first_include.hpp"

#include "service.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

PY_BASETYPEOBJECT( Service )

PY_BEGIN_METHODS( Service )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Service )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Service
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Service::Service( EntityID id, DatabaseID dbID, EntityType * pType ) :
	Base( id, dbID, pType )
{
	// TRACE_MSG( "Service::Service: id = %u\n", this->id() );


	// Make sure that this class does not have virtual functions. If it does,
	// it screws up the memory layout when we pretend it is a PyInstanceObject.
	MF_ASSERT( (void *)this == (void *)((PyObject *)this) );
	MF_ASSERT( id_ != 0 );
	MF_ASSERT( pType->description().isService() );
}

BW_END_NAMESPACE

// service.cpp
