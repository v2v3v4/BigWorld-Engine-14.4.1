// material.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif


namespace Moo
{


INLINE const BW::string &Material::identifier() const
{
	return identifier_;
}

INLINE uint32 Material::numTextureStages() const
{
	return static_cast<uint32>( textureStages_.size() );
}


INLINE bool Material::alphaBlended() const
{
	return doAlphaBlend_;
}

INLINE void Material::alphaBlended( bool blended )
{
	doAlphaBlend_ = blended;
}


INLINE Material::BlendType Material::srcBlend() const
{
	return srcBlend_;
}

INLINE void Material::srcBlend( BlendType blendType )
{
	srcBlend_=blendType;
}


INLINE Material::BlendType Material::destBlend() const
{
	return destBlend_;
}

INLINE void Material::destBlend( BlendType blendType )
{
	destBlend_=blendType;
}


INLINE const Colour &Material::ambient() const
{
	return ambient_;
}

INLINE void Material::ambient( const Colour &c )
{
	ambient_ = c;
}


INLINE const Colour &Material::diffuse() const
{
	return diffuse_;
}

INLINE void Material::diffuse( const Colour &c )
{
	diffuse_ = c;
}


INLINE const Colour &Material::specular() const
{
	return specular_;
}

INLINE void Material::specular( const Colour &s )
{
	specular_ = s;
}


INLINE uint32 Material::alphaReference() const
{
	return alphaReference_;
}

INLINE void Material::alphaReference( uint32 alpha )
{
	alphaReference_ = alpha;
}


INLINE bool Material::alphaTestEnable() const
{
	return alphaTestEnable_;
}

INLINE void Material::alphaTestEnable( bool b )
{
	alphaTestEnable_ = b;
}


INLINE bool Material::zBufferRead() const
{
	return zBufferRead_;
}

INLINE void Material::zBufferRead( bool b )
{
	zBufferRead_ = b;
}


INLINE bool Material::zBufferWrite() const
{
	return zBufferWrite_;
}

INLINE void Material::zBufferWrite( bool b )
{
	zBufferWrite_ = b;
}


INLINE bool Material::solid() const
{
	return !!( ((channelFlags_ & channelMask_) | channelOn_) & Material::SOLID );
}

INLINE void Material::solid( bool b )
{
	if (b)
	{
		//note setting solid unsets sorted
		channelFlags_ |= Material::SOLID;
		channelFlags_ &= ~Material::SORTED;
	}
	else
		//note you can turn off solid, and leave sorted off too
		channelFlags_ &= ~Material::SOLID;	
}


INLINE bool Material::sorted() const
{
	return !!( ((channelFlags_ & channelMask_) | channelOn_) & Material::SORTED );
}

INLINE void Material::sorted( bool b )
{
	if (b)
	{
		//note setting sorted unsets solid
		channelFlags_ |= Material::SORTED;
		channelFlags_ &= ~Material::SOLID;
	}
	else
		//note you can turn off sorted, and leave solid off too
		channelFlags_ &= ~Material::SORTED;
}

INLINE bool Material::flare() const
{
	return !!( ((channelFlags_ & channelMask_) | channelOn_) & Material::FLARE );
}

INLINE void Material::flare( bool b )
{
	if (b)
		channelFlags_ |= Material::FLARE;
	else
		channelFlags_ &= ~Material::FLARE;
}

INLINE bool Material::shimmer() const
{
	return !!( ((channelFlags_ & channelMask_) | channelOn_) & Material::SHIMMER );
}

INLINE void Material::shimmer( bool b )
{
	if (b)
		channelFlags_ |= Material::SHIMMER;
	else
		channelFlags_ &= ~Material::SHIMMER;
}

INLINE Material::UV2Generator Material::uv2Generator() const
{
	return uv2Generator_;
}

INLINE void Material::uv2Generator( UV2Generator mode )
{
	IF_NOT_MF_ASSERT_DEV( mode < Material::LAST_UV2_GENERATOR )
	{
		MF_EXIT( "invalid mode" );
	}
	uv2Generator_ = mode;
}


INLINE Material::UV2Angle Material::uv2Angle() const
{
	return uv2Angle_;
}

INLINE void Material::uv2Angle( UV2Angle mode )
{
	IF_NOT_MF_ASSERT_DEV( mode < Material::LAST_UV2_ANGLE )
	{
		MF_EXIT( "invalid mode" );
	}
	uv2Angle_ = mode;
}


INLINE uint32 Material::textureFactor() const
{
	return textureFactor_;
}

INLINE void Material::textureFactor( uint32 factor )
{
	textureFactor_ = factor;
}


INLINE bool Material::fogged() const
{
	return fogged_;
}

INLINE void Material::fogged( bool status )
{
	fogged_ = status;
}

INLINE bool Material::doubleSided() const
{
	return doubleSided_;
}

INLINE void Material::doubleSided( bool b )
{
	doubleSided_ = b;
}

INLINE uint32 Material::collisionFlags() const
{
	return collisionFlags_;
}

INLINE void Material::collisionFlags( uint32 f )
{
	collisionFlags_ = f;
}

INLINE uint8 Material::materialKind() const
{
	if (collisionFlags_ == -1)
		return 0;
	else
		return uint8( collisionFlags_ >> 8 );
}

INLINE void Material::materialKind( uint8 k )
{
	collisionFlags_ &= ~(0xff << 8);
	collisionFlags_ |= k << 8;
}



}

// material.ipp