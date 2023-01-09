#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for SourcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for SourcePSA.
 *
 *	@param pPositionSrc		Pointer to the position vector generator.
 *	@param pVelocitySrc		Pointer to the velocity vector generator.
 */
INLINE SourcePSA::SourcePSA( VectorGenerator *pPositionSrc,
		VectorGenerator *pVelocitySrc ) :
	motionTriggered_( false ),
	timeTriggered_( false ),
	grounded_( false ),
	rate_( 0.0f ),
	sensitivity_( 0.1f ),
	maxSpeed_( 0.f ),
	activePeriod_( 1.0f ),
	sleepPeriod_( 0.0f ),
	sleepPeriodMax_( -1.0f ),
	minimumSize_( 1.0f ),
	maximumSize_( 1.0f ),
	forcedUnitSize_( 1 ),
	allowedTime_( stampsPerSecond() ),
	pPositionSrc_( pPositionSrc ),
	pVelocitySrc_( pVelocitySrc ),
	queuedTotal_( 0 ),
	forcedTotal_( 0 ),
	periodTime_( 0.0f ),
	accumulatedTime_( 0.0f ),
	lastPositionOfPS_( 0.0f, 0.0f, 0.0f ),
	firstUpdate_( true ),
	dropDistance_( 15.0f ),
	pGS_( NULL ),
	currentTime_( 0 ),
	initialRotation_( 0.f, 0.f ),
	randomInitialRotation_( 0.f, 0.f ),
	initialColour_( 0.5f, 0.5f, 0.5f, 1.f ),
	ignoreRotation_( false ),
	randomSpin_( false ),
	minSpin_( 0.f ),
	maxSpin_( 0.f ),
	inheritVelocity_( 0.f ),
	currentSleepPeriod_( 0.f )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to SourcePSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor to the motionTrigger flag. If set to true, the
 *	source will generate particles when the model to which it is attached
 *	moves. The amount of particles generated is dependent on the sensitivity
 *	of the source.
 *
 *	@return The motionTrigger flag value.
 */
INLINE bool SourcePSA::motionTriggered( void ) const
{
	return motionTriggered_;
}


/**
 *	This is the Get-Accessor to the timeTriggered flag. If set to true, the
 *	source will generate particles over time according to the rate set.
 *	No particles will be generated when the particle is in its sleep period.
 *
 *	@return	The timeTriggered flag value.
 */
INLINE bool SourcePSA::timeTriggered( void ) const
{
	return timeTriggered_;
}


/**
 *	This is the Get-Accessor to the ground specifier for the particle
 *	source action. The ground-specifier is what defines the ground drop
 *	point for the grounded particles.
 *
 *	@return A pointer to the ground specifier used by the action.
 */
INLINE RompColliderPtr SourcePSA::groundSpecifier( void ) const
{
	return pGS_;
}


/**
 *	This is the Set-Accessor to the ground specifier for the particle source
 *	action.
 *
 *	@param pGS	The pointer to the ground specifier.
 */
INLINE void SourcePSA::groundSpecifier( RompColliderPtr pGS )
{
	pGS_ = pGS;
}


/**
 *	This is the Get-Accessor to the grounded flag. If set to true, each
 *	particle generated will be dropped to the terrain or scene below it.
 *	If there is no terrain or scene, nothing will be done.
 *
 *	@return The grounded flag value.
 */
INLINE bool SourcePSA::grounded( void ) const
{
	return grounded_;
}


/**
 *	This is the Set-Accessor to the grounded flag.
 *
 *	@param flag		The new value for the grounded flag.
 */
INLINE void SourcePSA::grounded( bool flag )
{
	grounded_ = flag;
}


/**
 *	This is the Get-Accessor for the dropDistance property. This is how far
 *	down to check for a collision.
 *
 *	@return	The vertical distance checked for ground.
 */
INLINE float SourcePSA::dropDistance( void ) const
{
	return dropDistance_;
}


/**
 *	This is the Set-Accessor for the dropDistance property.
 *
 *	@param newDistance	The new vertical distance checked for ground.
 */
INLINE void SourcePSA::dropDistance( float newDistance )
{
	dropDistance_ = newDistance;
}


