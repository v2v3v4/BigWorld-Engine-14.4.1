#include "pch.hpp"

#include "scene_draw_context.hpp"

#ifndef CODE_INLINE
#include "scene_draw_context.ipp"
#endif

#include "moo/render_context.hpp"

namespace BW
{

SceneDrawContext::SceneDrawContext( const Scene & scene, Moo::DrawContext & drawContext ) :
	scene_( scene ),
	drawContext_( drawContext ),
	renderFlares_( true )
{
	lodCameraView( Moo::rc().view() );
}


} // namespace BW
