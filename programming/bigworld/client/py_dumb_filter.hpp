#ifndef PY_DUMB_FILTER_HPP
#define PY_DUMB_FILTER_HPP

#include "dumb_filter.hpp"
#include "py_filter.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.DumbFilter
 *
 *	This subclass of the Filter class simply sets the position of the entity
 *	to the most recent position specified by the server, performing no
 *	interpolation.  It ignores position updates which are older than the
 *	current position of the entity.
 *
 *	A new DumbFilter can be created using the BigWorld.DumbFilter function.
 */
/**
 *	This is a Python object to manage an DumbFilter instance, matching the
 *	lifecycle of a Python object to the (shorter) lifecycle of a MovementFilter.
 */
class PyDumbFilter : public PyFilter
{
	Py_Header( PyDumbFilter, PyFilter )

public:
	PyDumbFilter( PyTypeObject * pType = &s_type_ );
	virtual ~PyDumbFilter() {}

	// Python Interface
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PyDumbFilter, END )

	// Implementation of PyFilter
	virtual DumbFilter * pAttachedFilter();
	virtual const DumbFilter * pAttachedFilter() const;

protected:
	// Implementation of PyFilter
	virtual DumbFilter * getNewFilter();
};

PY_SCRIPT_CONVERTERS_DECLARE( PyDumbFilter );

BW_END_NAMESPACE

#endif // PY_DUMB_FILTER_HPP
