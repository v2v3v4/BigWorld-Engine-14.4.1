#include "pch.hpp"

#include "py_font.hpp"
#include "font_manager.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_font.ipp"
#endif

PY_TYPEOBJECT( PyFont )

PY_BEGIN_METHODS( PyFont )
	/*~ function PyFont.stringWidth
	 *	@components{ client, tools }
	 *
	 *	This function returns the width (in pixels) that the specified string
	 *	will take when rendered to a TextGUIComponent using this font.
	 *
	 *	@param text	The string to measure the length of.
	 *	
	 *	@param multiline Optional boolean.
	 *		Whether or not newline characters should be wrapped.
	 *	
	 *	@param colourFormatting Optional boolean.
	 *		Whether or not this string is using formatting tags.
	 *
	 *	@return		An integer, the rendered width of the string in pixels.
	 */
	PY_METHOD( stringWidth )
	/*~ function PyFont.stringDimensions
	 *	@components{ client, tools }
	 *
	 *	This function returns a 2-tuple containing the dimensions (in pixels)
	 *	that the specified string will take when rendered to a
	 *	TextGUIComponent using this font.
	 *
	 *	@param text	The string to measure the length and height of.
	 *	
	 *	@param multiline Optional boolean.
	 *		Whether or not newline characters should be wrapped.
	 *	
	 *	@param colourFormatting Optional boolean.
	 *		Whether or not this string is using formatting tags.
	 *
	 *	@return		A 2-tuple of integers, the render width and height 
	 *				of the string in pixels.
	 */
	PY_METHOD( stringDimensions )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFont )
	/*~ attribute PyFont.name
	 *	@components{ client, tools }
	 *	
	 *	Get the name of this font.
	 *	
	 *	@return name of the font.
	 */
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Font
 *	@components{ client,  tools }
 *	Factory function to create and return a PyFont object.
 *
 *	@param name the resource id.
 *
 *	@return A new PyFont object.
 */
PY_FACTORY_NAMED( PyFont, "Font", BigWorld )

/**
 *	Create a PyFont with a font pointer.
 */
PyFont::PyFont(
	CachedFontPtr pFont,
	PyTypeObject * pType ) :

	PyObjectPlus( pType ),
	pFont_( pFont )
{
}

PyFont::~PyFont()
{
}


PyObject* PyFont::pyNew( PyObject* args )
{
	BW_GUARD;
	CachedFontPtr pFont;
	char * name;

	if ( PyArg_ParseTuple( args, "s", &name ) )
	{
		pFont = FontManager::instance().getCachedFont( name );

		if ( !pFont )
		{
			PyErr_SetString( PyExc_ValueError, "BigWorld.Font: "
				"Could not load font." );
			return NULL;
		}
	}

	if ( !pFont )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.Font: "
			"Argument parsing error: Expected a name." );
		return NULL;
	}

	return new PyFont( pFont );
}

BW_END_NAMESPACE

// py_font.cpp
