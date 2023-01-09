#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


namespace BW
{

INLINE const Scene & SceneDrawContext::scene() const 
{ 
	return scene_; 
}


INLINE Moo::DrawContext & SceneDrawContext::drawContext() const 
{
	return drawContext_; 
}


INLINE bool SceneDrawContext::renderFlares() const 
{ 
	return renderFlares_; 
}


INLINE void SceneDrawContext::renderFlares( bool b ) 
{ 
	renderFlares_ = b; 
}


INLINE void SceneDrawContext::lodCameraView( const Matrix& view )
{
	lodCameraView_ = view;
}


INLINE const Matrix& SceneDrawContext::lodCameraView() const
{
	return lodCameraView_;
}

}
