#ifndef TERRAIN_CHUNK_TEXTURE_TOOL_VIEW_HPP
#define TERRAIN_CHUNK_TEXTURE_TOOL_VIEW_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool.hpp"
#include "worldeditor/editor/texture_tool_view.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a toolview that draws a texture
 *	over each relevant chunk,
 *
 *	This view can scale its material bitmap.
 **/
class TerrainChunkTextureToolView : public TextureToolView
{
	Py_Header( TerrainChunkTextureToolView, TextureToolView )

public:
	explicit TerrainChunkTextureToolView(
		const BW::string &		resourceID	= "resources/maps/gizmo/square.dds",
		PyTypeObject *			pType		= &s_type_ );

	virtual void render( Moo::DrawContext& drawContext, const Tool & tool );

	void numPerChunk( float num );

	PY_RW_ATTRIBUTE_DECLARE( numPerChunk_, numPerChunk )

	PY_FACTORY_DECLARE()

private:
	VIEW_FACTORY_DECLARE( TerrainChunkTextureToolView() )

	float numPerChunk_;
};

BW_END_NAMESPACE

#endif // TERRAIN_CHUNK_TEXTURE_TOOL_VIEW_HPP
