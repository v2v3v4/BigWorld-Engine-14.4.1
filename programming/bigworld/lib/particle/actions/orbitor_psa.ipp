#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for OrbitorPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the three-float constructor for OrbitorPSA.
 *
 *	@param x			The x component of the point.
 *	@param y			The y component of the point.
 *	@param z			The z component of the point.
 *	@param newVelocity	The angular velocity in degrees.
 */
INLINE OrbitorPSA::OrbitorPSA( float x, float y, float z, float newVelocity ) :
	point_( x, y, z ),
	angularVelocity_( DEG_TO_RAD( newVelocity ) ),
	affectVelocity_( false )
{
}

/**
 *	This is the Vector3 constructor for OrbitorPSA.
 *
 *	@param newPoint		The centre of the orbitor.
 *	@param newVelocity	The angular velocity in degrees.
 */
INLINE OrbitorPSA::OrbitorPSA( const Vector3 &newPoint, float newVelocity ) :
	point_( newPoint ),
	angularVelocity_( DEG_TO_RAD( newVelocity ) ),
	affectVelocity_( false )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to OrbitorPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the centre of the OrbitorPSA.
 *
 *	@return	A point in world-space as the centre of orbiting activity.
 */
INLINE const Vector3 &OrbitorPSA::point( void ) const
{
	return point_;
}


/**
 *	This is the Set-Accessor for the centre of the OrbitorPSA.
 *
 *	@param newVector	The new centre for the orbitor.
 */
INLINE void OrbitorPSA::point( const Vector3 &newVector )
{
	point_ = newVector;
}


/**
 *	This is the Get-Accessor for the angular velocity of the OrbitorPSA. The
 *	value is stored in radians, though it is transferred to the user via
 *	degrees.
 *
 *	@return	A value in degrees per second.
 */
INLINE float OrbitorPSA::angularVelocity( void ) const
{
	return RAD_TO_DEG( angularVelocity_ );
}


/**
 *	This is the Set-Accessor for the angular velocity of the OrbitorPSA. The
 *	value is supplied in degrees per second, but stored as radians.
 *
 *	@param newVelocity	The new velocity in degrees per second.
 */
INLINE void OrbitorPSA::angularVelocity( float newVelocity )
{
	angularVelocity_ = DEG_TO_RAD( newVelocity );
}


/**
 *	This is the Get-Accessor for whether the OrbitorPSA affects velocity as
 *	well as position.
 *
 *	@return The current flag for affecting velocity.
 */
INLINE bool OrbitorPSA::affectVelocity( void ) const
{
	return affectVelocity_;
}


/**
 *	This is the Set-Accessor for whether the OrbitorPSA affects velocity as
 *	well as position.
 *
 *	@param flag		The new value for affecting velocity.
 */
INLINE void OrbitorPSA::affectVelocity( bool flag )
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
INLINE int OrbitorPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & OrbitorPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyOrbitorPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyOrbitorPSA.
 */
INLINE PyOrbitorPSA::PyOrbitorPSA( OrbitorPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyOrbitorPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the ptyhon Get-Accessor for the centre of the OrbitorPSA.
 *
 *	@return	A point in world-space as the centre of orbiting activity.
 */
INLINE const Vector3 &PyOrbitorPSA::point( void ) const
{
	return pAction_->point();
}


/**
 *	This is the python Set-Accessor for the centre of the OrbitorPSA.
 *
 *	@param newVector	The new centre for the orbitor.
 */
INLINE void PyOrbitorPSA::point( const Vector3 &newVector )
{
	pAction_->point( newVector );
}


/**
 *	This is the python Get-Accessor for the angular velocity of the OrbitorPSA. The
 *	value is stored in radians, though it is transferred to the user via
 *	degrees.
 *
 *	@return	A value in degrees per second.
 */
INLINE float PyOrbitorPSA::angularVelocity( void ) const
{
	return pAction_->angularVelocity();
}


/**
 *	This is the python Set-Accessor for the angular velocity of the OrbitorPSA. The
 *	value is supplied in degrees per second, but stored as radians.
 *
 *	@param newVelocity	The new velocity in degrees per second.
 */
INLINE void PyOrbitorPSA::angularVelocity( float newVelocity )
{
	pAction_->angularVelocity( newVelocity );
}


/**
 *	This is the python Get-Accessor for whether the OrbitorPSA affects velocity as
 *	well as position.
 *
 *	@return The current flag for affecting velocity.
 */
INLINE bool PyOrbitorPSA::affectVelocity( void ) const
{
	return pAction_->affectVelocity();
}


/**
 *	This is the python Set-Accessor for whether the OrbitorPSA affects velocity as
 *	well as position.
 *
 *	@param flag		The new value for affecting velocity.
 */
INLINE void PyOrbitorPSA::affectVelocity( bool flag )
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
INLINE int PyOrbitorPSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* orbitor_psa.ipp */
