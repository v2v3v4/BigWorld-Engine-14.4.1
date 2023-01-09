#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for ParticleSystemAction.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for ParticleSystemAction.
 */
INLINE ParticleSystemAction::ParticleSystemAction() :
	delay_( 0.0f ),
	minimumAge_( 0.0f ),
	age_( 0.0f ),
	enabled_( true )
{
}


/**
 *	This is the destructor for ParticleSystemAction.
 */
INLINE ParticleSystemAction::~ParticleSystemAction()
{
	removeLateUpdates( this );
}


// -----------------------------------------------------------------------------
// Section: Methods for the ParticleSystemAction.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the delay attribute. The delay attribute
 *	is the time in seconds after the starting time of the action before which
 *	the action takes no further action.
 *
 *	The default value is 0.0 seconds.
 *
 *	@return	Time in seconds until the action is active.
 */
INLINE float ParticleSystemAction::delay( void ) const
{
	return delay_;
}


/**
 *	This is the Set-Accessor for the delay attribute. Setting the delay
 *	attribute automatically resets the age_ attribute to zero.
 *
 *	@param seconds	The new delay time in seconds.
 */
INLINE void ParticleSystemAction::delay( float seconds )
{
	delay_ = seconds;
	age_ = 0.0f;
}


/**
 *	This is the Get-Accessor for the minimumAge attribute. The minimumAge
 *	attribute is the minimum age for a particle before the action will act
 *	upon it.
 *
 *	The default value is 0.0 seconds.
 *
 *	@return	The minimum particle age in seconds.
 */
INLINE float ParticleSystemAction::minimumAge( void ) const
{
	return minimumAge_;
}


/**
 *	This is the Set-Accessor for the minimumAge attribute.
 *
 *	@param seconds	The new minimumAge in seconds.
 */
INLINE void ParticleSystemAction::minimumAge( float seconds )
{
	minimumAge_ = seconds;
}


/**
 *  This is the Get-Accessor the GUI name attribute.
 */
INLINE const BW::string & ParticleSystemAction::name() const
{
    return name_;
}


/**
 *  This is the Set-Accessor the GUI name attribute.
 */
INLINE void ParticleSystemAction::name( const BW::string & str )
{
    name_ = str;
}


/**
 *	This is the Get-Accessor for the particle system actions's preview visible flag.
 *  If this flag is set then the particle system action will not effect the particle
 *	system.
 *
 *	@return	The current value of the particle system's visible flag.
 */
INLINE bool ParticleSystemAction::enabled( void ) const
{
	return enabled_;
}


/**
 *	This is the Get-Accessor for the particle system action's preview visible
 *	property.
 *
 *	@param state	The new value of the particle system's visible flag.
 */
INLINE void ParticleSystemAction::enabled( bool state )
{
	enabled_ = state;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyParticleSystemAction.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyParticleSystemAction.
 *
 *	@param 
 *	@param pType			Parameters passed to the parent PyObject class.
 */
INLINE PyParticleSystemAction::PyParticleSystemAction( ParticleSystemActionPtr pAction, PyTypeObject *pType ) :
	PyObjectPlus( pType ),
	pA_( pAction )
{
	IF_NOT_MF_ASSERT_DEV( pA_.hasObject() )
	{
		MF_EXIT( "PyParticleSystemAction: pAction is NULL" );
	}
}


// -----------------------------------------------------------------------------
// Section: Methods for the PyParticleSystemAction.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the delay attribute. The delay 
 *	attribute is the time in seconds after the starting time of the 
 *	action before which the action takes no further action.
 *
 *	The default value is 0.0 seconds.
 *
 *	@return	Time in seconds until the action is active.
 */
INLINE float PyParticleSystemAction::delay( void ) const
{
	return pA_->delay();
}


/**
 *	This is the python Set-Accessor for the delay attribute. Setting the
 *	delay attribute automatically resets the age_ attribute to zero.
 *
 *	@param seconds	The new delay time in seconds.
 */
INLINE void PyParticleSystemAction::delay( float seconds )
{
	pA_->delay( seconds );
}


/**
 *	This is the python Get-Accessor for the minimumAge attribute. The 
 *	minimumAge attribute is the minimum age for a particle before the 
 *	action will act upon it.
 *
 *	The default value is 0.0 seconds.
 *
 *	@return	The minimum particle age in seconds.
 */
INLINE float PyParticleSystemAction::minimumAge( void ) const
{
	return pA_->minimumAge();
}


/**
 *	This is the python Set-Accessor for the minimumAge attribute.
 *
 *	@param seconds	The new minimumAge in seconds.
 */
INLINE void PyParticleSystemAction::minimumAge( float seconds )
{
	pA_->minimumAge( seconds );
}

/* particle_system_action.ipp */
