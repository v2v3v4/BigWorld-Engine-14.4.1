#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Constructor(s) for BarrierPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for BarrierPSA.
 */
INLINE BarrierPSA::BarrierPSA() :
	shape_( BarrierPSA::NONE ),
	reaction_( BarrierPSA::ALLOW ),
	vecA_( 0.0f, 0.0f, 0.0f ),
	vecB_( 0.0f, 0.0f, 0.0f ),
	radius_( 0.0f ),
	resilience_( 1.0f)
{
}


// -----------------------------------------------------------------------------
// Section: Methods for the BarrierPSA.
// -----------------------------------------------------------------------------


INLINE void BarrierPSA::none()
{
	shape_ = NONE;
	vecA_ = Vector3::zero();
	radius_ = 0.f;
}


/**
 *	This method changes the barrier shape to the vertical cylinder specified.
 *	The cylinder is vertical and is infinitely tall and deep. It is described
 *	by a point on its axis, and a radius. The point is in world space.
 *
 *	@param pointOnAxis	A 3D vector for a point on the cylinder's axis.
 *	@param radius		The radius of the cylinder in metres.
 */
INLINE void BarrierPSA::verticalCylinder( const Vector3 &pointOnAxis,
		float radius )
{
	shape_ = VERTICAL_CYLINDER;
	vecA_ = pointOnAxis;
	radius_ = radius;
}


/**
 *	This method changes the barrier shape to the box specified. A box is always
 *	completely parallel to the axis vectors (X,Y,Z), hence it can be described
 *	by its opposite corners. The coordinates given are in world space.
 *
 *	@param corner			A 3D vector for one corner of the box.
 *	@param oppositeCorner	A 3D vector for the opposite corner.
 */
INLINE void BarrierPSA::box( const Vector3 &corner,
		const Vector3 &oppositeCorner )
{
	shape_ = BOX;
	vecA_ = corner;
	vecB_ = oppositeCorner;
}


/**
 *	This method changes the barrier shape to the sphere specified. The sphere
 *	is described by a centre and a radius. Coordinates supplied are in world
 *	space; the units used are in metres.
 *
 *	@param centre	A 3D vector for the sphere's centre.
 *	@param radius	The radius of the sphere in metres.
 */
INLINE void BarrierPSA::sphere( const Vector3 &centre, float radius )
{
	shape_ = SPHERE;
	vecA_ = centre;
	radius_ = radius;
}


/**
 *	This method switches the reaction of the barrier to bounce the particles
 *	off it.
 */
INLINE void BarrierPSA::bounceParticles( void )
{
	reaction_ = BOUNCE;
}


/**
 *	This method switches the reaction of the barrier to removing any particles
 *	that collide with it.
 */
INLINE void BarrierPSA::removeParticles( void )
{
	reaction_ = REMOVE;
}


/**
 *	This method switches the reaction of the barrier to allow particles to
 *	collide with it. This effectively turns it off.
 */
INLINE void BarrierPSA::allowParticles( void )
{
	reaction_ = ALLOW;
}

/**
 *	This method switches the reaction of the barrier to make particles
 *	wrap when they go outside the barrier.
 */
INLINE void BarrierPSA::wrapParticles( void )
{
	reaction_ = WRAP;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int BarrierPSA::typeID( void ) const
{
	return typeID_;
}


INLINE const BW::string & BarrierPSA::nameID() const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyBarrierPSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for BarrierPSA.
 *
 *	@param pType			Parameters passed to the parent class.
 */
INLINE PyBarrierPSA::PyBarrierPSA( BarrierPSAPtr pAction, PyTypeObject *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyBarrierPSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* barrier_psa.ipp */
