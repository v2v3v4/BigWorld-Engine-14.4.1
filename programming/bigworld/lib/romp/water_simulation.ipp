/*water_simulation.ipp*/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE void Simulation::size(int size)
{
	size_ = size;
}

INLINE int Simulation::size() const
{
	return size_;
}


INLINE void Simulation::addMovement( const Vector3& from, const Vector3& to, const float diameter )
{
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
	{
		movements_.push_back( Movement( MovementPair(from, to), diameter ));
	}
}


INLINE void Simulation::updateMovements()
{
	if (!Moo::rc().frameDrawn( lastMovementFrame_ ))
		movements_.clear();
}


INLINE const Movements& Simulation::movements() const
{ 
	return movements_;
}


INLINE void Simulation::resetIdleTimer()
{ 
	idleTime_ = 0.f;
}


INLINE float Simulation::idleTime() const
{
	return idleTime_;
}


INLINE bool Simulation::hasMovements() const
{
	return ( movements_.size() > 0 );
}


INLINE void Simulation::activate()
{
	bActive_ = true;
}


INLINE void Simulation::deactivate() 
{ 
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
		bActive_ = false;
}


INLINE bool Simulation::isActive() const
{ 
	return bActive_;
}


INLINE bool Simulation::shouldActivate() const
{
#ifdef _DEBUG
	if (SimulationManager::s_paused_)
		return false;
#endif

	return !bActive_ && perturb_;
}


INLINE bool Simulation::shouldDeactivate() const
{
#ifdef _DEBUG
	if (SimulationManager::s_paused_)
		return false;
#endif
	if( bActive_ && perturb_==false)
		return (idleTime_ > SimulationManager::instance().maxIdleTime());
	else
		return false;
}


INLINE bool Simulation::perturbed() const
{
	return perturb_;
}


INLINE void Simulation::perturbed( bool val )
{
	perturb_ = val;
}


INLINE void Simulation::updateTime( float dTime )
{
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
	{
		idleTime_ += dTime;
		lastSimTime_ += dTime;
	}
}


INLINE float Simulation::lastUpdateTime() const
{
	return lastSimTime_;
}


INLINE void Simulation::clearMovements()
{
	movements_.clear();
}


INLINE void SimulationCell::tick()
{ 
	if ( simulationTarget_
#ifdef _DEBUG
		&& !SimulationManager::s_paused_ 
#endif
	)
	{
		simulationTarget_->tickSimulation();
	}
}


INLINE void SimulationCell::mark( uint32 mark )
{
	tickMark_ = mark;
}


INLINE uint32 SimulationCell::mark( ) const
{
	return tickMark_;
}


INLINE SimulationTextureBlock* SimulationCell::simulationBlock() const
{
	return simulationTarget_;
}


INLINE void SimulationCell::simulationBlock( SimulationTextureBlock* val)
{
	simulationTarget_ = val;
}


INLINE bool SimulationCell::isActive() const
{ 
	return ( simulationTarget_ != 0) && Simulation::isActive();
}


INLINE void SimulationCell::activate()
{
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
	{
		simulationTarget_ = SimulationManager::instance().requestSimulationTextureBlock( this );
		if (simulationTarget_)
		{
			// Only make this active if we have acquired a valid block.
			Simulation::activate();
		}
	}
}


INLINE void SimulationCell::deactivate() 
{ 
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
	{
		Simulation::deactivate();
		if ( simulationTarget_ )
		{
			SimulationManager::instance().releaseSimulationTextureBlock( simulationTarget_ );
			simulationTarget_ = NULL;
		}
		edge(0);
	}
}

//////////////////////////////
// Edge index definitions
//////////////////////////////
//
//			  2
//		 -----------
//		| 	  	  	|
//	  1	|			| 3
//		|			|
//		| 		  	|
//		 -----------
//			  0
//
// Edges:
//	0 = negative Y
//  1 = negative X
//	2 = positive Y
//	3 = positive X
//
//////////////////////////////

/**
 *	This function will return a bitfield of the edges active according
 *	to the threshold passed.
 */
INLINE int SimulationCell::getActiveEdge( float threshold )
{ 
	float upper=1.f-threshold;
	int edges=0;	
	for (uint i=0; i<movements().size(); i++)
	{
		const Movement& m = movements()[i];
		const Vector3& move = m.first.second;

		if ( move.x <= threshold )
			edges |= 2;

		if ( move.x >= upper )
			edges |= 8;

		if ( move.z <= threshold )
			edges |= 1;

		if ( move.z >= upper )
			edges |= 4;
	}
	return edges;
}


INLINE void SimulationTextureBlock::tickSimulation() 
{
#ifdef _DEBUG
	if (!SimulationManager::s_paused_)
#endif
		simulationIndex_++;
}


INLINE void RainSimulation::tick()
{ 
	simulationTarget_.tickSimulation();
}


INLINE void RainSimulation::clear()
{
	simulationTarget_.clear();
}


INLINE void RainSimulation::recreate()
{
	simulationTarget_.recreate();
}


INLINE void RainSimulation::init(int size)
{
	simulationTarget_.init(size,size);
}


INLINE void RainSimulation::fini()
{
	simulationTarget_.fini();
}

INLINE Moo::RenderTargetPtr RainSimulation::result()
{ 
	return simulationTarget_.result();
}


INLINE Moo::RenderTargetPtr SimulationManager::rainTexture()
{ 
	return rainSimulation_.result();
}


/*water_simulation.ipp*/