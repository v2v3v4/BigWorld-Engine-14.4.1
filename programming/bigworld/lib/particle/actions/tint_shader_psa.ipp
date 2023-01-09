#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for TintShaderPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for TintShaderPSA.
 */
INLINE TintShaderPSA::TintShaderPSA() :
	repeat_( false ),
	period_( 1.0f ),
	fogAmount_( 0.f ),
	modulator_( NULL )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to TintShaderPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This method removes all tint values from the particles.
 */
INLINE void TintShaderPSA::clearAllTints( void )
{
	tints_.clear();
	period_ = 0.0f;
}


/**
 *	This is the Get-Accessor for the repeat attribute of TintShaderPSA. If this
 *	flag is true, the age of the particle will be in modular total time of the
 *	tint sequence.
 *
 *	@return	The repeat flag's current value.
 */
INLINE bool TintShaderPSA::repeat( void ) const
{
	return repeat_;
}


/**
 *	This is the Set-Accessor for the repeat attribute of TintShaderPSA.
 *
 *	@param flag		The new value for the flag.
 */
INLINE void TintShaderPSA::repeat( bool flag )
{
	repeat_ = flag;
}


/**
 *	This is the Get-Accessor for the period attribute of TintShaderPSA. This
 *	value represents the time that it takes for the particle to go through
 *	all of the TintShaderPSA's tints.
 *
 *	@return	The period's current value.
 */
INLINE float TintShaderPSA::period() const
{
	return period_;
}


/**
 *	This is the Set-Accessor for the period attribute of TintShaderPSA.
 *
 *	@param p		The new value for the period.
 */
INLINE void TintShaderPSA::period( float p )
{
	period_ = p;
}

/**
 *	This is the Set-Accessor for the fogAmount attribute of TintShaderPSA.
 *	This sets the amount of fog colour to blend in with the animated colour.
 *
 *	@param fogAmount	The new amount of fog to blend into the colour.
 */
INLINE void TintShaderPSA::fogAmount( float f )
{
	fogAmount_ = f;
}


/**
 *	This is the Get-Accessor for the fog attribute of TintShaderPSA.
 *	It returns the amount of the fog colour that is blended in with
 *	the animated colour.
 *
 *	@return	The amount of fog that is blended in with the colour.
 */
INLINE float TintShaderPSA::fogAmount( void ) const
{
	return fogAmount_;
}


/**
 *	This is the Get-Accessor for the modulator attribute of TintShaderPSA. 
 *
 *	@return	The current value of the modulator as a Vector4ProviderPtr.
 */
INLINE Vector4ProviderPtr TintShaderPSA::modulator( void ) const
{
	return modulator_;
}


/**
 *	This is the Set-Accessor for the modulator attribute of TintShaderPSA.
 *
 *	@param vector		The new Vector4ProviderPtr for the modulator.
 */
INLINE void TintShaderPSA::modulator( Vector4ProviderPtr vector )
{
	modulator_ = vector;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int TintShaderPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & TintShaderPSA::nameID( void ) const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyTintShaderPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyTintShaderPSA.
 */
INLINE PyTintShaderPSA::PyTintShaderPSA( TintShaderPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyTintShaderPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the repeat attribute of TintShaderPSA. If this
 *	flag is true, the age of the particle will be in modular total time of the
 *	tint sequence.
 *
 *	@return	The repeat flag's current value.
 */
INLINE bool PyTintShaderPSA::repeat( void ) const
{
	return pAction_->repeat();
}


/**
 *	This is the python Set-Accessor for the repeat attribute of TintShaderPSA.
 *
 *	@param flag		The new value for the flag.
 */
INLINE void PyTintShaderPSA::repeat( bool flag )
{
	pAction_->repeat( flag );
}


/**
 *	This is the python Set-Accessor for the period attribute of TintShaderPSA.
 *
 *	@param period	The new period for the keyframe colour animation.
 */
INLINE void PyTintShaderPSA::period( float p )
{
	pAction_->period(p);
}


/**
 *	This is the python Get-Accessor for the period attribute of TintShaderPSA.
 *	It returns the time, in seconds, of the period of the keyframe
 *	colour animation.
 *
 *	@return	The period of the keyframe colour animation.
 */
INLINE float PyTintShaderPSA::period( void ) const
{
	return pAction_->period();
}


/**
 *	This is the python Set-Accessor for the fogAmount attribute of TintShaderPSA.
 *	This sets the amount of fog colour to blend in with the animated colour.
 *
 *	@param fogAmount	The new amount of fog to blend into the colour.
 */
INLINE void PyTintShaderPSA::fogAmount( float f )
{
	pAction_->fogAmount( f );
}


/**
 *	This is the python Get-Accessor for the fog attribute of TintShaderPSA.
 *	It returns the amount of the fog colour that is blended in with
 *	the animated colour.
 *
 *	@return	The amount of fog that is blended in with the colour.
 */
INLINE float PyTintShaderPSA::fogAmount( void ) const
{
	return pAction_->fogAmount();
}


/**
 *	This is the python Get-Accessor for the modulator attribute of TintShaderPSA. 
 *
 *	@return	The current value of the modulator as a Vector4ProviderPtr.
 */
INLINE Vector4ProviderPtr PyTintShaderPSA::modulator( void ) const
{
	return pAction_->modulator();
}


/**
 *	This is the python Set-Accessor for the modulator attribute of TintShaderPSA.
 *
 *	@param vector		The new Vector4ProviderPtr for the modulator.
 */
INLINE void PyTintShaderPSA::modulator( Vector4ProviderPtr vector )
{
	pAction_->modulator( vector );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyTintShaderPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* tint_shader_psa.ipp */
