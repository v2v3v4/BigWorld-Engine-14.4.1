#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) for ForcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the three-float constructor for ForcePSA.
 *
 *	@param x		The x component of the vector.
 *	@param y		The y component of the vector.
 *	@param z		The z component of the vector.
 */
INLINE ForcePSA::ForcePSA( float x, float y, float z) :
	vector_( x, y, z )
{
}


/**
 *	This is the Vector3 constructor for ForcePSA.
 *
 *	@param newVector	The new vector for the force.
 */
INLINE ForcePSA::ForcePSA( const Vector3 &newVector) :
	vector_( newVector )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors for the ForcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the vector property of the ForcePSA. This
 *	property is describes the force as a 3D-vector with its units in metres
 *	per second per second.
 *
 *	@return	A 3D vector describing the force.
 */
INLINE const Vector3 &ForcePSA::vector( void ) const
{
	return vector_;
}


/**
 *	This is the Set-Accessor for the vector property of the ForcePSA.
 *
 *	@param newVector	The new force.
 */
INLINE void ForcePSA::vector( const Vector3 &newVector )
{
	vector_ = newVector;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int ForcePSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & ForcePSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyForcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyForcePSA.
 */
INLINE PyForcePSA::PyForcePSA( ForcePSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors for the PyForcePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the vector property of the ForcePSA. This
 *	property is describes the force as a 3D-vector with its units in metres
 *	per second per second.
 *
 *	@return	A 3D vector describing the force.
 */
INLINE const Vector3 &PyForcePSA::vector( void ) const
{
	return pAction_->vector();
}


/**
 *	This is the python Set-Accessor for the vector property of the ForcePSA.
 *
 *	@param newVector	The new force.
 */
INLINE void PyForcePSA::vector( const Vector3 &newVector )
{
	pAction_->vector( newVector );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyForcePSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* force_psa.ipp */