/**
 *	This is the Get-Accessor for the rate property. This is the number of
 *	particles to generate per second over its active period.
 *
 *	@return	The number of particles generated per second.
 */
INLINE float SourcePSA::rate( void ) const
{
	return rate_;
}


/**
 *	This is the Set-Accessor for the rate property.
 *
 *	@param amount	The new number of particles to generate per second.
 */
INLINE void SourcePSA::rate( float amount )
{
	rate_ = amount;
}


/**
 *	This is the Get-Accessor for the sensitivity property. This is how
 *	sensitive the source is to movement on its model. The value is speed
 *	in metres per second.
 *
 *	No particles are generated if the particle system moved at less than
 *	the sensitivity. If the particle system moved at N times sensitivity,
 *	then (N-1)*2 particles would be generated.
 *
 *	@return	The number of metres moved before a particle is generated.
 */
INLINE float SourcePSA::sensitivity( void ) const
{
	return sensitivity_;
}


/**
 *	This is the Get-Accessor for the activePeriod property. This is the length
 *	in seconds where the source will generate particles at the rate given when
 *	its timeTriggered flag is true.
 *
 *	@return Period in seconds of activity when timeTriggered is true.
 */
INLINE float SourcePSA::activePeriod( void ) const
{
	return activePeriod_;
}


/**
 *	This is the Set-Accessor for the activePeriod property.
 *
 *	@param timeInSeconds	The new period in seconds.
 */
INLINE void SourcePSA::activePeriod( float timeInSeconds )
{
	activePeriod_ = timeInSeconds;
}


/**
 *	This is the Get-Accessor for the sleepPeriod property. This is the length
 *	in seconds where the source will rest when its timeTriggered flag is true.
 *
 *	@return Period in seconds of inactivity when timeTriggered is true.
 */
INLINE float SourcePSA::sleepPeriod( void ) const
{
	return sleepPeriod_;
}


/**
 *	This is the Set-Accessor for the sleepPeriod property.
 *
 *	@param timeInSeconds	The new period in seconds.
 */
INLINE void SourcePSA::sleepPeriod( float timeInSeconds )
{
	sleepPeriod_ = timeInSeconds;
	currentSleepPeriod_ = generateSleepPeriod();
}


/**
 *	This is the Get-Accessor for the sleepPeriodMax property. This is the maximum 
 *	length in seconds where the source will rest when its timeTriggered flag is true.
 *
 *	@return maximum Period in seconds of inactivity when timeTriggered is true.
 */
INLINE float SourcePSA::sleepPeriodMax( void ) const
{
	return sleepPeriodMax_;
}


/**
 *	This is the Set-Accessor for the sleepPeriodMax property.
 *
 *	@param timeInSeconds	The new max period in seconds.
 */
INLINE void SourcePSA::sleepPeriodMax( float timeInSeconds )
{
	sleepPeriodMax_ = timeInSeconds;
	currentSleepPeriod_ = generateSleepPeriod();
}


/**
 *	This is the gererator of the currentSleepPeriod
 */
INLINE float SourcePSA::generateSleepPeriod() const
{
	if ( (sleepPeriodMax() >= 0.f) && (sleepPeriod() >= 0.f) )
	{
		return fabsf(sleepPeriodMax() - sleepPeriod()) * unitRand() + sleepPeriod();
	}
	else if ( sleepPeriod() >= 0.f )
	{
		return sleepPeriod();
	}
	else if ( sleepPeriodMax() >= 0.f )
	{
		return sleepPeriodMax();
	}
	else
	{
		return 0.f;
	}
}


/**
 *	This is the Get-Accessor for the minimumSize property. The particles
 *	generated by this source range in size from minimumSize to maximumSize,
 *	with 1.0 being original texture size.
 *
 *	@return	The minimum size value.
 */
INLINE float SourcePSA::minimumSize( void ) const
{
	return minimumSize_;
}


/**
 *	This is the Get-Accessor for the maximumSize property. The particles
 *	generated by this source range in size from minimumSize to maximumSize,
 *	with 1.0 being original texture size.
 *
 *	@return	The maximum size value.
 */
