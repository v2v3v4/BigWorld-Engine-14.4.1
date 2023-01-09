#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for SplatPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for SplatPSA.
 */
INLINE SplatPSA::SplatPSA() :
	callback_( NULL )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to SplatPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the SplatPSA's callback property. The callback
 *	is used to execute some scripted event when the particle collides with 
 *	the scenery.
 *
 *	@return				The callback script.
 */
INLINE SmartPointer<PyObject> SplatPSA::callback() const	
{ 
	return callback_; 
}


/**
 *	This is the Set-Accessor for the SplatPSA's callback property. The callback
 *	is used to execute some scripted event when the particle collides with 
 *	the scenery.
 *
 *	@param c			 The new callback script.
 */
INLINE void SplatPSA::callback( SmartPointer<PyObject> c )	
{ 
	callback_ = c; 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int SplatPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & SplatPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PySplatPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PySinkPSA.
 */
INLINE PySplatPSA::PySplatPSA( SplatPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PySplatPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the SplatPSA's callback property. The callback
 *	is used to execute some scripted event when the particle collides with 
 *	the scenery.
 *
 *	@return				The callback script.
 */
INLINE SmartPointer<PyObject> PySplatPSA::callback() const	
{ 
	return pAction_->callback(); 
}


/**
 *	This is the python Set-Accessor for the SplatPSA's callback property. The callback
 *	is used to execute some scripted event when the particle collides with 
 *	the scenery.
 *
 *	@param c			 The new callback script.
 */
INLINE void PySplatPSA::callback( SmartPointer<PyObject> c )	
{ 
	pAction_->callback( c ); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PySplatPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* splat_psa.ipp */
