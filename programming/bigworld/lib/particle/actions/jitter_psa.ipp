#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for JitterPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for JitterPSA.
 *
 *	@param pPositionSrc		Pointer to the position vector generator.
 *	@param pVelocitySrc		Pointer to the velocity vector generator.
 */
INLINE JitterPSA::JitterPSA( VectorGenerator *pPositionSrc,
		VectorGenerator *pVelocitySrc ) :
	affectPosition_( false ),
	affectVelocity_( false ),
	pPositionSrc_( pPositionSrc ),
	pVelocitySrc_( pVelocitySrc )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to JitterPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the affectPosition property of the JitterPSA.
 *	If this is set to true, then the positions of each particle will be
 *	jittered by the random amount from the position generator.
 *
 *	@return The current status of the affectPosition flag.
 */
INLINE bool JitterPSA::affectPosition( void ) const
{
	return affectPosition_;
}


/**
 *	This is the Set-Accessor for the affectPosition property of the JitterPSA.
 *
 *	@param flag		The new value of affectPosition.
 */
INLINE void JitterPSA::affectPosition( bool flag )
{
	affectPosition_ = flag;
}


/**
 *	This is the Get-Accessor for the affectVelocity property of the JitterPSA.
 *	If this is set to true, then the velocities of each particle will be
 *	jittered by the random amount from the position generator.
 *
 *	@return The current status of the affectVelocity flag.
 */
INLINE bool JitterPSA::affectVelocity( void ) const
{
	return affectVelocity_;
}


/**
 *	This is the Set-Accessor for the affectVelocity property of the JitterPSA.
 *
 *	@param flag		The new value of affectVelocity.
 */
INLINE void JitterPSA::affectVelocity( bool flag )
{
	affectVelocity_ = flag;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int JitterPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & JitterPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyJitterPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyJitterPSA.
 */
INLINE PyJitterPSA::PyJitterPSA( JitterPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyJitterPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the affectPosition property of the JitterPSA.
 *	If this is set to true, then the positions of each particle will be
 *	jittered by the random amount from the position generator.
 *
 *	@return The current status of the affectPosition flag.
 */
INLINE bool PyJitterPSA::affectPosition( void ) const
{
	return pAction_->affectPosition();
}


/**
 *	This is the python Set-Accessor for the affectPosition property of the JitterPSA.
 *
 *	@param flag		The new value of affectPosition.
 */
INLINE void PyJitterPSA::affectPosition( bool flag )
{
	pAction_->affectPosition( flag );
}


/**
 *	This is the python Get-Accessor for the affectVelocity property of the JitterPSA.
 *	If this is set to true, then the velocities of each particle will be
 *	jittered by the random amount from the position generator.
 *
 *	@return The current status of the affectVelocity flag.
 */
INLINE bool PyJitterPSA::affectVelocity( void ) const
{
	return pAction_->affectVelocity();
}


/**
 *	This is the python Set-Accessor for the affectVelocity property of the JitterPSA.
 *
 *	@param flag		The new value of affectVelocity.
 */
INLINE void PyJitterPSA::affectVelocity( bool flag )
{
	pAction_->affectVelocity( flag );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyJitterPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* jitter_psa.ipp */
