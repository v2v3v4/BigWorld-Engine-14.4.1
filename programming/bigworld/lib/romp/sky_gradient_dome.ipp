
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE bool
SkyGradientDome::fullOpacity( void ) const
{
	return fullOpacity_;
}


INLINE void
SkyGradientDome::fogModulation( const Vector3 & modulateColour, float fogMultiplier )
{
	modulateColour_ = modulateColour;
	fogMultiplier_ = fogMultiplier;
}


INLINE void
SkyGradientDome::nearMultiplier( float n )
{
	fogEmitter_.nearMultiplier_ = n;
	FogController::instance().update( fogEmitter_ );
}


INLINE float
SkyGradientDome::nearMultiplier() const
{
	return fogEmitter_.nearMultiplier_;
}


INLINE void
SkyGradientDome::farMultiplier( float n )
{
	fogEmitter_.maxMultiplier_ = n;
	FogController::instance().update( fogEmitter_ );
}


INLINE float
SkyGradientDome::farMultiplier() const
{
	return fogEmitter_.maxMultiplier_;
}


/*skygradientdome.ipp*/
