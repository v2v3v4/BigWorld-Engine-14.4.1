#include "pch.hpp"
#include "texture_renderer.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TextureSceneInterface
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TextureSceneInterface::TextureSceneInterface() :
	skipFrames_( 0 )
{
	BW_GUARD;
}

/**
 *	Destructor.
 */
TextureSceneInterface::~TextureSceneInterface()
{
	BW_GUARD;
	TextureRenderer::clearCachable();
}

// -----------------------------------------------------------------------------
// Section: TextureRenderer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TextureRenderer::TextureRenderer( int width, int height, D3DFORMAT format ) :
	width_( width ),
	height_( height ),
	format_( format ),
	clearTargetEachFrame_( true )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
TextureRenderer::~TextureRenderer()
{
	BW_GUARD;
	TextureRenderer::delDynamic( this );
}

/**
 *	Makes sure the render target has been created
 */
void TextureRenderer::touchRenderTarget()
{
	BW_GUARD;
	// make the render target if necessary
	if (!pRT_)
	{
		pRT_ = new Moo::RenderTarget( "TextureRenderer" );
		pRT_->create( width_, height_, false, format_ );
	}
}

/**
 *	This method does the render into this texture
 */
void TextureRenderer::render( float dTime )
{
	BW_GUARD;

	static DogWatch dwTextureRender("TextureRenderer");
	ScopedDogWatch dogWatch( dwTextureRender );

	this->touchRenderTarget();
	MF_ASSERT( pRT_ != NULL );

	// Set all textures to NULL, to make sure the render target
	// is not still one of our set textures.
	for( uint32 i = 0; i < Moo::rc().maxSimTextures(); i++ )
	{
		Moo::rc().setTexture( i, NULL );
	}

	// set and clear the render target
	if ( pRT_->push() )
	{
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
		Moo::rc().setWriteMask( 1, 0 );

		Moo::rc().beginScene();
		if (Moo::rc().mixedVertexProcessing())
			Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);
		if ( clearTargetEachFrame_ )
		{
			Moo::rc().device()->Clear(
				0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.f, 0 );
		}
		Moo::rc().updateProjectionMatrix();	// view, etc. left to renderSelf

		// do the draw
		this->renderSelf( dTime );

		// and reset everything
		Moo::rc().endScene();
		Moo::rc().popRenderState();
		pRT_->pop();
	}
}


/**
 *	Static method to add a dynamic texture renderer
 */
void TextureRenderer::addDynamic( TextureSceneInterface * pTR )
{
	BW_GUARD;
	TextureRenderer::delDynamic( pTR );
	s_dynamicRenderers_.push_back( pTR );
}


/**
 *	Static method to remove a dynamic texture renderer
 */
void TextureRenderer::delDynamic( TextureSceneInterface * pTR )
{	
	BW_GUARD_PROFILER( TextureRenderer_delDynamic );
	TextureRenderers::iterator found = std::find(
		s_dynamicRenderers_.begin(), s_dynamicRenderers_.end(), pTR );
	if (found != s_dynamicRenderers_.end())
	{
		s_dynamicRenderers_.erase( found );
	}
}


/**
 *	Static method to add a staggered dynamic texture renderer
 */
void TextureRenderer::addStaggeredDynamic( TextureSceneInterface * pTR )
{
	BW_GUARD;
	if (!pTR)
		return;

	TextureRenderer::delStaggeredDynamic( pTR );

	if (pTR->cachable())
		s_cachableRenderers_.push_back( pTR );
	else
		s_staggeredRenderers_.push_back( pTR );
}


/**
 *	Static method to remove a staggered dynamic texture renderer
 */
void TextureRenderer::delStaggeredDynamic( TextureSceneInterface * pTR )
{
	BW_GUARD_PROFILER( TextureRenderer_delStaggeredDynamic );
	if (!pTR)
		return;

	TextureRenderers& rendererList = pTR->cachable() ?
										s_cachableRenderers_ :
										s_staggeredRenderers_;

	TextureRenderers::iterator found = std::find(
		rendererList.begin(), rendererList.end(), pTR );

	if (found != rendererList.end())
		rendererList.erase( found );
}


