#ifndef PY_FILTER_HPP
#define PY_FILTER_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

class Filter;

/*~ class BigWorld.Filter
 *
 *	This is the abstract base class for all Filter objects.  Filters are used
 *	to interpolate volatile data of entities (the position and the orientation)
 *	between server updates of these variables.
 *
 *	The only functionality defined in the base class is the reset function,
 *	which resets the filter to have its current time set to the specified time.
 *
 *	Every entity has a filter attribute.  Assigning a filter to that attribute
 *	sets that entity to be the owner of the specified filter.  This will make
 *	the filter process server updates for that entity.
 */
/**
 *	This is a Python object to manage a Filter instance, matching the lifecycle
 *	of a Python object to the (shorter) lifecycle of a MovementFilter.
 *
 *	Setting this as the "filter" property of a PyEntity will cause a filter
 *	to be attached to the Entity.
 */
class PyFilter : public PyObjectPlusWithWeakReference
{
	Py_Header( PyFilter, PyObjectPlusWithWeakReference )

public:
	PyFilter( PyTypeObject * pType );
	virtual ~PyFilter();

	/**
	 *	This method returns the Filter * created by getNewFilter() if it
	 *	still exists, and NULL otherwise.
	 *	Subclasses may override it covariantly for their own convenience.
	 */
	virtual Filter * pAttachedFilter() { return pAttachedFilter_; }

	/**
	 *	This method returns the const Filter * created by getNewFilter() if it
	 *	still exists, and NULL otherwise.
	 *	Subclasses may override it covariantly for their own convenience.
	 */
	virtual const Filter * pAttachedFilter() const
		{ return pAttachedFilter_; }

	// This method is for Filter to call when it is destroyed.
	void onFilterDestroyed( Filter * pDestroyedFilter );

	// This getter/setter method is for PyEntity to track whether this filter
	// is attached to an entity or not.
	bool isAttached() const { return isAttached_; }
	void isAttached( bool newValue ) { isAttached_ = newValue; }

	// This method is for PyEntity to get a Filter * from this PyFilter
	Filter * getFilter();

	// Python Interface
	PY_AUTO_METHOD_DECLARE( RETVOID, reset,
		OPTARG( double, PyFilter::getTimeNow(), END ) );

protected:
	static double getTimeNow();

private:
	// Disable copy-constructor and copy-assignment
	PyFilter( const PyFilter & other );
	PyFilter & operator=( const PyFilter & other );

	void reset( double time );

	// This is the main interface for subclasses to override.
	/**
	 *	Return a new Filter for the given Entity. After this, the
	 *	life-cycle of the returned Filter is opaque to the subclass, so
	 *	any further access to the Filter instance must be done using
	 *	pAttachedFilter()
	 */
	virtual Filter * getNewFilter() = 0;

	/**
	 *	pAttachedFilter() is about to be destroyed. This is an opportunity for
	 *	subclasses to cache any settings on pAttachedFilter_ for later
	 *	recreation.
	 */
	virtual void onLosingAttachedFilter() {}

	// Weak-pointer to the last filter created by getNewFilter,
	// cleared by onFilterDestroyed
	Filter * pAttachedFilter_;

	bool isAttached_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyFilter );

BW_END_NAMESPACE

#endif // PY_FILTER_HPP
