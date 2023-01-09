
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE void	FogController::enable( bool state )
{
	enabled_ = state;
}


INLINE bool FogController::enable( void ) const
{
	return enabled_;
}


INLINE uint32 FogController::colour( void ) const
{
	return colour_;
}


INLINE void FogController::colour( uint32 c )
{
	colour_ = c;
}


INLINE float FogController::multiplier( void ) const
{
	return multiplier_;
}


INLINE void FogController::multiplier( float m )
{
	multiplier_ = m;

	if ( multiplier_ < 1.f )
		multiplier_ = 1.f;
}


INLINE float FogController::nearMultiplier( void ) const
{
	return nearMultiplier_;
}


INLINE void FogController::nearMultiplier( float m )
{
	nearMultiplier_ = m;

	if ( nearMultiplier_ > 1.f )
		nearMultiplier_ = 1.f;
	if ( nearMultiplier_ < -1.f )
		nearMultiplier_ = -1.f;
}



INLINE int FogController::addEmitter( const Emitter & emitter )
{
	//add emitter
	if (emitter.id_ == -1)
		emitter.id_ = global_emitter_id_++;

	emitters_.push_back( emitter );

	return emitter.id_;
}


INLINE void	FogController::delEmitter( int emitterID )
{
	BW_GUARD;
	Emitters::iterator it = emitters_.begin();
	Emitters::iterator end = emitters_.end();

	while ( it != emitters_.end() )
	{
		if ( (*it).id_ == emitterID )
		{
			emitters_.erase( it );
			break;
		}

		it++;
	}
}


INLINE uint32 FogController::farObjectTFactor( void ) const
{
	return farObjectTFactor_;
}


INLINE uint32 FogController::additiveFarObjectTFactor( void ) const
{
	return additiveFarObjectTFactor_;
}

/*fog_controller.ipp*/
