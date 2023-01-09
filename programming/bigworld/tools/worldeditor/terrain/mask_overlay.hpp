#ifndef MASK_OVERLAY_HPP
#define MASK_OVERLAY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "worldeditor/misc/editor_renderable.hpp"

#include "moo/image_texture.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to draw the terrain mask.
 */
class MaskOverlay : public EditorRenderable
{
public:
	MaskOverlay( EditorChunkTerrainPtr pTerrain, 
		const Moo::Image<uint8> & mask );

	/*virtual*/ void render();

private:
	EditorChunkTerrainPtr		pTerrain_;
	Moo::ImageTextureARGBPtr	pTexture_;
};

BW_END_NAMESPACE

#endif // MASK_OVERLAY_HPP
