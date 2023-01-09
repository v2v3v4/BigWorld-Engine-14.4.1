#ifndef PY_AVATAR_FILTER_HPP
#define PY_AVATAR_FILTER_HPP

#include "avatar_filter.hpp"
#include "py_filter.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.AvatarFilter
 *
 *	This class inherits from Filter.  It implements a filter which tracks
 *	the last 8 entity updates from the server, and linearly interpolates
 *	between them.
 *
 *	Linear interpolation is done at ( time - latency ), where time is the
 *	current engine time, and latency is how far in the past the entity
 *	currently is.
 *
 *	Latency moves from its current value in seconds to the "ideal latency"
 *	which is the time between the two most recent updates if an update
 *	has just arrived, otherwise it is 2 * minLatency.  Lantency moves at
 *	velLatency seconds per second.
 *
 *	A new AvatarFilter is created using the BigWorld.AvatarFilter function.
 */
/**
 *	This is a Python object to manage an AvatarFilter instance, matching the
 *	lifecycle of a Python object to the (shorter) lifecycle of a MovementFilter.
 */
class PyAvatarFilter : public PyFilter
{
	Py_Header( PyAvatarFilter, PyFilter )

public:
	PyAvatarFilter( PyTypeObject * pType = &s_type_ );
	virtual ~PyAvatarFilter() {}

	float latency() const;
	void latency( float newLatency );

	bool callback(	int whence,
					ScriptObject callbackFn,
					float extra = 0.f,
					bool shouldPassMissedBy = false );

	// Python Interface
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PyAvatarFilter, END );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, latency, latency );

	PY_RW_ATTRIBUTE_DECLARE( AvatarFilterSettings::s_latencyVelocity_,
		velLatency );
	PY_RW_ATTRIBUTE_DECLARE( AvatarFilterSettings::s_latencyMinimum_,
		minLatency );
	PY_RW_ATTRIBUTE_DECLARE( AvatarFilterSettings::s_latencyFrames_,
		latencyFrames );
	PY_RW_ATTRIBUTE_DECLARE( AvatarFilterSettings::s_latencyCurvePower_,
		latencyCurvePower );

	PY_AUTO_METHOD_DECLARE( RETOK,
							callback,
							ARG( int,
							CALLABLE_ARG( ScriptObject,
							OPTARG( float, 0.f,
							OPTARG( bool, false, END ) ) ) ) );

	PY_METHOD_DECLARE( py_debugMatrixes );

	// Implementation of PyFilter
	virtual AvatarFilter * pAttachedFilter();
	virtual const AvatarFilter * pAttachedFilter() const;

protected:
	// Implementation of PyFilter
	virtual AvatarFilter * getNewFilter();
};

PY_SCRIPT_CONVERTERS_DECLARE( PyAvatarFilter );

BW_END_NAMESPACE

#endif // PY_AVATAR_FILTER_HPP
