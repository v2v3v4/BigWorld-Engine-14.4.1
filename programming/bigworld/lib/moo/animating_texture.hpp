#ifndef ANIMATING_TEXTURE_HPP
#define ANIMATING_TEXTURE_HPP

#include <iostream>
#include "base_texture.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	Class representing an animated texture.
 *	Stored as a list of frames with associated timing.
 */
class AnimatingTexture : public BaseTexture
{
public:
	AnimatingTexture(	const BW::StringRef & resourceID = BW::StringRef(),
						const BW::StringRef & allocator = "texture/unknown animation texture" );

	void									open( const BW::StringRef & resourceID );

	DX::BaseTexture*						pTexture( );
	uint32									width( ) const;
	uint32									height( ) const;
	D3DFORMAT								format( ) const;
	uint32									textureMemoryUsed() const;
	const BW::string&						resourceID( ) const;

	static void								tick( float dTime );

	virtual void							reload( );
	virtual void							reload( const BW::StringRef & resourceID );

	bool									isAnimated() { return true; }

private:
	virtual void destroy() const;
	~AnimatingTexture();

	void									calculateFrame( );
	BW::string								resourceID_;
	BW::vector< BaseTexturePtr >			textures_;
	float									fps_;
	uint64									lastTime_;
	float									animFrame_;
	uint32									frameTimestamp_;

	static uint64							currentTime_;

	AnimatingTexture(const AnimatingTexture&);
	AnimatingTexture& operator=(const AnimatingTexture&);

	friend std::ostream& operator<<(std::ostream&, const AnimatingTexture&);
};

} // namespace Moo

#ifdef CODE_INLINE
#include "animating_texture.ipp"
#endif

BW_END_NAMESPACE


#endif
/*animating_texture.hpp*/