INLINE float SourcePSA::maximumSize( void ) const
{
	return maximumSize_;
}


/**
 *	This is the Get-Accessor for the forcedUnitSize property. This number is
 *	the number of particles created for each unit of the forced total. The
 *	default value is set to 1.
 *
 *	@return	The number of particles created per forceTotal unit.
 */
INLINE int SourcePSA::forcedUnitSize( void ) const
{
	return forcedUnitSize_;
}


/**
 *	This is the Set-Accessor for the forcedUnitSize property.
 *
 *	@param newUnitSize	How many particles force(1) will create.
 */
INLINE void SourcePSA::forcedUnitSize( int newUnitSize )
{
	forcedUnitSize_ = newUnitSize;
}


/**
 *	This is the Get-Accessor for the allowedTime property. For grounded
 *	sources, the collision detection can take a long time. The allowedTime
 *	sets a maximum allowed time in seconds for the source to create its
 *	particles. The values supplied through the interface are in seconds,
 *	although the actual allowedTime is stored as CPU clock cycles (or
 *	timestamps.)
 *
 *	@return The maximum time allowed in a frame for the SourcePSA to create
 *			particles. Units in seconds.
 */
INLINE float SourcePSA::allowedTimeInSeconds( void ) const
{
	return float( double(allowedTime_) / stampsPerSecondD() );
}


/**
 *	This is the Set-Accessor for the allowedTime property.
 *
 *	@param timeInSeconds	This is the new allowedTime value.
 */
INLINE void SourcePSA::allowedTimeInSeconds( float timeInSeconds )
{
	allowedTime_ = uint64( double(timeInSeconds) * stampsPerSecondD() );
}


/**
 *	This is the Get-Accessor for the initialRotation_ property. 
 *
 *	@return	The initial rotation.
 */
INLINE Vector2 SourcePSA::initialRotation( void ) const
{
	return initialRotation_;
}


/**
 *	This is the Set-Accessor for the initialRotation_ property.
 *
 *	@param rotation	This is the new initialRotation value.
 */
INLINE void SourcePSA::initialRotation( const Vector2& rotation)
{
	initialRotation_ = rotation;
}


/**
 *	This is the Get-Accessor for the randomInitialRotation property. 
 *
 *	@return	The initial rotation randomisation amount.
 */
INLINE Vector2 SourcePSA::randomInitialRotation( void ) const
{
	return randomInitialRotation_;
}


/**
 *	This is the Set-Accessor for the randomInitialRotation property.
 *
 *	@param rotation	This is the new initial rotation randomisation amount.
 */
INLINE void SourcePSA::randomInitialRotation( const Vector2& rotation)
{
	randomInitialRotation_ = rotation;
}


/**
 *	This is the Get-Accessor for the initialColour_ property. 
 *
 *	@return	The initial colour.
 */
INLINE Vector4 SourcePSA::initialColour( void ) const
{
	return initialColour_;
}


/**
 *	This is the Set-Accessor for the initialColour_ property.
 *
 *	@param colour	This is the new initialColour_ value.
 */
INLINE void SourcePSA::initialColour( Vector4 colour)
{
	initialColour_ = colour;
}


/**
 *	This is the Get-Accessor for the randomSpin_ property. 
 *
 *	@return	The whether mesh particles should have a random spin.
 */
INLINE bool SourcePSA::randomSpin( void ) const	
{ 
	return randomSpin_; 
}


/**
 *	This is the Set-Accessor for the randomSpin_ property.
 *
 *	@param enable	This is the new randomSpin_ value.
 */
INLINE void SourcePSA::randomSpin(bool enable)	
{ 
	randomSpin_ = enable; 
}


/**
 *	This is the Get-Accessor for the minSpin_ property. 
 *
 *	@return	The minimum spin speed.
 */
INLINE float SourcePSA::minSpin( void ) const	
{ 
	return minSpin_; 
}


/**
 *	This is the Set-Accessor for the minSpin_ property.
 *
 *	@param amount	This is the new minSpin_ value.
 */
INLINE void SourcePSA::minSpin(float amount)	
{ 
	minSpin_ = amount; 
}


