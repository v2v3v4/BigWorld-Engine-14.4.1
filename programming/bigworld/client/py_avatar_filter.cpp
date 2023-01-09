#include "pch.hpp"

#include "py_avatar_filter.hpp"

#include "avatar_filter_callback.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

namespace
{
/**
 *	This class implements AvatarFilterCallback to pass the
 *	callback on to a Python callable object
 */
// This is an AvatarFilterCallback for PyAvatarFilter, not
// a Python wrapper around AvatarFilterCallback...
class PyAvatarFilterCallback : public AvatarFilterCallback
{
public:
	/**
	 *	Constructor
	 *
	 *	@param	targetTime	The time in the filter's playback that the python
	 *						function should be called.
	 *	@param	callbackFn	The python object that will be called when the
	 *						specified time is reached.
	 *	@param	shouldPassMissedBy
							True if the callback function wishes to receive the
	 *						difference between the call time and the call time
	 *						requested as a parameter.
	 */
	PyAvatarFilterCallback( double targetTime, ScriptObject callbackFn,
		bool shouldPassMissedBy ) :
		AvatarFilterCallback( targetTime ),
		callbackFn_( callbackFn ),
		shouldPassMissedBy_( shouldPassMissedBy )
	{
		BW_GUARD;

		MF_ASSERT( callbackFn_.isCallable() );
	}

	bool onCallback( double missedBy )
	{
		BW_GUARD;

		bool result = false;

		// Just in case it ceased to be callable
		if (!callbackFn_.isCallable())
		{
			return result;
		}

		ScriptArgs callbackArgs;

		if (shouldPassMissedBy_)
		{
			callbackArgs = ScriptArgs::create( float( missedBy ) );
		}
		else
		{
			callbackArgs = ScriptArgs::none();
		}

		ScriptObject callbackResult =
			callbackFn_.callFunction( callbackArgs, ScriptErrorPrint() );

		if (!callbackResult)
		{
			return result;
		}

		if (ScriptInt::check( callbackResult ))
		{
			result = (ScriptInt::create( callbackResult ).asLong() == 1L);
		}

		return result;
	}


	ScriptObject callbackFn_;
	bool shouldPassMissedBy_;
};
}


PY_TYPEOBJECT( PyAvatarFilter )

PY_BEGIN_METHODS( PyAvatarFilter )
	/*~ function AvatarFilter.callback
	 *
	 *	This method adds a callback function to the filter.  This function will
	 *	be called at a time after the event specified by the whence argument.
	 *	The amount of time after the event is specified, in seconds, by the
	 *	extra argument.
	 *
	 *	If whence is -1, then the event is when the filter's
	 *	timeline reaches the penultimate update (i.e. just before the current
	 *	update position starts influencing the entity).  If whence is 0, then
	 *	the event is when the current update is reached.  If whence is 1
	 *	then the event is when the timeline reaches the time that
	 *	the callback was specified.
	 *
	 *	The function will get one argument, which will be zero if the
	 *	passMissedBy argument is zero, otherwise it will be the amount
	 *	of time that passed between the time specified for the callback
	 *	and the time it was actually called at.
	 *
	 *	<b>Note:</b> When the callback is made, the entity position will
	 *			already have been set for the frame (so it'll be at some spot
	 *			after the desired callback time), but the motors will not yet
	 *			have been run on the model to move it. The positions of other
	 *			entities may or may not yet have been updated for the frame.
	 *
	 *	<b>Note:</b>	If a callback function returns '1', the entity position
	 *			will be snapped to the position that the entity would have had
	 *			at the exact time the callback should have been called.
	 *
	 *	<b>Note:</b>	If this AvatarFilter is not currently active on an
	 *			Entity (e.g., the Entity has been destroyed) this will raise
	 *			a ValueError exception.
	 *
	 *	@param	whence	an integer (-1, 0, 1).  Which event to base the
	 *					callback on.
	 *	@param	pCallback
	 *					a callable object.  The function to call.  It should
	 *					take one integer argument.
	 *	@param	extra	a float.  The time after the event specified by whence
	 *					the function should be called at.
	 *	@param	shouldPassMissedBy
	 *					whether or not to pass to the function the time
	 *					it missed its ideal call-time by.
	 *
	 */
	PY_METHOD( callback )
	/*~ function AvatarFilter.debugMatrixes
	 *
	 *	This function returns a tuple of matrix providers that when applied to
	 *	a unit cube visualises the received position data being used by the
	 *	filter.
	 *
	 *	Note: These matrix providers hold references to the filter that
	 *	issued them.
	 *
	 *	@return tuple([MatrixProvider,])	A tuple of newly created matrix
	 *										providers
	 */
	PY_METHOD( debugMatrixes )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAvatarFilter )
	/*~ attribute AvatarFilter.latency
	 *
	 *	This attribute is the current latency applied to the entity.  That is,
	 *	how far in the past that entity is, relative to the current game time.
	 *
	 *	Latency always moves towards the "ideal" latency with a velocity of
	 *	velLatency, measured in seconds per second.
	 *
	 *	If an update has just come in from the server, then the ideal latency is
	 *	the time between the two most recent updates, otherwise it is
	 *	two times minLatency. If this filter is not attached to an entity in
	 *	the world, latency will be the maximum possible float value.
	 *
	 *	The position and yaw of the entity are linearly interpolated between
	 *	the positions and yaws in the last 8 updates using (time - latency).
	 *
	 *	@type float
	 */
	PY_ATTRIBUTE( latency )
	/*~ attribute AvatarFilter.velLatency
	 *
	 *	This attribute is the speed, in seconds per second, that the latency
	 *	attribute can change at, as it moves towards its ideal latency.
	 *	If an update has just come in from the server, then the ideal latency is
	 *	the time between the two most recent updates, otherwise it is
	 *	two times minLatency.
	 *
	 *	@type float
	 */
	PY_ATTRIBUTE( velLatency )
	/*~ attribute AvatarFilter.minLatency
	 *
	 *	This attribute is the minimum bound for latency.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( minLatency )
	/*~ attribute AvatarFilter.latencyFrames
	 *
	 *
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( latencyFrames )
	/*~ attribute AvatarFilter.latencyCurvePower
	 *
	 *	The power used to scale the latency velocity |latency - idealLatency|^power
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( latencyCurvePower )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyAvatarFilter )

/*~ function BigWorld.AvatarFilter
 *
 *	This function creates a new AvatarFilter, which is used to
 *	interpolate between the position and yaw updates from the server
 *	for its owning entity.
 *
 *	@return	A new AvatarFilter object
 */
