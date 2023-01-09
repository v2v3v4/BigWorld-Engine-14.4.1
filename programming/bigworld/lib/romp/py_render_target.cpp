#include "pch.hpp"
#include "py_render_target.hpp"
#include "pyscript/py_data_section.hpp"
#include "romp/py_texture_provider.hpp"
#include "py_d3d_enums.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_render_target.ipp"
#endif

// Python statics
PY_TYPEOBJECT( PyRenderTarget )

PY_BEGIN_METHODS( PyRenderTarget )
	/*~ attribute PyRenderTarget.release
	 *	@components{ client,  tools }
	 *	Release the render target memory.  The render target memory will
	 *	be automatically re-alloacted when next used for rendering.
	 */
	PY_METHOD( release )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyRenderTarget )
	/*~ attribute PyRenderTarget.width
	 *	@components{ client,  tools }
	 *	Read-only width of the render target, in pixels.
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute PyRenderTarget.height
	 *	@components{ client,  tools }
	 *	Read-only height of the render target, in pixels.
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( height )
	/*~ attribute PyRenderTarget.format
	 *	@components{ client,  tools }
	 *	Read-only D3DFormat of the render target.
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( format )
	/*~ attribute PyRenderTarget.textureMemoryUsed
	 *	@components{ client,  tools }
	 *	Read-only number of bytes used by the render target.
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( textureMemoryUsed )
	/*~ attribute PyRenderTarget.name
	 *	@components{ client,  tools }
	 *	Read-only name of the render target.
	 *	@type String.
	 */
	PY_ATTRIBUTE( name )
	/*~ attribute PyRenderTarget.texture
	 *	@components{ client,  tools }
	 *	Read-only texture of the render target.  As this is
	 *	a PyTextureProvider it will always provide the latest
	 *	version of the render target contents.  Use this to
	 *	hook up to other objects taking PyTextureProviders, for
	 *	example GUI components, PostProcessing FilterQuads etc.
	 *	@type PyTextureProvider.
	 */
	PY_ATTRIBUTE( texture )
PY_END_ATTRIBUTES()

/*~ function BigWorld.RenderTarget
 *	@components{ client,  tools }
 *	Factory function to create and return a PyRenderTarget object.
 *
 *	Note that width and height can be specified either in absolute pixels,
 *	or they can represents multipliers of the screen size.  These dimensions
 *	automatically adjust when the screen size changes.
 *
 *	w,h = 0  : use the screen size
 *	w,h = -1 : screen size / 2
 *	w,h = -2 : screen size / 4
 *	w,h = -n : screen size / pow(2,n)
 *
 *	@param	name	Name of the render target
 *	@param	width	Desired Width.  0, ... -n have special meaning (see above)
 *	@param	height	Desired Height.  0, ... -n have special meaning (see above)
 *	@param	reuseZ	(optional)Re-use the back-buffer's Z-buffer, or False to
 *	create a unique z-buffer for this render target.
 *	@param	format	(optional)D3DFORMAT enum value.
 *
 *	@return A new PyRenderTarget object.
 */
PY_FACTORY_NAMED( PyRenderTarget, "RenderTarget", BigWorld )


PyRenderTarget::PyRenderTarget( Moo::RenderTargetPtr pRT, PyTypeObject *pType ):
	PyObjectPlusWithWeakReference( pType ),
	pRenderTarget_( pRT )
{
	BW_GUARD;

}


PyRenderTarget::~PyRenderTarget()
{
	BW_GUARD;
}


PyObject* PyRenderTarget::pyNew(PyObject* args)
{
	BW_GUARD;
	PyObject * pObj = NULL;
	char * name;
	int width, height;
	bool reuseZ = false;
	D3DFORMAT fmt = D3DFMT_A8R8G8B8;
	PyObject* poFMT = Py_None;
	Moo::RenderTargetPtr rt = NULL;


	if (PyArg_ParseTuple( args, "sii|bO", &name, &width, &height, &reuseZ, &poFMT ))
	{
		// If our optional format property has been set, 
		// interpret it as a D3DFORMAT value
		if (poFMT != Py_None)
		{
			if (Script::setData( poFMT, fmt, "format" ) != 0)
			{
				// Not setting error string, as setData does this
				return NULL;
			}
		}

		Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get( name, false, true, false );
		if ( pTex.hasObject() )
		{
			rt = dynamic_cast<Moo::RenderTarget*>(pTex.getObject());
		}

		if ( !rt )
		{
			rt = new Moo::RenderTarget( name );
		}

		rt->create( width, height, reuseZ, fmt );
	}

	if (!rt)
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.PyRenderTarget: "
			"Argument parsing error: Expected a  name, width and height. "
			"You may also specify reuseZ(boolean) and format(D3DFORMAT)." );
		return NULL;
	}

	return new PyRenderTarget( rt );
}

PY_SCRIPT_CONVERTERS( PyRenderTarget )

BW_END_NAMESPACE

// py_render_target.cpp
