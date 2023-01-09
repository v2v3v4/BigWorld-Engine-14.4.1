// lens_effect.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method returns the colour of the lens effect
 */
INLINE uint32 FlareData::colour() const
{
	return colour_;
}


/**
 * @see colour
 */
INLINE void	FlareData::colour( uint32 c )
{
	colour_ = c;
}


/**
 * This method returns the size, in clip space, of the lens effect
 * Note that a lens effect doesn't really have a size,
 * just a width and height.  Size() returns the larger of the two.	
 */
INLINE float FlareData::size() const
{
	return (this->width() > this->height()) ? this->width() : this->height();
}


/**
 * @see size
 */
INLINE void	FlareData::size( float s )
{
	this->width( s );
	this->height( s );
}


/**
 * This method returns the material of the lens effect
 */
INLINE const BW::string & FlareData::material() const
{
	return material_;
}


/**
 *	@see material
 */
INLINE void FlareData::material( const BW::string & m )
{
	material_ = m;
}

/**
 *	This method sets the clip depth of the flare.
 *	The clip depth is the amount the clip position
 *	of the flare is scaled by; useful for multi-flare
 *	effects, like sun through a telephoto lens
 */
INLINE float FlareData::clipDepth() const
{
	return clipDepth_;
}


/**
 *	@see clipDepth
 */
INLINE void	FlareData::clipDepth( float d )
{
	clipDepth_ = d;
}


/**
 *	This method gets the width of the flare.
 *	@see size
 */
INLINE float FlareData::width() const
{
	return width_;
}


/**
 *	@see width
 */
INLINE void FlareData::width( float w )
{
	width_ = w;
}


/**
 *	This method gets the height of the flare.
 *	@see size
 */
INLINE float FlareData::height() const
{
	return height_;
}


/**
 *	@see height
 */
INLINE void FlareData::height( float h )
{
	height_ = h;
}

/**
 *	This method gets the z cull bias of the flare.
 */
INLINE float FlareData::zCullBias() const
{
	return zCullBias_;
}


/**
 *	@see zCullBias
 */
INLINE void FlareData::zCullBias( float zcb )
{
	zCullBias_ = zcb;
}

/**
 *	This method returns the age of the flare
 */
INLINE float FlareData::age() const
{
	return age_;
}


/**
 * @see age
 */
INLINE void FlareData::age( float a )
{
	age_ = a;
}


/**
 *	This method gets the secondary flares for this flare.
 */
INLINE const FlareData::Flares & FlareData::secondaries() const
{
	return secondaries_;
}


/**
 * This method returns the id of the lens effect
 */
INLINE size_t LensEffect::id() const
{
	return id_;
}


/**
 * @see id
 */
INLINE void	LensEffect::id( size_t value )
{
	id_ = value;
}


/**
 *	This method returns the world position of the lens effect
 */
INLINE const Vector3 & LensEffect::position() const
{
	return position_;
}


/**
 * @see position
 */
INLINE void	LensEffect::position( const Vector3 & pos )
{
	position_ = pos;
}


/**
 *	This method gets the counter when this flare was last added.
 */
INLINE uint32 LensEffect::added() const
{
	return added_;
}


/**
 *	@see added
 */
INLINE void LensEffect::added( uint32 when )
{
	added_ = when;
}


/**
 *	This method returns the maximum distance this flare
 *	will be visible at.
 */
INLINE float LensEffect::maxDistance() const
{
	return maxDistance_;
}


INLINE void LensEffect::maxDistance( float md )
{
	maxDistance_ = md;
}

/**
 *	This method returns whether this lens effect should be clamped to the 
 *	farplane.
 */
INLINE bool LensEffect::clampToFarPlane() const
{
	return clampToFarPlane_;
}


/**
 *	This method sets whether this lens effect should be clamped to the 
 *	farplane.
 */
INLINE void LensEffect::clampToFarPlane( bool s ) 
{
	clampToFarPlane_ = s;
}


/**
 *	This method returns the area of the flare geometry for testing occlusion.
 */
INLINE float LensEffect::area() const
{
	return area_;
}


/**
 *	This method sets the area of the flare geometry for testing occlusion.
 */
INLINE void LensEffect::area( float a ) 
{
	area_ = a;
}


/**
 *	This method returns the fading speed multiplier which determins how slow 
 *	the lens effect should fade in/out its flares.
 */
INLINE float LensEffect::fadeSpeed() const
{
	return fadeSpeed_;
}


/**
 *	This method sets the fading speed multiplier which determins how slow 
 *	the lens effect should fade in/out its flares.
 */
INLINE void LensEffect::fadeSpeed( float f ) 
{
	fadeSpeed_ = f;
}


/**
 *	This method gets the colour of the Lens Effect.
 */
INLINE uint32 LensEffect::colour() const
{
	return colour_;
}


/**
 *	This method sets the colour for this LensEffect.
 */
INLINE void LensEffect::colour( uint32 c )
{
	colour_ = c;
}


/**
 *	This method sets the colour for this LensEffect to default colour.
 */
INLINE void LensEffect::defaultColour()
{
	colour_ = DEFAULT_COLOUR;
}


/**
 * This method returns the occlusion level data for this Lens Effect.
 */
INLINE const LensEffect::OcclusionLevels & LensEffect::occlusionLevels() const
{
	return occlusionLevels_;
}

// lens_effect.ipp