PY_FACTORY_NAMED( PyAvatarFilter, "AvatarFilter", BigWorld )


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 *					The default is 'AvatarFilter' but it may be overridden by
 *					derived types like AvatarDropFilter.
 */
PyAvatarFilter::PyAvatarFilter( PyTypeObject * pType ) :
	PyFilter( pType )
{
	BW_GUARD;
}


/**
 *	This method retrieves the latency of the filter attached to our
 *	entity if we own it, or std::numeric_limits<float>::max() otherwise.
 */
float PyAvatarFilter::latency() const
{
	BW_GUARD;

	const AvatarFilter * pFilter = this->pAttachedFilter();

	if (pFilter == NULL)
	{
		return std::numeric_limits<float>::max();
	}

	return pFilter->latency();
}


/**
 *	This method updates the latency of the filter attached to the target
 *	entity if we own it, and does nothing otherwise.
 */
void PyAvatarFilter::latency( float newLatency )
{
	BW_GUARD;

	AvatarFilter * pFilter = this->pAttachedFilter();

	if (pFilter == NULL)
	{
		return;
	}

	pFilter->latency( newLatency );
}


/**
 *	Call the given function when the filter's timeline reaches a given
 *	point in time, plus or minus any extra time. The point can be either
 *	-1 for the penultimate update time (i.e. just before current position
 *	update begins influencing the entity), 0 for the last update time
 *	(i.e. when at the current position update), or 1 for the game time
 *	(i.e. some time after the current position update is at its zenith).
 *
 *	@note	When the callback is made, the entity position will already have
 *			been set for the frame (so it'll be at some spot after the desired
 *			callback time), but the motors will not yet have been run on the
 *			model to move it. The positions of other entities may or may not
 *			yet have been updated for the frame.
 *	@note	If a callback function returns '1', the entity position will be
 *			snapped to the position that the entity would have had at the
 *			exact time the callback should have been called.
 */
