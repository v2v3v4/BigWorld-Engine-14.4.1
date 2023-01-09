#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for ScalerPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for ScalerPSA.
 *
 *	@param newSize	The new size to which to scale.
 *	@param newRate	The new rate for the scaling.
 */
INLINE ScalerPSA::ScalerPSA( float newSize, float newRate ) :
	size_( newSize ),
	rate_( newRate )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to ScalerPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the size attribute of ScalerPSA. This is the
 *	absolute scale that each of the particles will eventually become.
 *
 *	@return	The size target of the ScalerPSA.
 */
INLINE float ScalerPSA::size( void ) const
{
	return size_;
}


/**
 *	This is the Set-Accessor for the size attribute of the ScalerPSA.
 *
 *	@param newSize	The new size target for the ScalerPSA.
 */
INLINE void ScalerPSA::size( float newSize )
{
	size_ = newSize;
}


/**
 *	This is the Get-Accessor for the rate attribute of the ScalerPSA. This is
 *	the increment per second towards the target size.
 *
 *	@return	Rate as an increment size per second.
 */
INLINE float ScalerPSA::rate( void ) const
{
	return rate_;
}


/**
 *	This is the Set-Accessor for the rate attribute of the ScalerPSA. It is
 *	always stored as a positive value.
 *
 *	@param newRate	The new rate for the ScalerPSA.
 */
INLINE void ScalerPSA::rate( float newRate )
{
	rate_ = fabsf( newRate );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int ScalerPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & ScalerPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyScalerPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyScalerPSA.
 */
INLINE PyScalerPSA::PyScalerPSA( ScalerPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyScalerPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the size attribute of ScalerPSA. This is the
 *	absolute scale that each of the particles will eventually become.
 *
 *	@return	The size target of the ScalerPSA.
 */
INLINE float PyScalerPSA::size( void ) const
{
	return pAction_->size();
}


/**
 *	This is the python Set-Accessor for the size attribute of the ScalerPSA.
 *
 *	@param newSize	The new size target for the ScalerPSA.
 */
INLINE void PyScalerPSA::size( float newSize )
{
	pAction_->size( newSize );
}


/**
 *	This is the python Get-Accessor for the rate attribute of the ScalerPSA. This is
 *	the increment per second towards the target size.
 *
 *	@return	Rate as an increment size per second.
 */
INLINE float PyScalerPSA::rate( void ) const
{
	return pAction_->rate();
}


/**
 *	This is the python Set-Accessor for the rate attribute of the ScalerPSA. It is
 *	always stored as a positive value.
 *
 *	@param newRate	The new rate for the ScalerPSA.
 */
INLINE void PyScalerPSA::rate( float newRate )
{
	pAction_->rate( newRate );
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyScalerPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* scaler_psa.ipp */
