#ifndef FORCED_LOD_OVERLAY_HPP
#define FORCED_LOD_OVERLAY_HPP

#include "cstdmf/bw_vector.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/misc/editor_renderable.hpp"

BW_BEGIN_NAMESPACE

class ForcedLodOverlay : public EditorRenderable
{
public:
	explicit ForcedLodOverlay( EditorChunkTerrain * terrain );

	void render();

private:
	BW::vector<Moo::BaseTexturePtr> textures_;
	EditorChunkTerrain* terrain_;
};

typedef SmartPointer<ForcedLodOverlay> ForcedLodOverlayPtr;

BW_END_NAMESPACE

#endif // FORCED_LOD_OVERLAY_HPP
