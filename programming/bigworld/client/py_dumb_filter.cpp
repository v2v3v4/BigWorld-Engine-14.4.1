#include "pch.hpp"

#include "py_dumb_filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyDumbFilter )

PY_BEGIN_METHODS( PyDumbFilter )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyDumbFilter )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyDumbFilter )

/*~ function BigWorld.DumbFilter
 *
 *	This function creates a new DumbFilter, which is the simplest filter and
 *	just sets the position of the owning entity to the most recently received
 *	position from the server.
 *
 *	@return	A new DumbFilter object
 */
PY_FACTORY_NAMED( PyDumbFilter, "DumbFilter", BigWorld )


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
PyDumbFilter::PyDumbFilter( PyTypeObject * pType ) :
	PyFilter( pType )
{
	BW_GUARD;
}


/**
 *	This method returns the DumbFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
DumbFilter * PyDumbFilter::pAttachedFilter()
{
	BW_GUARD;

	return static_cast< DumbFilter * >( this->PyFilter::pAttachedFilter() );
}


/**
 *	This method returns the const DumbFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
const DumbFilter * PyDumbFilter::pAttachedFilter() const
{
	BW_GUARD;

	return static_cast< const DumbFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	Generate a new DumbFilter
 */
DumbFilter * PyDumbFilter::getNewFilter()
{
	BW_GUARD;

	return new DumbFilter( this );
}


BW_END_NAMESPACE

// py_dumb_filter.cpp
