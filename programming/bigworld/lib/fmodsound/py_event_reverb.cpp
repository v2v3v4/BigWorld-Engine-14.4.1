#include "pch.hpp"
#include "py_event_reverb.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"

#if FMOD_SUPPORT
#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PyEventReverb", 0 );


BW_BEGIN_NAMESPACE

/*~ class FMOD.EventReverb
 *
 *  A EventReverb provides access to FMOD Event Reverbs.
 */
PY_TYPEOBJECT( PyEventReverb );

PY_BEGIN_METHODS( PyEventReverb ) 
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEventReverb )

	/*~ attribute EventReverb.active
	 *
	 *  Activates/deactivates this reverb effect within the 3D world.
	 *
	 *  @type	Read-write boolean
	 */
	PY_ATTRIBUTE( active )

	/*~ attribute EventReverb.position
	 *
	 *  The centre position of the reverb. This is overwritten with the 
	 *  output of the source attribute if it has been specified.
	 *
	 *  @type	Read-write Vector3
	 */
    PY_ATTRIBUTE( position )

	/*~ attribute EventReverb.source
	 *
	 *  The matrix provider which will be used to determine the location
	 *	of the reverb within the world. This overwrites the position 
	 *	attribute.
	 *
	 *  @type	Read-write MatrixProvider
	 */
    PY_ATTRIBUTE( source )

	/*~ attribute EventReverb.maxDistance
	 *
	 *  The minimum distance the listener has to be to this reverb
	 *	object before the reverb effect is applied at full volume.
	 *
	 *  @type	Read-write float
	 */
    PY_ATTRIBUTE( minDistance )

	/*~ attribute EventReverb.maxDistance
	 *
	 *  The maximum distance at which the reverb affects the listener. The
	 *	reverb effect is not audible outside this radius.
	 *
	 *  @type	Read-write float
	 */
    PY_ATTRIBUTE( maxDistance )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyEventReverb )
PY_SCRIPT_CONVERTERS( PyEventReverb )

PY_FACTORY_NAMED( PyEventReverb, "EventReverb", _FMOD )

PyEventReverb::PyEventReverb( SoundManager::EventReverb * pEventReverb, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
        eventReverb_( pEventReverb ),
        minDistance_( 0 ),
        maxDistance_( 0 ),
		position_( 0,0,0 ),
		source_( NULL ),
		active_( false )
{
	BW_GUARD;	
}

PyEventReverb::~PyEventReverb()
{
	BW_GUARD;
	fini();
}

void PyEventReverb::fini()
{
	if (eventReverb_)
	{
		FMOD_RESULT result = eventReverb_->release();
		SoundManager::FMOD_ErrCheck( result, "PyEventReverb::fini" );
		eventReverb_ = NULL;
	}
}

bool PyEventReverb::active() const
{
    BW_GUARD;
	if (eventReverb_)
	{
		FMOD_RESULT result = eventReverb_->getActive( &active_ );
		SoundManager::FMOD_ErrCheck(result, "PyEventReverb::active");
	}

	return active_;
}

void PyEventReverb::active( bool active )
{
    BW_GUARD;
	active_ = active;

	if (eventReverb_)
	{
		FMOD_RESULT result = eventReverb_->setActive( active_ );
		SoundManager::FMOD_ErrCheck(result, "PyEventReverb::active");
	}	
}

float PyEventReverb::minDistance() const
{
    BW_GUARD;
    return minDistance_;
}

void PyEventReverb::minDistance( float newMinDistance )
{
    BW_GUARD;
    minDistance_ = newMinDistance;
    update3DAttributes();
}

float PyEventReverb::maxDistance() const
{
    BW_GUARD;
    return maxDistance_;
}

void PyEventReverb::maxDistance( float newMaxDistance )
{
    BW_GUARD;
    maxDistance_ = newMaxDistance;
    update3DAttributes();
}

const Vector3 & PyEventReverb::position() const
{
	return position_;
}

void PyEventReverb::position( const Vector3 & v )
{
	position_ = v;
}

/**
 *	Specialised get method for the 'source' attribute
 */
PyObject * PyEventReverb::pyGet_source()
{
	BW_GUARD;
	return Script::getReadOnlyData( source_ );
}

/**
 *	Specialised set method for the 'source' attribute
 */
int PyEventReverb::pySet_source( PyObject * value )
{
	BW_GUARD;
    int success = Script::setData( value, source_ );
    update3DAttributes();
    return success;
}


void PyEventReverb::update3DAttributes()
{
    BW_GUARD;
	if (source_)
	{
		Matrix m;
		source_->matrix(m);
		position_ = m.applyToOrigin();
	}

	if (eventReverb_)
	{
		FMOD_RESULT result = eventReverb_->set3DAttributes( (FMOD_VECTOR *)&position_, minDistance_, maxDistance_ );
		SoundManager::FMOD_ErrCheck(result, "PyEventReverb::update3DAttributes");
	}
}

PyObject *PyEventReverb::pyNew( PyObject *args )
{
	BW_GUARD;

	char* presetName;
	if (!PyArg_ParseTuple( args, "s", &presetName ))
	{
		PyErr_SetString( PyExc_TypeError, "FMOD.EventReverb() "
			"expects a string reverb preset name argument" );
		return NULL;
	}

	PyObject* reverb = 
		SoundManager::instance().pyEventReverb( presetName );

	if (SoundManager::errorLevel() != SoundManager::ERROR_LEVEL_EXCEPTION &&
		(reverb == Py_None || !reverb))
	{
		Py_XDECREF(reverb);

		// Always return a valid instance if we aren't throwing an exception
		// since we're in a type constructor!
		reverb = new PyEventReverb(NULL);
	}

	return reverb;
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// py_event_reverb.cpp
