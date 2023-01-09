#ifndef TEXTURE_RENDERER_HPP
#define TEXTURE_RENDERER_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/render_target.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class TextureSceneInterface
{
protected:
	TextureSceneInterface();
	virtual ~TextureSceneInterface();

public:
	virtual bool shouldDraw() const = 0;
	virtual void render( float dTime ) = 0;
	virtual bool cachable() const = 0;

	virtual uint skipFrames() const { return skipFrames_; }
	virtual void skipFrames( int value );
protected:
	uint					skipFrames_;
};

/**
 *	This class manages the rendering of things into a texture.
 *	They can be static or dynamic; if dynamic they are re-rendered
 *	every frame.
 */
class TextureRenderer : public TextureSceneInterface
{
public:
	TextureRenderer( int width, int height, D3DFORMAT format = D3DFMT_A8R8G8B8 );
	virtual ~TextureRenderer();

	virtual void render( float dTime = 0.f );
	virtual void renderSelf( float dTime ) = 0;

	static void addDynamic( TextureSceneInterface * pTR );
	static void delDynamic( TextureSceneInterface * pTR );
	static void addStaggeredDynamic( TextureSceneInterface * pTR );
	static void delStaggeredDynamic( TextureSceneInterface * pTR );
	static void updateDynamics( float dTime );

	static void clearCachable( );
	static void updateCachableDynamics( float dTime );

	virtual bool shouldDraw() const	{ return true; }
	virtual bool cachable() const { return false; }

	void touchRenderTarget();

private:
	TextureRenderer( const TextureRenderer& );
	TextureRenderer& operator=( const TextureRenderer& );

	typedef BW::vector<TextureSceneInterface*> TextureRenderers;
	static TextureRenderers s_dynamicRenderers_;
	static TextureRenderers s_staggeredRenderers_;
	static TextureRenderers s_cachableRenderers_;
	static TextureRenderers s_currentFrame_;
	static int staggerIdx_;
	static int stagger_cache_Idx_;

protected:
	static void addCachable( TextureSceneInterface * pTR );
	Moo::RenderTargetPtr	pRT_;
	int						width_;
	int						height_;
	D3DFORMAT				format_;
	bool					clearTargetEachFrame_;
};

BW_END_NAMESPACE

#endif // TEXTURE_RENDERER_HPP