/**
 *	This is the Get-Accessor for the maxSpin_ property. 
 *
 *	@return	The maximum spin speed.
 */
INLINE float SourcePSA::maxSpin( void ) const	
{ 
	return maxSpin_; 
}


/**
 *	This is the Set-Accessor for the maxSpin_ property.
 *
 *	@param amount	This is the new maxSpin_ value.
 */
INLINE void SourcePSA::maxSpin(float amount)	
{ 
	maxSpin_ = amount; 
}
	
	
/**
 *	This is the Get-Accessor for the ignoreRotation_ property. 
 *
 *	@return	The ignoreRotation flag
 */
INLINE bool SourcePSA::ignoreRotation( void ) const
{
	return ignoreRotation_;
}


/**
 *	This is the Set-Accessor for the ignoreRotation_ property.
 *
 *	@param option	This is the new ignoreRotation_ flag value.
 */
INLINE void SourcePSA::ignoreRotation( bool option)
{
	ignoreRotation_ = option;
}


/**
 *	This is the Get-Accessor for the inheritVelocity_ property. 
 *
 *	@return	The inheritVelocity_ value.
 */
INLINE float SourcePSA::inheritVelocity() const 
{ 
	return inheritVelocity_; 
}


/**
 *	This is the Set-Accessor for the inheritVelocity_ property.
 *
 *	@param amount	This is the new inheritVelocity_ value.
 */
INLINE void SourcePSA::inheritVelocity(float amount) 
{ 
	inheritVelocity_ = amount; 
}


/**
 *	This is the Get-Accessor for the maxSpeed_ property. 
 *
 *	@return	The maxSpeed_ value.
 */
INLINE float SourcePSA::maxSpeed() const 
{ 
	return maxSpeed_; 
}


/**
 *	This is the Set-Accessor for the maxSpeed_ property.
 *
 *	@param amount	This is the new maxSpeed_ value.
 */
INLINE void SourcePSA::maxSpeed(float amount) 
{ 
	maxSpeed_ = amount; 
}


/**
 *	This method will create a number of particles over time according to the
 *	rate, sleepPeriod and activePeriod properties. No more than the required
 *	number will be created.
 *
 *	@param number	The number of particles to be created on demand over time.
 */
INLINE void SourcePSA::create( int number )
{
	queuedTotal_ += number;
}


/**
 *	This method will force the creation of a number of particles or particle
 *	chains on the next execute() invocation. No more than the required
 *	number will be created. The forcedUnitSize times this number is the number
 *	of particles in total. The forcedUnitSize is the length of separate chains
 *	that will be created, while the forced number stands for the number of
 *	actual chains.
 *
 *	@param number	The number of particles/chains to be forced.
 */
