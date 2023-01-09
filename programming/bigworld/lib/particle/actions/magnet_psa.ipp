#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for MagnetPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for MagnetPSA.
 *
 *	@param s			The strength of the magnet.
 */
INLINE MagnetPSA::MagnetPSA( float s ) :
	source_( NULL ),
	strength_( s ),
	minDist_( 1.f )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to MagnetPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Set-Accessor for the strength property of the MagnetPSA.
 *	The stronger the magnet for quicker it will affect particle velocities.
 *
 *	@param s		The new value of strength.
 */
INLINE void MagnetPSA::strength( float s ) 
{ 
	strength_ = s; 
}


/**
 *	This is the Get-Accessor for the strength property of the MagnetPSA.
 *	The stronger the magnet for quicker it will affect particle velocities.
 *
 *	@return			The current value of strength.
 */
INLINE float MagnetPSA::strength() const 
{ 
	return strength_; 
}


/**
 *	This is the Set-Accessor for the minDist property of the MagnetPSA.
 *	This is the minimum distance that the particle has to be from the source
 *	of the magnet before the action is acted upon it.
 *
 *	@param s		The new value of minDist.
 */
INLINE void MagnetPSA::minDist( float s ) 
{ 
	MF_ASSERT_DEV(s > 0.f); 
	minDist_ = s; 
}


/**
 *	This is the Get-Accessor for the minDist property of the MagnetPSA.
 *	This is the minimum distance that the particle has to be from the source
 *	of the magnet before the action is acted upon it.
 */
INLINE float MagnetPSA::minDist() const 
{ 
	return minDist_; 
}


/**
 *	This is the Set-Accessor for the source property of the MagnetPSA.
 *	This is the source position of the magnet. Particles will be attracted
 *	to this position.
 *
 *	@param s		A MatrixProviderPtr object to the new source.
 */
INLINE void MagnetPSA::source( MatrixProviderPtr s ) 
{ 
	source_ = s; 
}


/**
 *	This is the Get-Accessor for the source property of the MagnetPSA.
 *	This is the source position of the magnet. Particles will be attracted
 *	to this position.
 *
 *	@return			The current value of source as a MatrixProviderPtr.
 */
INLINE MatrixProviderPtr MagnetPSA::source() const 
{ 
	return source_; 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int MagnetPSA::typeID() const
{ 
	return typeID_; 
}


INLINE const BW::string & MagnetPSA::nameID()  const
{ 
	return nameID_; 
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyMagnetPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyMagnetPSA.
 */
INLINE PyMagnetPSA::PyMagnetPSA( MagnetPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyMagnetPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Set-Accessor for the strength property of the MagnetPSA.
 *	The stronger the magnet for quicker it will affect particle velocities.
 *
 *	@param s		The new value of strength.
 */
INLINE void PyMagnetPSA::strength( float s ) 
{ 
	pAction_->strength( s ); 
}


/**
 *	This is the python Get-Accessor for the strength property of the MagnetPSA.
 *	The stronger the magnet for quicker it will affect particle velocities.
 *
 *	@return			The current value of strength.
 */
INLINE float PyMagnetPSA::strength() const 
{ 
	return pAction_->strength(); 
}


/**
 *	This is the python Set-Accessor for the minDist property of the MagnetPSA.
 *	This is the minimum distance that the particle has to be from the source
 *	of the magnet before the action is acted upon it.
 *
 *	@param s		The new value of minDist.
 */
INLINE void PyMagnetPSA::minDist( float s ) 
{ 
	if ( s < 0 )
	{
		ERROR_MSG( "PyMagnetPSA::minDist: Trying to set a minimum distance less than 0\n" );
		return;
	}
	pAction_->minDist( s ); 
}


/**
 *	This is the python Get-Accessor for the minDist property of the MagnetPSA.
 *	This is the minimum distance that the particle has to be from the source
 *	of the magnet before the action is acted upon it.
 *
 *	@param			The current value of minDist.
 */
INLINE float PyMagnetPSA::minDist() const 
{ 
	return pAction_->minDist(); 
}


/**
 *	This is the python Set-Accessor for the source property of the MagnetPSA.
 *	This is the source position of the magnet. Particles will be attracted
 *	to this position.
 *
 *	@param s		A MatrixProviderPtr object to the new source.
 */
INLINE void PyMagnetPSA::source( MatrixProviderPtr s ) 
{ 
	pAction_->source( s ); 
}


/**
 *	This is the python Get-Accessor for the source property of the MagnetPSA.
 *	This is the source position of the magnet. Particles will be attracted
 *	to this position.
 *
 *	@return			The current value of source as a MatrixProviderPtr.
 */
INLINE MatrixProviderPtr PyMagnetPSA::source() const 
{ 
	return pAction_->source(); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyMagnetPSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* magnet_psa.ipp */
