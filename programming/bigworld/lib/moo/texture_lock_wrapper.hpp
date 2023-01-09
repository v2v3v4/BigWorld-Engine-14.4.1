#ifndef TEXTURE_LOCK_WRAPPER_HPP
#define TEXTURE_LOCK_WRAPPER_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{
#ifndef MF_SERVER
/**
 *	Texture lock wrapper to make code IDirect3DDevice9Ex device compatible
 *	It will use staging texture and propagate changes back to original texture
 *  if IDirect3DDevice9Ex device is in use and original texture isn't lockable
*/
class TextureLockWrapper
{
public:
	TextureLockWrapper( const ComObjectWrap<DX::Texture>& tex = NULL );
	~TextureLockWrapper();
	void	init( const ComObjectWrap<DX::Texture>& tex );
	void	fini();

	HRESULT	lockRect( UINT level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD flags );
	HRESULT	unlockRect( UINT level );
	void	flush();

private:
	// 32 since must fit in bit masks
	static const uint32 MAX_LEVELS = 32;

	ComObjectWrap<DX::Texture>	texture_;
	ComObjectWrap<DX::Texture>	stagingTexture_;

	uint32						lockedMask_;		// currently locked levels
	uint32						dirtyMask_;			// dirty levels pending flush
	uint32						dirtyRectsMask_;	// dirty levels with recorded rects

	RECT						dirtyRects_[MAX_LEVELS];	// user-provided rects per level

};
/**
 *	CubeTexture lock wrapper to make code IDirect3DDevice9Ex device compatible
 *	It will use staging texture and propagate changes back to original texture
 *  if IDirect3DDevice9Ex device is in use and original cube texture isn't lockable
*/
class CubeTextureLockWrapper
{
public:
	CubeTextureLockWrapper( const ComObjectWrap<DX::CubeTexture>& tex = NULL );
	~CubeTextureLockWrapper();
	void	init( const ComObjectWrap<DX::CubeTexture>& tex );
	void	fini();

	HRESULT	lockRect( D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT* pLockedRect, DWORD flags );
	HRESULT	unlockRect( D3DCUBEMAP_FACES face, UINT level );
	void	flush();

private:
	static const int	NUM_CUBE_FACES = 6;
	ComObjectWrap<DX::CubeTexture>	texture_;
	ComObjectWrap<DX::CubeTexture>	stagingTexture_;
	uint32							lockedMask_[NUM_CUBE_FACES];
	uint32							dirtyMask_[NUM_CUBE_FACES];
};
#endif

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_LOCK_WRAPPER_HPP

