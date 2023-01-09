#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for StreamPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the three-float constructor for StreamPSA.
 *
 *	@param x			The x component of the vector.
 *	@param y			The y component of the vector.
 *	@param z			The z component of the vector.
 *	@param newHalfLife	The half-life component of the stream.
 */
INLINE StreamPSA::StreamPSA( float x, float y, float z, float newHalfLife ) :
	vector_( x, y, z ),
	halfLife_( newHalfLife )
{
}


/**
 *	This is the Vector3 constructor for StreamPSA.
 *
 *	@param newVector	The new vector for the force.
 *	@param newHalfLife	The half-life component of the stream.
 */
INLINE StreamPSA::StreamPSA( const Vector3 &newVector, float newHalfLife ) :
	vector_( newVector ),
	halfLife_( newHalfLife )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to StreamPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the vector property of the StreamPSA. This
 *	property is describes the stream as a 3D-vector with its units in metres
 *	per second.
 *
 *	@return	A 3D vector describing the force.
 */
INLINE const Vector3 &StreamPSA::vector( void ) const
{
	return vector_;
}


/**
 *	This is the Set-Accessor for the vector property of the StreamPSA.
 *
 *	@param newVector	The new force.
 */
INLINE void StreamPSA::vector( const Vector3 &newVector )
{
	vector_ = newVector;
}


/**
 *	This is the Get-Accessor for the half-life property of the StreamPSA. This
 *	property is the time in seconds for the current velocity of the particle
 *	to reach half-way towards the streamPSA's velocity.
 *
 *	@return	A value in seconds.
 */
INLINE float StreamPSA::halfLife( void ) const
{
	return halfLife_;
}


/**
 *	This is the Set-Accessor for the half-life property of the StreamPSA.
 *
 *	@param newHalfLife	The new half-life in seconds.
 */
INLINE void StreamPSA::halfLife( float newHalfLife )
{
	halfLife_ = newHalfLife;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int StreamPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & StreamPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyStreamPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyStreamPSA.
 */
INLINE PyStreamPSA::PyStreamPSA( StreamPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyStreamPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the vector property of the StreamPSA. This
 *	property is describes the stream as a 3D-vector with its units in metres
 *	per second.
 *
 *	@return	A 3D vector describing the force.
 */
INLINE const Vector3 &PyStreamPSA::vector( void ) const
{
	return pAction_->vector();
}


/**
 *	This is the python Set-Accessor for the vector property of the StreamPSA.
 *
 *	@param newVector	The new force.
 */
INLINE void PyStreamPSA::vector( const Vector3 &newVector )
{
	pAction_->vector( newVector );
}


/**
 *	This is the python Get-Accessor for the half-life property of the StreamPSA. This
 *	property is the time in seconds for the current velocity of the particle
 *	to reach half-way towards the streamPSA's velocity.
 *
 *	@return	A value in seconds.
 */
INLINE float PyStreamPSA::halfLife( void ) const
{
	return pAction_->halfLife();
}


/**
 *	This is the python Set-Accessor for the half-life property of the StreamPSA.
 *
 *	@param newHalfLife	The new half-life in seconds.
 */
INLINE void PyStreamPSA::halfLife( float newHalfLife )
{
	pAction_->halfLife( newHalfLife );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyStreamPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* stream_psa.ipp */
