#ifndef MAX_WARNING_OVERLAY_HPP
#define MAX_WARNING_OVERLAY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/misc/editor_renderable.hpp"

BW_BEGIN_NAMESPACE

class MaxLayersOverlay : public EditorRenderable
{
public:
	explicit MaxLayersOverlay( EditorChunkTerrain * pTerrain );

	void render();

private:
	Moo::BaseTexturePtr pTexture_;
	EditorChunkTerrain* pTerrain_;
};


typedef SmartPointer<MaxLayersOverlay> MaxLayersOverlayPtr;


BW_END_NAMESPACE

#endif // MAX_WARNING_OVERLAY_HPP
