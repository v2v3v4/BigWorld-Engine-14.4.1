#include "pch.hpp"

#include "resmgr/bwresource.hpp"
#include "texture_manager.hpp"
#include "cstdmf/debug.hpp"

#include "render_context.hpp"

#include "animating_texture.hpp"
#ifndef CODE_INLINE
#include "animating_texture.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Moo
{

uint64 AnimatingTexture::currentTime_ = 0;

static const int64 ANIM_TEX_TICKS_PER_SECOND = 1000;


AnimatingTexture::AnimatingTexture(	const BW::StringRef & resourceID,
	const BW::StringRef & allocator)
: frameTimestamp_( 0 )
#if ENABLE_RESOURCE_COUNTERS
	, BaseTexture(allocator)
#endif
{
	BW_GUARD;
	if (!resourceID.empty())
		this->open( resourceID );
}

AnimatingTexture::~AnimatingTexture()
{
	BW_GUARD;
}


void AnimatingTexture::destroy() const
{
	// attempt to safety delete the texture
	this->tryDestroy();
}


/**
 * Reloads the animated texture.
 */
void AnimatingTexture::reload() 
{
	BW_GUARD;
	//Make sure we purge the cache first
	BWResource::instance().purge( resourceID_ );

	//Make sure we clear the old textures first
	textures_.clear();

	open( resourceID_ );

}

void	AnimatingTexture::reload( const BW::StringRef & resourceID )
{
	resourceID_.assign( resourceID.data(), resourceID.size() );
	reload();
}

/**
 * Opens an animating texture.
 *
 * @param resourceID the identifier of the animating texture resource
 *
 */
void AnimatingTexture::open( const BW::StringRef & resourceID )
{
	BW_GUARD;
	// open the texture section.
	DataSectionPtr animationSection = BWResource::openSection( resourceID );
	if (animationSection)
	{
		// store the identifier
		resourceID_.assign( resourceID.data(), resourceID.size() );

		// read the framerate of the animating texture
		fps_ = animationSection->readFloat( "fps", 10 );

		// read the names of the textures included in this animation
		BW::vector< BW::string > textureNames;
		animationSection->readStrings( "texture", textureNames );


		DataSectionPtr pFramesSection = animationSection->findChild("frames");

		if (pFramesSection)
		{
			// read the frames string for this animation.
			// the frames string is built up of characters from a-z each character
			// references a texture in the texturenames list, and gets converted at
			// load time, ( a references the first texture b the second etc. )
			BW::string s = pFramesSection->asString();

			// Map the frames string to actual textures.
			for (uint32 i = 0; i < s.length(); i++ )
			{
				uint32 n = (unsigned char)(s[ i ] - 'a');

				if (n < textureNames.size())
				{
#if ENABLE_RESOURCE_COUNTERS
					textures_.push_back( TextureManager::instance()->get( textureNames[n], false, true, true, allocator_.c_str() ) );
#else
					textures_.push_back( TextureManager::instance()->get( textureNames[n], false ) );
#endif
				}
				else
				{
					WARNING_MSG( "Moo::AnimatingTexture::open: Unable to add texture %c from %s\n", s[i], resourceID_.c_str() );
					textures_.push_back( NULL );
				}
			}
		}
		else
		{
			// If there is no frames string defined in the texanim file, just play the images back
			// in the order they appear in the file.
			for (uint32 i = 0; i < textureNames.size(); i++)
			{
#if ENABLE_RESOURCE_COUNTERS
				textures_.push_back( TextureManager::instance()->get( textureNames[i], false, true, true, allocator_.c_str() ) );
#else
				textures_.push_back( TextureManager::instance()->get( textureNames[i], false ) );
#endif
			}
		}

		lastTime_ = animationSection->readBool( "synced", false ) ?
			0 : currentTime_;
		animFrame_ = animationSection->readFloat( "firstFrame",
			( float( rand() ) / float( RAND_MAX ) ) * textures_.size() );
		animFrame_ = fmodf( fabsf( animFrame_ ), float( textures_.size() ) );
	}
}

/**
 * Returns a pointer to texture referenced by the current animFrame_.
 * Updates animFrame_ the first time the function gets called in a frame.
 *
 * @return pointer to the current texture.
 */
DX::BaseTexture* AnimatingTexture::pTexture( )
{
	BW_GUARD;
	calculateFrame();

	// Find the texture for this frame.
	BaseTexturePtr tp;
	uint32 uanim = uint32(animFrame_);
	if (uanim < textures_.size())
	{
		tp = textures_[uanim];
	}
	else
	{
		DEBUG_MSG( "Wrong animFrame %f %d\n", animFrame_, textures_.size() );
	}

	// If a texture was found, return it.
	if (tp)
		return tp->pTexture();

	// Return NULL if no texture was found.
	return NULL;
}

/**
 * Returns the width of the current texture.
 *
 * @return the width of the texture 0 if there is no texture.
 */
uint32 AnimatingTexture::width() const
{
	BW_GUARD;
	BaseTexturePtr tp;
	uint32 uanim = uint32(animFrame_);
	if (uanim < textures_.size())
	{
		tp = textures_[uanim];
	}
	if (tp)
		return tp->width();
	return 0;
}

/**
 * Returns the height of the current texture.
 *
 * @return the height of the texture 0 if there is no texture.
 */
uint32 AnimatingTexture::height() const
{
	BW_GUARD;
	BaseTexturePtr tp;
	uint32 uanim = uint32(animFrame_);
	if (uanim < textures_.size())
	{
		tp = textures_[uanim];
	}
	if (tp)
		return tp->height();
	return 0;
}

/**
 * Returns the format of the current texture.
 *
 * @return the format of the texture 0 if there is no texture.
 */
D3DFORMAT AnimatingTexture::format() const
{
	BW_GUARD;
	BaseTexturePtr tp;
	uint32 uanim = uint32(animFrame_);
	if (uanim < textures_.size())
	{
		tp = textures_[uanim];
	}
	if (tp)
		return tp->format();
	return D3DFMT_UNKNOWN;
}

/**
 * Calculates the current frame in the texture animation.
 * Only calculates this the first time the function gets called in a given frame.
 *
 */
void AnimatingTexture::calculateFrame()
{
	BW_GUARD;
	if (!rc().frameDrawn( frameTimestamp_ ))
	{
		float timeDelta = float( double(int64(currentTime_ - lastTime_)) /
			double(ANIM_TEX_TICKS_PER_SECOND) );
		animFrame_ += timeDelta * fps_;
		lastTime_ = currentTime_;
		animFrame_ = fmodf( animFrame_, float(textures_.size()) );
	}
}

/**
 *	This static method ticks over to the next frame.
 *	@param dTime the time since the last tick.
 */
void AnimatingTexture::tick( float dTime )
{
	currentTime_ += int64( double(dTime) * ANIM_TEX_TICKS_PER_SECOND );
}


/**
 *	This overrides BaseTexture::textureMemoryUsed()
 */
uint32 AnimatingTexture::textureMemoryUsed( ) const
{
	BW_GUARD;
	uint32 result = 0;

	BW::vector< BaseTexturePtr >::const_iterator beg = textures_.begin();
	BW::vector< BaseTexturePtr >::const_iterator it = beg;
	BW::vector< BaseTexturePtr >::const_iterator end = textures_.end();

	while ( it != end )
	{
		//  do not add it if the same texture appeared before
		if (std::find( beg, it, *it) == it && it->exists() )
		{
			result += BaseTexture::textureMemoryUsed( (*it)->pTexture() );
		}
		it++;
	}

	return result;
}

const BW::string& AnimatingTexture::resourceID() const
{
	return resourceID_;
}

} // namespace Moo

BW_END_NAMESPACE


// animating_texture.cpp