INLINE void SourcePSA::force( int number )
{
	forcedTotal_ += number;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int SourcePSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & SourcePSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PySourcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PySourcePSA.
 */
INLINE PySourcePSA::PySourcePSA( SourcePSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PySourcePSA properties.
// -----------------------------------------------------------------------------


/**
 *	This method returns the Source Action that we are the python wrapper for.
 *
 *	@return The Source Action.
 */
INLINE SourcePSAPtr PySourcePSA::pAction()
{
	return pAction_;
}


/**
 *	This is the python Get-Accessor to the motionTrigger flag. If set to true, the
 *	source will generate particles when the model to which it is attached
 *	moves. The amount of particles generated is dependent on the sensitivity
 *	of the source.
 *
 *	@return The motionTrigger flag value.
 */
INLINE bool PySourcePSA::motionTriggered( void ) const
{
	return pAction_->motionTriggered();
}


/**
 *	This is the python Set-Accessor to the motionTrigger flag.
 *
 *	@param flag		The new value for the motionTrigger flag.
 */
INLINE void PySourcePSA::motionTriggered( bool flag )
{
	pAction_->motionTriggered( flag );
}


/**
 *	This is the python Get-Accessor to the timeTriggered flag. If set to true, the
 *	source will generate particles over time according to the rate set.
 *	No particles will be generated when the particle is in its sleep period.
 *
 *	@return	The timeTriggered flag value.
 */
INLINE bool PySourcePSA::timeTriggered( void ) const
{
	return pAction_->timeTriggered();
}


/**
 *	This is the python Set-Accessor to the timeTriggered flag.
 *
 *	@param flag		The new value for the timeTriggered flag.
 */
INLINE void PySourcePSA::timeTriggered( bool flag )
{
	pAction_->timeTriggered( flag );
}


/**
 *	This is the python Get-Accessor to the grounded flag. If set to true, each
 *	particle generated will be dropped to the terrain or scene below it.
 *	If there is no terrain or scene, nothing will be done.
 *
 *	@return The grounded flag value.
 */
INLINE bool PySourcePSA::grounded( void ) const
{
	return pAction_->grounded();
}


/**
 *	This is the python Set-Accessor to the grounded flag.
 *
 *	@param flag		The new value for the grounded flag.
 */
INLINE void PySourcePSA::grounded( bool flag )
{
	pAction_->grounded( flag );
}


/**
 *	This is the python Get-Accessor for the dropDistance property. This is how far
 *	down to check for a collision.
 *
 *	@return	The vertical distance checked for ground.
 */
INLINE float PySourcePSA::dropDistance( void ) const
{
	return pAction_->dropDistance();
}


/**
 *	This is the python Set-Accessor for the dropDistance property.
 *
 *	@param amount	The new vertical distance checked for ground.
 */
INLINE void PySourcePSA::dropDistance( float newDistance )
{
	pAction_->dropDistance( newDistance );
}


/**
 *	This is the python Get-Accessor for the rate property. This is the number of
 *	particles to generate per second over its active period.
 *
 *	@return	The number of particles generated per second.
 */
INLINE float PySourcePSA::rate( void ) const
{
	return pAction_->rate();
}


/**
 *	This is the python Set-Accessor for the rate property.
 *
 *	@param amount	The new number of particles to generate per second.
 */
INLINE void PySourcePSA::rate( float amount )
{
	pAction_->rate( amount );
}


/**
 *	This is the python Get-Accessor for the sensitivity property. This is how
 *	sensitive the source is to movement on its model. The value is speed
 *	in metres per second.
 *
 *	No particles are generated if the particle system moved at less than
 *	the sensitivity. If the particle system moved at N times sensitivity,
 *	then (N-1)*2 particles would be generated.
 *
 *	@return	The number of metres moved before a particle is generated.
 */
INLINE float PySourcePSA::sensitivity( void ) const
{
	return pAction_->sensitivity();
}


/**
 *	This is the python Set-Accessor for the sensitivity property.
 *
 *	Note: Sensitivity cannot be set to zero - it means infinite particles
 *	would be generated on any finite displacement. If sensitivity is set
 *	below a threshold of 0.0001, it will be forced to that minimum threshold.
 *
 *	@param value	The number of metres moved before a particle is generated.
 */
INLINE void PySourcePSA::sensitivity( float value )
{
	pAction_->sensitivity( value );
}


/**
 *	This is the python Get-Accessor for the activePeriod property. This is the length
 *	in seconds where the source will generate particles at the rate given when
 *	its timeTriggered flag is true.
 *
 *	@return Period in seconds of activity when timeTriggered is true.
 */
INLINE float PySourcePSA::activePeriod( void ) const
{
	return pAction_->activePeriod();
}


/**
 *	This is the python Set-Accessor for the activePeriod property.
 *
 *	@param timeInSeconds	The new period in seconds.
 */
INLINE void PySourcePSA::activePeriod( float timeInSeconds )
{
	pAction_->activePeriod( timeInSeconds );
}


/**
 *	This is the python Get-Accessor for the sleepPeriod property. This is the length
 *	in seconds where the source will rest when its timeTriggered flag is true.
 *
 *	@return Period in seconds of inactivity when timeTriggered is true.
 */
INLINE float PySourcePSA::sleepPeriod( void ) const
{
	return pAction_->sleepPeriod();
}


/**
 *	This is the python Set-Accessor for the sleepPeriod property.
 *
 *	@param timeInSeconds	The new period in seconds.
 */
INLINE void PySourcePSA::sleepPeriod( float timeInSeconds )
{
	pAction_->sleepPeriod( timeInSeconds );
}


/**
 *	This is the python Get-Accessor for the sleepPeriodMax property. This is the maximum 
 *	length in seconds where the source will rest when its timeTriggered flag is true.
 *
 *	@return maximum Period in seconds of inactivity when timeTriggered is true.
 */
INLINE float PySourcePSA::sleepPeriodMax( void ) const
{
	return pAction_->sleepPeriodMax();
}


/**
 *	This is the python Set-Accessor for the sleepPeriodMax property.
 *
 *	@param timeInSeconds	The new max period in seconds.
 */
INLINE void PySourcePSA::sleepPeriodMax( float timeInSeconds )
{
	pAction_->sleepPeriodMax( timeInSeconds );
}


/**
 *	This is the python Get-Accessor for the minimumSize property. The particles
 *	generated by this source range in size from minimumSize to maximumSize,
 *	with 1.0 being original texture size.
 *
 *	@return	The minimum size value.
 */
INLINE float PySourcePSA::minimumSize( void ) const
{
	return pAction_->minimumSize();
}


/**
 *	This is the Set-Accessor for the minimumSize property.
 *
 *	@param newScale		The new value for the minimum size.
 */
INLINE void PySourcePSA::minimumSize( float newScale )
{
	pAction_->minimumSize( newScale );
}


/**
 *	This is the python Get-Accessor for the maximumSize property. The particles
 *	generated by this source range in size from minimumSize to maximumSize,
 *	with 1.0 being original texture size.
 *
 *	@return	The maximum size value.
 */
INLINE float PySourcePSA::maximumSize( void ) const
{
	return pAction_->maximumSize();
}


/**
 *	This is the python Set-Accessor for the maximumSize property.
 *
 *	@param newScale		The new value for the maximum size.
 */
INLINE void PySourcePSA::maximumSize( float newScale )
{
	pAction_->maximumSize( newScale );
}

/**
 *	This is the python Get-Accessor for the forcedUnitSize property. This number is
 *	the number of particles created for each unit of the forced total. The
 *	default value is set to 1.
 *
 *	@return	The number of particles created per forceTotal unit.
 */
INLINE int PySourcePSA::forcedUnitSize( void ) const
{
	return pAction_->forcedUnitSize();
}


/**
 *	This is the python Set-Accessor for the forcedUnitSize property.
 *
 *	@param newUnitSize	How many particles force(1) will create.
 */
INLINE void PySourcePSA::forcedUnitSize( int newUnitSize )
{
	pAction_->forcedUnitSize( newUnitSize );
}


/**
 *	This is the python Get-Accessor for the allowedTime property. For grounded
 *	sources, the collision detection can take a long time. The allowedTime
 *	sets a maximum allowed time in seconds for the source to create its
 *	particles. The values supplied through the interface are in seconds,
 *	although the actual allowedTime is stored as CPU clock cycles (or
 *	timestamps.)
 *
 *	@return The maximum time allowed in a frame for the SourcePSA to create
 *			particles. Units in seconds.
 */
INLINE float PySourcePSA::allowedTime( void ) const
{
	return pAction_->allowedTimeInSeconds();
}


/**
 *	This is the python Set-Accessor for the allowedTime property.
 *
 *	@param timeInSeconds	This is the new allowedTime value.
 */
INLINE void PySourcePSA::allowedTime( float timeInSeconds )
{
	pAction_->allowedTimeInSeconds( timeInSeconds );
}


/**
 *	This is the python Get-Accessor for the initialRotation_ property. 
 *
 *	@return	The initial rotation.
 */
INLINE Vector2 PySourcePSA::initialRotation( void ) const
{
	return pAction_->initialRotation();
}


/**
 *	This is the python Set-Accessor for the initialRotation_ property.
 *
 *	@param rotation	This is the new initialRotation value.
 */
INLINE void PySourcePSA::initialRotation( const Vector2& rotation)
{
	pAction_->initialRotation( rotation );
}


/**
 *	This is the python Get-Accessor for the randomInitialRotation property. 
 *
 *	@return	The initial rotation randomisation amount.
 */
INLINE Vector2 PySourcePSA::randomInitialRotation( void ) const
{
	return pAction_->randomInitialRotation();
}


/**
 *	This is the python Set-Accessor for the randomInitialRotation property.
 *
 *	@param rotation	This is the new initial rotation randomisation amount.
 */
INLINE void PySourcePSA::randomInitialRotation( const Vector2& rotation)
{
	pAction_->randomInitialRotation( rotation );
}


/**
 *	This is the python Get-Accessor for the initialColour_ property. 
 *
 *	@return	The initial colour.
 */
INLINE Vector4 PySourcePSA::initialColour( void ) const
{
	return pAction_->initialColour();
}


/**
 *	This is the python Set-Accessor for the initialColour_ property.
 *
 *	@param colour	This is the new initialColour_ value.
 */
INLINE void PySourcePSA::initialColour( Vector4 colour)
{
	pAction_->initialColour( colour );
}


/**
 *	This is the python Get-Accessor for the randomSpin_ property. 
 *
 *	@return	The whether mesh particles should have a random spin.
 */
INLINE bool PySourcePSA::randomSpin( void ) const	
{ 
	return pAction_->randomSpin(); 
}


/**
 *	This is the python Set-Accessor for the randomSpin_ property.
 *
 *	@param enable	This is the new randomSpin_ value.
 */
INLINE void PySourcePSA::randomSpin(bool enable)	
{ 
	pAction_->randomSpin( enable ); 
}


/**
 *	This is the python Get-Accessor for the minSpin_ property. 
 *
 *	@return	The minimum spin speed.
 */
INLINE float PySourcePSA::minSpin( void ) const	
{ 
	return pAction_->minSpin(); 
}


/**
 *	This is the python Set-Accessor for the minSpin_ property.
 *
 *	@param amount	This is the new minSpin_ value.
 */
INLINE void PySourcePSA::minSpin(float amount)	
{ 
	pAction_->minSpin( amount ); 
}


/**
 *	This is the python Get-Accessor for the maxSpin_ property. 
 *
 *	@return	The maximum spin speed.
 */
INLINE float PySourcePSA::maxSpin( void ) const	
{ 
	return pAction_->maxSpin(); 
}


/**
 *	This is the python Set-Accessor for the maxSpin_ property.
 *
 *	@param amount	This is the new maxSpin_ value.
 */
INLINE void PySourcePSA::maxSpin(float amount)	
{ 
	pAction_->maxSpin( amount ); 
}
	
	
/**
 *	This is the python Get-Accessor for the ignoreRotation_ property. 
 *
 *	@return	The ignoreRotation flag
 */
INLINE bool PySourcePSA::ignoreRotation( void ) const
{
	return pAction_->ignoreRotation();
}


/**
 *	This is the python Set-Accessor for the ignoreRotation_ property.
 *
 *	@param option	This is the new ignoreRotation_ flag value.
 */
INLINE void PySourcePSA::ignoreRotation( bool option)
{
	pAction_->ignoreRotation( option );
}


/**
 *	This is the python Get-Accessor for the inheritVelocity_ property. 
 *
 *	@return	The inheritVelocity_ value.
 */
INLINE float PySourcePSA::inheritVelocity() const 
{ 
	return pAction_->inheritVelocity(); 
}


/**
 *	This is the python Set-Accessor for the inheritVelocity_ property.
 *
 *	@param amount	This is the new inheritVelocity_ value.
 */
INLINE void PySourcePSA::inheritVelocity(float amount) 
{ 
	pAction_->inheritVelocity( amount ); 
}


/**
 *	This is the python Get-Accessor for the maxSpeed_ property. 
 *
 *	@return	The maxSpeed_ value.
 */
INLINE float PySourcePSA::maxSpeed() const 
{ 
	return pAction_->maxSpeed(); 
}


/**
 *	This is the python Set-Accessor for the maxSpeed_ property.
 *
 *	@param amount	This is the new maxSpeed_ value.
 */
INLINE void PySourcePSA::maxSpeed(float amount) 
{ 
	pAction_->maxSpeed( amount ); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PySourcePSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* source_psa.ipp */
