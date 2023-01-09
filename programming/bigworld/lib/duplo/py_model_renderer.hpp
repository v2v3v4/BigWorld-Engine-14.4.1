#ifndef PY_MODEL_RENDERER_HPP
#define PY_MODEL_RENDERER_HPP

#include "moo/texture_renderer.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/stl_to_py.hpp"
#include "chunk_attachment.hpp"


BW_BEGIN_NAMESPACE

class ChunkSpace;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;

namespace Moo
{
	class LightContainer;
	typedef SmartPointer<LightContainer>	LightContainerPtr;
};

/*~ class BigWorld.PyModelRenderer
 *
 *	This class is used to render a model to a PyTextureProvider, which can be
 *  used by gui components, such as a SimpleGUIComponent to render that model
 *  to the screen.  It can either render a single frame of a model or provide a
 *  dynamic PyTextureProvider which updates every frame.
 *
 *	In order for a model to be rendered, it must not be attached to anything,
 *	which means that it cannot be in the world.
 *
 *	The camera is positioned at (0,1,-1), looking at (0,-1,1). It cannot be 
 *	configured.
 *
 *	 The models scale, position and rotation attributes can
 *	be used to setup the model in the correct position.  The model can have
 *	Actions playing on it.  
 *
 *	Multiple models are all added to the same render space, and rendered at
 *	whatever position they are placed at.
 *
 *	The background for the render has an alpha of zero.
 *
 *	A new PyModelRenderer is created using the BigWorld.PyModelRenderer function.
 *
 *	Example:
 *	@{
 *	# create a new model to render
 *	mod = BigWorld.Model( "objects/models/characters/biped.model" )
 *
 *	# create the model renderer
 *	mr = BigWorld.PyModelRenderer( 256, 256 )
 *	mr.models = [ mod ]
 *
 *	# the pyModelRenderer camera is setup well to look at a unit cube.
 *	# here we make the model fit into that cube
 *	mod.zoomExtents()
 *
 *	# setting the renderer to dynamic means it will update every frame.
 *	# this is good for animating models, or if there is a motor attached
 *	# (for example an oscillator, which makes it spin around)
 *	# usage : mr.dynamic = 1
 *
 *	# however in this case, the biped model isn't animating, and there is
 *	# no motor attached.  therefore we have to call render() explicitly.
 *	# Note that render MUST be called before accessing the texture() attribute.
 *	mr.dynamic = 0
 *	mr.render()
 *
 *	# create a component to display it in
 *	cmp = GUI.Simple( "somedummytexture.bmp" )
 *	cmp.texture = mr.texture
 *	GUI.addRoot( cmp )
 *	@}
 */
/**
 *	This class renders PyModels into a texture.
 */
class PyModelRenderer : public PyObjectPlusWithWeakReference, public TextureRenderer
{
	Py_Header( PyModelRenderer, PyObjectPlusWithWeakReference )
public:
	class ModelHolder : public PySTLSequenceHolder<ChunkEmbodiments>
	{
	public:
		ModelHolder( ChunkEmbodiments & seq,PyObject * pOwnerIn, bool writableIn ):
			PySTLSequenceHolder<ChunkEmbodiments>( seq, pOwnerIn, writableIn ){}
		int insert( PyObject * pItem );
	};

	PyModelRenderer( int width, int height, PyTypeObject * pType = &s_type_ );
	virtual ~PyModelRenderer();

	virtual void renderSelf( float dTime );

	bool dynamic() const					{ return dynamic_; }
	void dynamic( bool b );

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	PY_AUTO_METHOD_DECLARE( RETVOID, render, OPTARG( float, 0.f, END ) )

	PY_DEFERRED_ATTRIBUTE_DECLARE( models )
	PyObject * pyGet_texture();
	PY_RO_ATTRIBUTE_SET( texture )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, dynamic, dynamic )

	PY_FACTORY_DECLARE()

	static void fini();

private:
	PyModelRenderer( const PyModelRenderer& );
	PyModelRenderer& operator=( const PyModelRenderer& );

	ChunkEmbodiments						models_;
	ModelHolder modelsHolder_;
	bool				dynamic_;
	Moo::DrawContext*	drawContext_;

public:
	static ChunkSpacePtr	s_pSpace_;
	static Moo::LightContainerPtr	s_lighting_;
	static Moo::LightContainerPtr	s_specularLighting_;
};

BW_END_NAMESPACE

#endif // PY_MODEL_RENDERER_HPP
