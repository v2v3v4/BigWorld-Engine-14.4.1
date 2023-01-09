#include "pch.hpp"
#include "py_sound_parameter.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"

#if FMOD_SUPPORT

#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PySoundParameter", 0 );


BW_BEGIN_NAMESPACE

/*~ class FMOD.SoundParameter
 *
 *  A SoundParameter provides access to sound event parameters and is
 *  basically a partial interface to FMOD::EventParameter.  For more information
 *  about event parameters and how they are used, please see the FMOD Designer API
 *  documentation and the FMOD Designer User Manual, both available from
 *  www.fmod.org.
 */
PY_TYPEOBJECT( PySoundParameter );

PY_BEGIN_METHODS( PySoundParameter )
    PY_METHOD( keyOff )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySoundParameter )

	/*~ attribute SoundParameter.min
	 *
	 *  The minimum legal value for this parameter.
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( min )

	/*~ attribute SoundParameter.max
	 *
	 *  The maximum legal value for this parameter.
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( max )

	/*~ attribute SoundParameter.value
	 *
	 *  The current value of this parameter.
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( value )

	/*~ attribute SoundParameter.velocity
	 *
	 *  The current velocity of this parameter.  Please see the documentation
	 *  for FMOD::EventParameter::setVelocity() for details on the mechanics and
	 *  usage of EventParameter velocities.
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( velocity )

	/*~ attribute SoundParameter.seekSpeed
	 *
	 *  The current seek speed of this parameter.  Please see the documentation
	 *  for FMOD::EventParameter::setSeekSpeed() for details on the mechanics and
	 *  usage of EventParameter seek speeds.
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( seekSpeed )

	/*~ attribute SoundParameter.name
	 *
	 *  The name of this parameter.
	 */
	PY_ATTRIBUTE( name )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PySoundParameter )
PY_SCRIPT_CONVERTERS( PySoundParameter )

static PyFactoryMethodLink PySoundParameter_MethodLink("_FMOD", "SoundParameter", &PySoundParameter::s_type_);

PySoundParameter::PySoundParameter( SoundManager::EventParameter * pParam,
	PySound * pSound, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pParam_( pParam ),
	minimum_( 0 ),
	maximum_( 0 ),
	pSound_( pSound ),
	pEvent_( pSound->pEvent_ ),
	index_( 0 )
{
	BW_GUARD;	
	FMOD_RESULT result;

	result = pParam_->getRange( &minimum_, &maximum_ );
    SoundManager::FMOD_ErrCheck( result, "PySoundParameter::PySoundParameter: "
			"Couldn't get min/max for %s",
			this->name());

	result = pParam_->getInfo( &index_, NULL );
	SoundManager::FMOD_ErrCheck( result,  "PySoundParameter::PySoundParameter: "
			"Couldn't get index");
}


PySoundParameter::~PySoundParameter()
{
}


/**
 *  Getter for parameter value.
 */
float PySoundParameter::value()
{
	BW_GUARD;
	if (!this->refresh())
		return -1;

	float value;
	FMOD_RESULT result = pParam_->getValue( &value );

    return SoundManager::FMOD_ErrCheck(result, "PySoundParameter::value( %s )",
            this->name()) ? value : -1.f;
}


/**
 *  Setter for parameter value.
 */
void PySoundParameter::value( float value )
{
	BW_GUARD;
	if (!this->refresh())
		return;

	if (value < minimum_ || value > maximum_)
	{
		ERROR_MSG( "PySoundParameter::value( %s ): "
			"Value %f is outside valid range [%f,%f]\n",
			this->name(), value, minimum_, maximum_ );
		return;
	}

	FMOD_RESULT result = pParam_->setValue( value );

    SoundManager::FMOD_ErrCheck( result, "PySoundParameter::value( %s )",
			this->name());
}


/**
 *  Getter for parameter velocity.
 */
float PySoundParameter::velocity()
{
	BW_GUARD;
	if (!this->refresh())
		return -1;

	float velocity;
	FMOD_RESULT result = pParam_->getVelocity( &velocity );

    if (!SoundManager::FMOD_ErrCheck( result, "PySoundParameter::velocity( %s )", this->name()))
        return -1.f;
    return velocity;
}

/**
 *  Setter for parameter velocity.
 */
void PySoundParameter::velocity( float velocity )
{
	BW_GUARD;
	if (!this->refresh())
		return;

	FMOD_RESULT result = pParam_->setVelocity( velocity );

	SoundManager::FMOD_ErrCheck( result, "PySoundParameter::velocity( %s )",
			this->name());
}


/**
 *  Getter for parameter seek speed.
 */
float PySoundParameter::seekSpeed()
{
	BW_GUARD;
	if (!this->refresh())
		return -1;

	float seekSpeed;
	FMOD_RESULT result = pParam_->getSeekSpeed( &seekSpeed );

    if (!SoundManager::FMOD_ErrCheck( result, "PySoundParameter::seekSpeed( %s )", this->name()))
        return -1.f;
    return seekSpeed;
}

/**
 *  Setter for parameter seek speed.
 */
void PySoundParameter::seekSpeed( float newSeekSpeed )
{
	BW_GUARD;
	if (!this->refresh())
		return;

	FMOD_RESULT result = pParam_->setSeekSpeed( newSeekSpeed );

	SoundManager::FMOD_ErrCheck( result, "PySoundParameter::seekSpeed( %s )",
			this->name());
}


/**
 *  Triggers a keyoff on an event parameter that has sustain points in it.
 */
void PySoundParameter::keyOff()
{
	BW_GUARD;
	if (!this->refresh())
		return;

	FMOD_RESULT result = pParam_->keyOff();

	SoundManager::FMOD_ErrCheck( result, "PySoundParameter::keyOff( %s )",
			this->name());
}

/**
 *  Get the name of this parameter.  Uses memory managed by FMOD so don't expect
 *  it to be around for long.
 */
const char *PySoundParameter::name() const
{
	BW_GUARD;
	static const char *err = "<error>";
	char *name;

	FMOD_RESULT result = pParam_->getInfo( NULL, &name );

    return SoundManager::FMOD_ErrCheck( result , "PySoundParameter::name" ) ? name : err;
}


/**
 *  Ensure that the FMOD::EventParameter* handle in this object actually
 *  corresponds to the FMOD::Event* stored in the PySound associated with this
 *  object.
 */
bool PySoundParameter::refresh()
{
	BW_GUARD;
	// Make sure the sound is up-to-date
	if (!pSound_->refresh( PySoundParameter::REFRESH_MASK ))
		return false;

	// If the Event* hasn't changed, we can break now
	if (pSound_->pEvent_ == pEvent_)
		return true;

	// If we haven't returned yet, then we need to get a new reference.
	pEvent_ = pSound_->pEvent_;
	FMOD_RESULT result = pEvent_->getParameterByIndex( index_, &pParam_ );

    return SoundManager::FMOD_ErrCheck( result, "PySoundParameter::refresh: "
			"Couldn't re-acquire parameter handle for %s",
			this->name());
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// pysoundparameter.cpp
