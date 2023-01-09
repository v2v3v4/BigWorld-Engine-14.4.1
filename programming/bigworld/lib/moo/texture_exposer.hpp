#ifndef TEXTURE_EXPOSER_HPP
#define TEXTURE_EXPOSER_HPP

#include "moo/base_texture.hpp"
#include "moo/texture_lock_wrapper.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 * This class locks a DX texture and caches information for a given LOD
 * level.
 */
class TextureExposer
{
public:
	TextureExposer( Moo::BaseTexturePtr tx );
	~TextureExposer();

	bool level( int lod );

	D3DFORMAT format() const		{ return levFormat_; }
	int width() const				{ return levWidth_; }
	int height() const				{ return levHeight_; }

	int pitch() const				{ return levPitch_; }
	const char * bits() const		{ return levBits_; }

private:
	DX::BaseTexture*	pDXBaseTexture_;
	DX::Texture*		pDXTexture_;
	int					rectLocked_;
	TextureLockWrapper	textureLockWrapper_;

	D3DFORMAT	levFormat_;
	int			levWidth_;
	int			levHeight_;
	int			levPitch_;
	char *		levBits_;
};


} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_EXPOSER_HPP
