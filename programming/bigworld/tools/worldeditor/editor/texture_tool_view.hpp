#ifndef TEXTURE_TOOL_VIEW_HPP
#define TEXTURE_TOOL_VIEW_HPP

#include "gizmo/tool_view.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a toolview that draws into a render
 *	target, and uses the render target texture to draw in the world.
 **/
class TextureToolView : public ToolView
{
	Py_Header( TextureToolView, ToolView )
public:
	TextureToolView( const BW::string& resourceID = "resources/maps/gizmo/disc.dds",
					PyTypeObject * pType = &s_type_ );

	virtual void viewResource( const BW::string& resourceID );
	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_FACTORY_DECLARE()
protected:	
	Moo::EffectMaterialPtr		pMaterial_;
	Moo::BaseTexturePtr			pTexture_;

	VIEW_FACTORY_DECLARE( TextureToolView() )
};

BW_END_NAMESPACE

#endif // TEXTURE_TOOL_VIEW_HPP