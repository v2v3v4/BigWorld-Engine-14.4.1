#ifndef TERRAIN_TEXTURE_TOOL_VIEW_HPP
#define TERRAIN_TEXTURE_TOOL_VIEW_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool.hpp"
#include "worldeditor/editor/texture_tool_view.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a toolview that draws into a render
 *	target, and projects the render target onto the terrain.
 **/
class TerrainTextureToolView : public TextureToolView
{
	Py_Header( TerrainTextureToolView, TextureToolView )
public:
	TerrainTextureToolView(
		const BW::string& resourceID = "resources/maps/gizmo/disc.dds",
		PyTypeObject * pType = &s_type_ );

	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_RW_ATTRIBUTE_DECLARE( rotation_, rotation )
	PY_RW_ATTRIBUTE_DECLARE( showHoles_, showHoles )

	PY_FACTORY_DECLARE()
private:
	VIEW_FACTORY_DECLARE( TerrainTextureToolView() )

	float rotation_;
	bool  showHoles_;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURE_TOOL_VIEW_HPP
