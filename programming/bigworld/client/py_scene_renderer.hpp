#ifndef PY_SCENE_RENDERER_HPP
#define PY_SCENE_RENDERER_HPP

#include "moo/texture_renderer.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/stl_to_py.hpp"
#include "camera/base_camera.hpp"


BW_BEGIN_NAMESPACE

class ChunkSpace;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;

namespace Moo
{
	class LightContainer;
	typedef SmartPointer<LightContainer>	LightContainerPtr;
};


/*~ class BigWorld.PySceneRenderer
 *
 *	This class is used to render the world, as visible through specified cameras
 *	(which inherit from BaseCamera),
 *	onto a PyTextureProvider.  The size of the texture is specified when the
 *	PySceneRenderer is created (using the BigWorld.PySceneRenderer function).
 *	If more than one camera is specified, then the texture is divided 
 *	horizontally between them, with each camera getting a portion to render to.
 *
 *	If the dynamic attribute is non-zero (true), then the texture is updated 
 *	every tick to reflect the scene showing in each camera.  If dynamic is 
 *	zero (false) then the texture is only updated when the render function is 
 *	called.
 *
 *	For example:
 *	@{
 *	psr = BigWorld.PySceneRenderer( 100, 100 )
 *	psr.cameras = [ BigWorld.camera() ]
 *	psr.dynamic = 1
 *	cmp = GUI.Simple( "some_random_starting_texture.dds" )
 *	cmp.texture = psr.texture
 *	@}
 *	This example causes the cmp SimpleGUIComponent to render a mini view
 *	in the middle of the screen similar to what is showing in the main camera.
 */
/**
 *	This class renders the scene from a number of given cameras into a texture.
 *	Only one texture is used, this is divided into how every many sections we need
 *	for however many cameras.  Only one camera renders per frame.
 */
class PySceneRenderer : public PyObjectPlusWithWeakReference, public TextureRenderer
{
	Py_Header( PySceneRenderer, PyObjectPlusWithWeakReference )
public:
	PySceneRenderer( int width, int height, PyTypeObject * pType = &s_type_ );
	virtual ~PySceneRenderer();

	virtual void renderSelf( float dTime );

	bool dynamic() const	{ return dynamic_; }
	void dynamic( bool state );

	bool staggered() const	{ return staggered_; }
	void staggered( bool state );

	PY_AUTO_METHOD_DECLARE( RETVOID, render, OPTARG( float, 0.f, END ) )

	PY_RW_ATTRIBUTE_DECLARE( camerasHolder_, cameras )
	PY_RW_ATTRIBUTE_DECLARE( fovRadians_, fov )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, dynamic, dynamic )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, staggered, staggered )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, skipFrames, skipFrames )
	PY_RW_ATTRIBUTE_DECLARE( drawSplodges_, drawSplodges )
	PyObject * pyGet_texture();
	PY_RO_ATTRIBUTE_SET( texture )

	PY_FACTORY_DECLARE()

	typedef BW::vector< BaseCameraPtr > Cameras;

private:
	PySceneRenderer( const PySceneRenderer& );
	PySceneRenderer& operator=( const PySceneRenderer& );

	Cameras							cameras_;
	PySTLSequenceHolder<Cameras>	camerasHolder_;
	int								idx_;	
	float							fovRadians_;

	bool							dynamic_;
	bool							staggered_;
	bool							drawSplodges_;
	Moo::DrawContext*				drawContext_;


public:
	static Moo::LightContainerPtr	s_lighting_;
};


void PySceneRendererCamera_release( BaseCamera *&pCamera );
/*
 *	Wrapper to let the C++ vector of cameras work as a vector/list in Python.
 */
namespace PySTLObjectAid
{
	/// Releaser is same for all PyObject's
	template <> struct Releaser< BaseCamera * >
	{
		static void release( BaseCamera *&pCamera )
        {
        	BW_GUARD;
			PySceneRendererCamera_release( pCamera );
        }
	};
}

BW_END_NAMESPACE


#endif // PY_SCENE_RENDERER_HPP