/**
 *	Static method to add a staggered dynamic texture renderer
 */
void TextureRenderer::addCachable( TextureSceneInterface * pTR )
{
	BW_GUARD;
	s_currentFrame_.push_back( pTR );
}


/**
 *	Static method to remove a staggered dynamic texture renderer
 */
void TextureRenderer::clearCachable( )
{
	BW_GUARD;
	s_currentFrame_.clear();
}


/**
 *	Static method to update all the dynamic texture renderers
 */
void TextureRenderer::updateDynamics( float dTime )
{
	BW_GUARD_PROFILER( TextureRenderer_updateDynamics );
	static uint frameCount = 0;
	frameCount++;

	size_t sz = int(s_dynamicRenderers_.size());
	for (size_t i = 0; i < sz; i++)
	{
		if( (frameCount + i) % ( s_dynamicRenderers_[ i ]->skipFrames() + 1 ) != 0 )
			continue;

		// render this dynamic renderer
		size_t osz = sz;
		TextureSceneInterface * pTR = s_dynamicRenderers_[i];		
		pTR->render( dTime );

		// go back one if this renderer removed itself
		sz = s_dynamicRenderers_.size();
		if ( (sz + 1) == osz && (i + 1) != sz && s_dynamicRenderers_[ i ] != pTR )
		{
			i--;
		}
	}

	//update just one staggered dynamic renderer	
	if ( !s_staggeredRenderers_.empty() )
	{
		staggerIdx_ = ( staggerIdx_ + 1 ) % s_staggeredRenderers_.size();
		size_t ss = s_staggeredRenderers_.size();
		for (uint i = 0; i < ss; i++)
		{
			uint actualIndex = (i + staggerIdx_) % ss;

			TextureSceneInterface * pTR = s_staggeredRenderers_[ actualIndex ];
			if (pTR->shouldDraw())
			{
				pTR->render( dTime );
				staggerIdx_ = actualIndex;
				break;
			}
		}
	}
}


/**
 *	Static method to update all the cachable dynamic texture renderers
 */
void TextureRenderer::updateCachableDynamics( float dTime )
{
	BW_GUARD;
	if ( !s_cachableRenderers_.empty() )
	{
		stagger_cache_Idx_ = 
			( stagger_cache_Idx_ + 1 ) % s_cachableRenderers_.size();		
		size_t ss = s_cachableRenderers_.size();
		for (uint i = 0; i < ss; i++)
		{
			uint actualIndex = (i + stagger_cache_Idx_) % ss;

			TextureSceneInterface * pTR = s_cachableRenderers_[ actualIndex ];

			if (pTR->shouldDraw())
			{
				addCachable( pTR );
				stagger_cache_Idx_ = actualIndex;
				break;
			}
		}
	}



	// update the cachable staggered renderers
	size_t ss = s_currentFrame_.size();	
	for (uint i = 0; i < ss; i++)
	{
		TextureSceneInterface * pTR = s_currentFrame_[i];
		pTR->render( dTime );
	}
	clearCachable();
}


void TextureSceneInterface::skipFrames( int newSkipFrames )
{
	skipFrames_ = std::max( newSkipFrames, 0 );
}


// static initialiser
TextureRenderer::TextureRenderers TextureRenderer::s_dynamicRenderers_;
TextureRenderer::TextureRenderers TextureRenderer::s_staggeredRenderers_;
TextureRenderer::TextureRenderers TextureRenderer::s_cachableRenderers_;
TextureRenderer::TextureRenderers TextureRenderer::s_currentFrame_;
int TextureRenderer::staggerIdx_ = 0;
int TextureRenderer::stagger_cache_Idx_ = 0;

BW_END_NAMESPACE

// texture_renderer.cpp
