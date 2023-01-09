#ifndef SYS_MEM_TEXTURE_HPP
#define SYS_MEM_TEXTURE_HPP

#include "base_texture.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements a system memory texture.
 */
class SysMemTexture : public BaseTexture
{
public:
	SysMemTexture( const BW::StringRef & resourceID, D3DFORMAT format = D3DFMT_UNKNOWN );
	SysMemTexture( BinaryPtr data, D3DFORMAT format = D3DFMT_UNKNOWN );
	SysMemTexture( DX::Texture* pTex );

	DX::BaseTexture*	pTexture( ) { return pTexture_.pComObject(); }
	uint32				width( ) const { return width_; }
	uint32				height( ) const { return height_; }
	D3DFORMAT			format( ) const { return format_; }
	uint32				textureMemoryUsed( ) const { return 0; }
	const BW::string&	resourceID( ) const { return resourceID_; }
	void				load( );
	void				load(BinaryPtr data);
	void				release();

	void				reload();
	void				reload( const BW::StringRef & resourceID );

	uint32				maxMipLevel() const { return 0; }
	void				maxMipLevel( uint32 level ) {}

private:
	virtual void destroy() const;
	~SysMemTexture();

	ComObjectWrap< DX::Texture >	pTexture_;
	uint32							width_;
	uint32							height_;
	D3DFORMAT						format_;
	BW::string						resourceID_;
	BW::string						qualifiedResourceID_;
	bool							reuseOnRelease_;

	SysMemTexture( const SysMemTexture& );
	SysMemTexture& operator=( const SysMemTexture& );
};

} // namespace Moo

BW_END_NAMESPACE


#endif // SYS_MEM_TEXTURE_HPP