bool PyAvatarFilter::callback(	int whence,
								ScriptObject callbackFn,
								float extra,
								bool shouldPassMissedBy )
{
	BW_GUARD;

	MF_ASSERT( callbackFn.isCallable() );

	AvatarFilter * pFilter = this->pAttachedFilter();

	if (pFilter == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "AvatarFilter.Callback(): "
			"AvatarFilter is not active on an Entity" );
		return false;
	}

	double targetTime;

	switch ( whence )
	{
		case -1:	targetTime = pFilter->getStoredInput( 7 ).time_;	break;
		case  0:	targetTime = pFilter->getStoredInput( 0 ).time_;	break;
		case  1:	targetTime = PyFilter::getTimeNow();				break;
		// Next output - undocumented
		default:	targetTime = 0.0; 									break;
	}

	targetTime += extra;

	pFilter->addCallback(
		new PyAvatarFilterCallback( targetTime, callbackFn,
			shouldPassMissedBy ) );

	return true;
}


/**
 *	This class implements a matrix provider for the purpose of visualising the
 *	input data stored by the AvatarFilter.
 */
class AvatarFilterDebugMatrixProvider : public MatrixProvider
{
	Py_Header( AvatarFilterDebugMatrixProvider, MatrixProvider )

public:
	AvatarFilterDebugMatrixProvider(	PyAvatarFilter * pPyAvatarFilter,
										int inputIndex,
										PyTypeObject * pType = &s_type_ ) :
			pPyAvatarFilter_( pPyAvatarFilter ),
			inputIndex_( inputIndex ),
			MatrixProvider( false, pType ) {}

	virtual void matrix( Matrix & m ) const
	{
		BW_GUARD;

		AvatarFilter * pAvatarFilter = pPyAvatarFilter_->pAttachedFilter();

		if (pAvatarFilter == NULL)
		{
			return;
		}

		const AvatarFilterHelper::StoredInput & storedInput =
			pAvatarFilter->getStoredInput( inputIndex_ );

		// Lower-bound of the position error is 0.05f so that low-error boxes
		// are not completely invisible.
		Vector3 visibleError(
			std::max( storedInput.positionError_.x, 0.05f ),
			std::max( storedInput.positionError_.y, 0.05f ),
			std::max( storedInput.positionError_.z, 0.05f ) );

		m.setScale( visibleError * 2 );
		Matrix unitCubeOffset;
		unitCubeOffset.setTranslate( -visibleError );
		m.postMultiply( unitCubeOffset );

		Position3D position( storedInput.position_ );
		// Error box is axis-aligned to the local position, so we don't
		// rotate our matrix to match storedInput.direction_
		Direction3D vehicleDirection( Vector3::zero() );
		pAvatarFilter->transformIntoCommon( storedInput.spaceID_,
			storedInput.vehicleID_, position, vehicleDirection );

		Matrix rotation;
		rotation.setRotate( vehicleDirection.yaw, vehicleDirection.pitch,
			vehicleDirection.roll );
		m.postMultiply( rotation );

		Matrix translation;
		translation.setTranslate( position );
		m.postMultiply( translation );
	}


private:
	const SmartPointer< PyAvatarFilter >	pPyAvatarFilter_;
	const int							inputIndex_;
};

PY_TYPEOBJECT( AvatarFilterDebugMatrixProvider )

PY_BEGIN_METHODS( AvatarFilterDebugMatrixProvider )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( AvatarFilterDebugMatrixProvider )
PY_END_ATTRIBUTES()


/**
 *	This is a python exposed function to provide debug information in the form
 *	of bounding boxes of received inputs for use in visualisation of the
 *	filter's behaviour.
 */
PyObject * PyAvatarFilter::py_debugMatrixes( PyObject * params )
{
	BW_GUARD;
	ScriptTuple result = ScriptTuple::create( AvatarFilter::NUM_STORED_INPUTS );

	for (int i = 0; i < AvatarFilter::NUM_STORED_INPUTS; i++)
	{
		ScriptObject newMatrix( new AvatarFilterDebugMatrixProvider( this, i ),
			ScriptObject::FROM_NEW_REFERENCE );

		result.setItem( i, newMatrix );
	}

	return result.newRef();
}


/**
 *	This method returns the AvatarFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
AvatarFilter * PyAvatarFilter::pAttachedFilter()
{
	BW_GUARD;

	return static_cast< AvatarFilter * >( this->PyFilter::pAttachedFilter() );
}


/**
 *	This method returns the const AvatarFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
const AvatarFilter * PyAvatarFilter::pAttachedFilter() const
{
	BW_GUARD;

	return static_cast< const AvatarFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	This method provides a new AvatarFilter
 */
AvatarFilter * PyAvatarFilter::getNewFilter()
{
	BW_GUARD;

	return new AvatarFilter( this );
}


BW_END_NAMESPACE

// py_avatar_filter.cpp
