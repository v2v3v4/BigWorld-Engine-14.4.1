/*water.ipp*/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE float Waters::movementImpact() const
{ 
	return movementImpact_;
}

/**
 *
 */
INLINE bool Waters::drawReflection() 
{ 
	return s_drawReflection_;
}


/**
 *
 */
INLINE void Waters::drawReflection( bool val )
{ 
	s_drawReflection_ = val;
}


/**
 *	Flag for water surface drawing
 */
INLINE void Waters::drawWaters( bool draw )
{
	s_drawWaters_ = draw;
}


INLINE void Waters::simulationEnabled( bool val)
{ 
	s_simulationEnabled_ = val;
}


INLINE bool Waters::simulationEnabled()
{
	return s_simulationEnabled_;
}


/**
 *	Flag that indicates the eye is inside a water volume.
 */
INLINE bool Waters::insideVolume() const
{ 
	return insideWaterVolume_;
}


/**
 *	Flag that indicates the eye is inside a water volume.
 */
INLINE void Waters::insideVolume( bool val )
{ 
	insideWaterVolume_ = val;
}


/**
 *	Flag for water surface drawing
 */
INLINE void Waters::drawWireframe( bool wire )
{
	drawWireframe_ = wire;
}


/**
 *	This method sets the amount of rain that affects all waters
 */
INLINE void Waters::rainAmount( float rainAmount )
{
	rainAmount_ = ::max( 0.f, ::min( 1.f, rainAmount ) );
}


/**
 *
 */
INLINE float Waters::rainAmount() const
{ 
	return rainAmount_;
}

//TODO: check against the bounding box and send off enter/exit events.
INLINE void Waters::playerPos( Vector3 pos )
{
	playerPos_ = pos;
}


#ifdef SPLASH_ENABLED
/**
 * Pass the new splash info to the manager
 */
INLINE void Waters::addSplash( const Vector4& impact,const float height, bool force )
{ 
	splashManager_.addSplash(impact,height,force);
}


/**
 * Check the timing of the automatic impact generation.
 */
INLINE bool Waters::checkImpactTimer() 
{
	if ( s_autoImpactInterval_ >= 0 && (impactTimer_ > s_autoImpactInterval_) )
	{
		impactTimer_ = 0.f;
		return true;
	}
	return false;
}
#endif //SPLASH_ENABLED


INLINE bool Water::shouldDraw() const
{
	return ( initialised_ && visible_ && Waters::s_drawWaters_ );
}


INLINE const Vector3& Water::position() const
{ 
	return state_.position_;
}


INLINE const Vector2& Water::size() const
{ 
	return state_.size_;
}

INLINE float Water::orientation() const
{ 
	return state_.orientation_;
}


INLINE Moo::RenderTargetPtr WaterCell::simTexture()
{
	return result();
} 


INLINE int WaterCell::indexCount() const
{
	return nIndices_;
}


INLINE bool WaterCell::simulationActive() const
{
	return isActive();
}


INLINE void WaterCell::initSimulation( int size, float cellSize )
{	
	SimulationCell::init( size, cellSize );
}


INLINE bool WaterCell::bind()
{ 
	if (nIndices_ > 2)	
		return indexBuffer_.set() == D3D_OK;
	else
		return false;
}


INLINE void WaterCell::draw( int nVerts )
{ 
	MF_ASSERT_DEBUG( nIndices_ > 2 );
	Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLESTRIP, 0, 0, nVerts, 0, nIndices_ - 2 );
}


INLINE void WaterCell::initEdge( int index, WaterCell* cell )
{
	edgeCells_[index] = cell;
}


INLINE const Vector2& WaterCell::min()
{
	return min_;
}


INLINE const Vector2& WaterCell::max()
{
	return max_;
}


INLINE const Vector2& WaterCell::size()
{
	return size_;
}

INLINE void WaterCell::deactivate()
{
	SimulationCell::deactivate();
	edge(0);
}


/*water.ipp*/