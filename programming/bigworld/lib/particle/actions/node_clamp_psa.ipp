#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for NodeClampPSA.
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for NodeClampPSA.
 */
INLINE NodeClampPSA::NodeClampPSA() :
	lastPositionOfPS_( 0.0f, 0.0f, 0.0f ),
	firstUpdate_( true ),
	fullyClamp_( true )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to NodeClampPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the fullClamp property of the NodeClampPSA.
 *	This determines if all particles should be clamped to the relevant node.
 *
 *	@return			The current value of fullClamp.
 */
INLINE bool NodeClampPSA::fullyClamp() const	
{ 
	return fullyClamp_; 
}


/**
 *	This is the Set-Accessor for the fullClamp property of the NodeClampPSA.
 *	This determines if all particles should be clamped to the relevant node.
 *
 *	@param f		The new value of fullClamp.
 */
INLINE void NodeClampPSA::fullyClamp( bool f ) 
{ 
	fullyClamp_ = f; 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int NodeClampPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & NodeClampPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyNodeClampPSA.
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for PyNodeClampPSA.
 */
INLINE PyNodeClampPSA::PyNodeClampPSA( NodeClampPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyNodeClampPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the fullClamp property of the NodeClampPSA.
 *	This determines if all particles should be clamped to the relevant node.
 *
 *	@return			The current value of fullClamp.
 */
INLINE bool PyNodeClampPSA::fullyClamp() const	
{ 
	return pAction_->fullyClamp(); 
}


/**
 *	This is the python Set-Accessor for the fullClamp property of the NodeClampPSA.
 *	This determines if all particles should be clamped to the relevant node.
 *
 *	@param f		The new value of fullClamp.
 */
INLINE void PyNodeClampPSA::fullyClamp( bool f ) 
{ 
	pAction_->fullyClamp( f ); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyNodeClampPSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* node_clamp_psa.ipp */
