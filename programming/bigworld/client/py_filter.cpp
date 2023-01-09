#include "pch.hpp"

#include "py_filter.hpp"

#include "app.hpp"
#include "filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT_WITH_WEAKREF( PyFilter )

PY_BEGIN_METHODS( PyFilter )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFilter )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyFilter )


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
PyFilter::PyFilter( PyTypeObject * pType ) :
	PyObjectPlusWithWeakReference( pType ),
	pAttachedFilter_( NULL ),
	isAttached_( false )
{
}


/**
 *	Destructor
 */
PyFilter::~PyFilter()
{
}


/*~ function Filter.reset
 *
 *	This function resets the current time for the filter to the specified
 *	time.
 *
 *	Filters use the current time to interpolate forwards the last update of
 *	the volatile data from the server.  Setting this time to a different
 *	value will move all entities to the place the filter thinks they should
 *	be at that specified time.
 *
 *	As a general rule this shouldn't need to be called.
 *
 *	@param	time	a Float the time to set the filter to.
 */
/**
 *	This method resets this filter to the input time.
 */
void PyFilter::reset( double time )
{
	if (pAttachedFilter_ != NULL)
	{
		pAttachedFilter_->reset( time );
	}
}


/**
 *	This method provides a new Filter instance from this PyFilter.
 */
Filter * PyFilter::getFilter()
{
	pAttachedFilter_ = this->getNewFilter();
	return pAttachedFilter_;
}


/**
 *	This method is called to notify us that our filter has been
 *	destroyed.
 */
void PyFilter::onFilterDestroyed( Filter * pDestroyedFilter )
{
	// Not sure if this can happen sensibly.
	IF_NOT_MF_ASSERT_DEV( pDestroyedFilter == pAttachedFilter_ )
	{
		return;
	}

	this->onLosingAttachedFilter();

	pAttachedFilter_ = NULL;
}


/**
 *	Static helper method to get the current game time.
 *
 *	@return	The time in seconds since the client was started.
 */
/* static */ double PyFilter::getTimeNow()
{
	BW_GUARD;
	return App::instance().getGameTimeFrameStart();
}


BW_END_NAMESPACE

// py_filter.cpp
