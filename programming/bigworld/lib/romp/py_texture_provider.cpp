#include "pch.hpp"

#include "py_texture_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyTextureProvider
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyTextureProvider )
PY_BEGIN_METHODS( PyTextureProvider )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyTextureProvider )
	/*~ attribute PyTextureProvider.owner
	 *	@components{ client, tools }
	 *
	 *	This attribute is the owner of the PyTextureProvider.  In the case
	 *	of a PyTextureProvider created directly using the
	 *	BigWorld.PyTextureProvider function, this is None. In the case of
	 *	an object which exposes a PyTextureProvider, such as a PyModelRenderer,
	 *	the owner is the object which does the exposing.
	 *
	 *	For example:
	 *	@{
	 *	>>>	mod = BigWorld.Model( "models/man.model" )
	 *	>>>	modRend = BigWorld.PyModelRenderer(100, 100)
	 *	>>>	modRend.models = [ mod ]
	 *	>>>	tex = modRend.texture
	 *	>>>	tex.owner
	 *	PyModelRender at 0x00ff4565
	 *	@}
	 *	This example gets the PyTextureProvider out of a PyModelRenderer
	 *	and checks its owner, which is the PyModelRenderer.
	 *
	 *	@type	Read-Only Object
	 */
	PY_ATTRIBUTE( owner )
	/*~ attribute PyTextureProvider.width
	 *	@components{ client, tools }
	 *
	 *	This attribute is the texel width of the texture referenced by the
	 *  PyTextureProvider.
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute PyTextureProvider.height
	 *	@components{ client, tools }
	 *
	 *	This attribute is the texel height of the texture referenced by the
	 *  PyTextureProvider.
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( height )
	/*~ attribute PyTextureProvider.name
	 *	@components{ client, tools }
	 *
	 *	This attribute is the resource ID of the texture referenced by the
	 *  PyTextureProvider.
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyTextureProvider )

/*~ function BigWorld.PyTextureProvider
 *	@components{ client, tools }
 *
 *	This function creates a PyTextureProvider.  It takes a texture filename
 *	as an argument.  A PyTextureProvider is used to supply a texture to
 *	classes which need a texture, such as SimpleGUIComponent.
 *
 *	For example:
 *	@{
 *	tex = BigWorld.PyTextureProvider( "maps/fx/picture.bmp" )
 *	@}
 *	In the example, tex is set to be a PyTextureProvider which exposes the
 *	picture.bmp texture.
 *
 *	@param	resourceId	the filename of the texture to load.
 *
 *	@return				a new PyTextureProvider.
 */
PY_FACTORY( PyTextureProvider, BigWorld )


/**
 *	Special get method for python attribute 'width'
 */
PyObject * PyTextureProvider::pyGet_width()
{
	BW_GUARD;
	return Script::getData( pTexture_->width() );
}

/**
 *	Special get method for python attribute 'height'
 */
PyObject * PyTextureProvider::pyGet_height()
{
	BW_GUARD;
	return Script::getData( pTexture_->height() );
}


/**
 *	Factory method
 */
PyObject * PyTextureProvider::pyNew( PyObject * args )
{
	BW_GUARD;
	char * textureName;
	if (!PyArg_ParseTuple( args, "s", &textureName ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.PyTextureProvider() expects "
			"a string texture name argument" );
		return NULL;
	}

	return new PyTextureProvider(
		NULL, Moo::TextureManager::instance()->get( textureName ) );
}


static PyObject * Texture( const BW::string & path )
{
	BW_GUARD;
	// TODO: load in the background
	Moo::BaseTexturePtr newTexture = Moo::TextureManager::instance()->get(path);
	if (newTexture.exists())
	{
		return new PyTextureProvider( Py_None, newTexture );
	}
	else
	{
		PyErr_Format( PyExc_ValueError, 
			"BigWorld.Texture(): Could not "
			"load requested texture: %s", path.c_str() );

		return NULL;
	}
}

PY_AUTO_MODULE_FUNCTION( RETOWN, Texture, ARG( BW::string, END ), BigWorld )


BW_END_NAMESPACE

// simple_gui_component.cpp
