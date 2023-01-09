#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for SinkPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for SinkPSA.
 *
 *	@param maximumAge		The maximumAge allowed by the action.
 *	@param minimumSpeed		The slowest speed allowed by the action.
 */
INLINE SinkPSA::SinkPSA( float maximumAge, float minimumSpeed ) :
	maximumAge_( maximumAge ),
	minimumSpeed_( minimumSpeed ),
	outsideOnly_( false )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to SinkPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the SinkPSA's maximumAge property. Any
 *	particle found to be older than this will be removed from the particle
 *	system.
 *
 *	@return	The current maximum age allowed by the SinkPSA.
 */
INLINE float SinkPSA::maximumAge( void ) const
{
	return maximumAge_;
}


/**
 *	This is the Set-Accessor for the SinkPSA's maximumAge property. If the
 *	property is set to negative, then there is no maximum age.
 *
 *	@param value	The new maximum age to be allowed by the SinkPSA.
 */
INLINE void SinkPSA::maximumAge( float value )
{
	maximumAge_ = value;
}


/**
 *	This is the Get-Accessor for the SinkPSA's minimumSpeed property. Any
 *	particle found to be slower than this will be removed from the particle
 *	system.
 *
 *	@return	The current minimum speed allowed by the SinkPSA.
 */
INLINE float SinkPSA::minimumSpeed( void ) const
{
	return minimumSpeed_;
}


/**
 *	This is the Set-Accessor for the SinkPSA's outsideOnly property.
 *
 *	@param value	The new outside only flag value.
 */
INLINE void SinkPSA::outsideOnly( bool value )
{
	outsideOnly_ = value;
}

/**
 *	This is the Get-Accessor for the SinkPSA's outsideOnly property.
 *
 *	@param value	The new outside only flag value.
 */
INLINE bool SinkPSA::outsideOnly( void ) const
{
	return outsideOnly_;
}


/**
 *	This is the Set-Accessor for the SinkPSA's minimumSpeed property. If the
 *	property is set to negative, then there is no minimum speed.
 *
 *	@param value	The new minimum speed to be allowed by the SinkPSA.
 */
INLINE void SinkPSA::minimumSpeed( float value )
{
	minimumSpeed_ = value;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int SinkPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & SinkPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PySinkPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PySinkPSA.
 */
INLINE PySinkPSA::PySinkPSA( SinkPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PySinkPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the SinkPSA's maximumAge property. Any
 *	particle found to be older than this will be removed from the particle
 *	system.
 *
 *	@return	The current maximum age allowed by the SinkPSA.
 */
INLINE float PySinkPSA::maximumAge( void ) const
{
	return pAction_->maximumAge();
}


/**
 *	This is the python Set-Accessor for the SinkPSA's maximumAge property. If the
 *	property is set to negative, then there is no maximum age.
 *
 *	@param value	The new maximum age to be allowed by the SinkPSA.
 */
INLINE void PySinkPSA::maximumAge( float value )
{
	pAction_->maximumAge( value );
}


/**
 *	This is the python Get-Accessor for the SinkPSA's minimumSpeed property. Any
 *	particle found to be slower than this will be removed from the particle
 *	system.
 *
 *	@return	The current minimum speed allowed by the SinkPSA.
 */
INLINE float PySinkPSA::minimumSpeed( void ) const
{
	return pAction_->minimumSpeed();
}


/**
 *	This is the python Set-Accessor for the SinkPSA's minimumSpeed property. If the
 *	property is set to negative, then there is no minimum speed.
 *
 *	@param value	The new minimum speed to be allowed by the SinkPSA.
 */
INLINE void PySinkPSA::minimumSpeed( float value )
{
	pAction_->minimumSpeed( value );
}

/**
 *	This is the python Get-Accessor for the SinkPSA's outsideOnly property. 
 *
 *	@return	The current outside only flag for the SinkPSA.
 */
INLINE bool PySinkPSA::outsideOnly( void ) const
{
	return pAction_->outsideOnly();
}


/**
 *	This is the python Set-Accessor for the SinkPSA's SinkPSA's outsideOnly property. 
 *
 *	@param value	The new outside only flag for the SinkPSA.
 */
INLINE void PySinkPSA::outsideOnly( bool value )
{
	pAction_->outsideOnly( value );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PySinkPSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* sink_psa.ipp */
