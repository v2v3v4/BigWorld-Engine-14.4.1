#ifndef SCENE_DRAW_CONTEXT_HPP
#define SCENE_DRAW_CONTEXT_HPP

#include "math/matrix.hpp"

namespace BW
{

namespace Moo
{
	class DrawContext;
}

class Scene;

class SceneDrawContext
{
public:
	SceneDrawContext( const Scene & scene, Moo::DrawContext & drawContext );

	const Scene & scene() const;
	Moo::DrawContext & drawContext() const;

	bool renderFlares() const;
	void renderFlares( bool b );

	void lodCameraView( const Matrix& view );
	const Matrix& lodCameraView() const;

private:
	const Scene & scene_;
	Moo::DrawContext & drawContext_;
	Matrix lodCameraView_;

	bool renderFlares_;
};

} // namespace BW

#ifdef CODE_INLINE
#include "scene_draw_context.ipp"
#endif

#endif // SCENE_DRAW_CONTEXT_HPP
