#include "pch.hpp"
#include "texture_tool_view.hpp"

#include "common/material_utility.hpp"
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "ToolView", 0 );

BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE TextureToolView::

PY_TYPEOBJECT( TextureToolView )

PY_BEGIN_METHODS( TextureToolView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TextureToolView )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TextureToolView, "TextureToolView", View )

VIEW_FACTORY( TextureToolView )

static DogWatch s_textureToolView( "TextureToolView" );

/**
 *	Constructor.
 */
TextureToolView::TextureToolView( const BW::string& resourceID, PyTypeObject * pType ):
	ToolView( pType ),
	pMaterial_( new Moo::EffectMaterial )
{
	BW_GUARD;

	s_textureToolView.start();

	this->viewResource( resourceID );

	s_textureToolView.stop();
}


/**
 *	This method sets the texture name for the texture view.
 *
 *	@param textureResource	The name of the texture to use.
 */
void TextureToolView::viewResource(
	const BW::string& textureResource )
{
	BW_GUARD;

	DataSectionPtr pSection = BWResource::openSection( 
		"resources/materials/generic_tool.mfm" );
	if (pSection)
	{
		pMaterial_->load( pSection );
		MaterialUtility::setTexture( pMaterial_, 0, textureResource );	
		pTexture_ = Moo::TextureManager::instance()->get( textureResource );
	}
	else
	{
		ERROR_MSG( "TextureToolView::viewResource : Could not load \
				   resources/materials/generic_tool.mfm\n" );
	}
}


/**
 *	Update the animations in this tool view.
 *	@param tool the tool.
 */
void TextureToolView::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/** 
 *	This method is TextureToolView's own way of drawing the tool.
 *
 *	@param	tool	The tool to draw.
 */
void TextureToolView::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	Moo::rc().push();
	
	Matrix transform;
	viewTransform( tool, transform );
	Moo::rc().world( transform );

	pMaterial_->begin();
	for ( uint32 i=0; i<pMaterial_->numPasses(); i++ )
	{
		pMaterial_->beginPass(i);
		Geometrics::texturedUnitWorldRectOnXZPlane( 0xffffffff );
		pMaterial_->endPass();
	}
	pMaterial_->end();

	Moo::rc().pop();
}


/**
 *	Static python factory method
 */
PyObject * TextureToolView::pyNew( PyObject * args )
{
	BW_GUARD;

	char * textureName;
	if (!PyArg_ParseTuple( args, "|s", &textureName ))
	{
		PyErr_SetString( PyExc_TypeError, "View.TextureToolView: "
			"Argument parsing error: Expected an optional texture name" );
		return NULL;
	}

	if ( textureName != NULL )
		return new TextureToolView( textureName );
	else
		return new TextureToolView( "resources/maps/gizmo/disc.dds" );
}

BW_END_NAMESPACE
// texture_tool_view.cpp
