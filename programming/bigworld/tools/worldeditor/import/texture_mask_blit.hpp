#ifndef TEXTURE_MASK_BLIT_HPP
#define TEXTURE_MASK_BLIT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

namespace TextureMaskBlit
{
	size_t numChunksAffected( TerrainPaintBrushPtr pBrush );

    bool saveUndoForImport(
        TerrainPaintBrushPtr pBrush,
        const BW::string & barrierDesc,
		const BW::string & progressDesc,
        bool                showProgress );

	bool import(
		TerrainPaintBrushPtr pBrush,
		const BW::string &	texture,
		const BW::string &	bumpTexture,
		const Vector4 &		uProj,
		const Vector4 &		vProj,
		bool				showProgress,
		const BW::string &	progDesc,
		bool				forceSave );
}

BW_END_NAMESPACE

#endif // TEXTURE_MASK_BLIT_HPP
