#ifndef TEXTURE_MASK_CACHE_HPP
#define TEXTURE_MASK_CACHE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "worldeditor/terrain/terrain_paint_brush.hpp"

#include "cstdmf/singleton.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This represents a pixel in a mask image.
 */
#pragma pack(1)
struct MaskPixel
{
	uint8				fuzziness_;	// fuzziness in terms of mask 0 = off, 255 = fully on
	uint8				maxValue_;	// maximum value that a pixel can have if painted using mask
};
#pragma pack()


typedef Moo::Image< MaskPixel > MaskImage;


/**
 *	This is the mask image, plus any other bits of information needed when
 *  using the results of a mask operation.
 */
struct MaskResult
{
	MaskImage			image_;				// the image of MaskPixels.
	BW::vector<bool>	maskTexLayer_;		// which layers do we mask against?
};


/**
 *	This is used to generate mask images which can be used for painting.
 */
class TextureMaskCache : public Singleton< TextureMaskCache >
{
public:

	TextureMaskCache();
	~TextureMaskCache();

	static void beginFuzziness(
		Terrain::EditorBaseTerrainBlockPtr terrain,
		const TerrainPaintBrush & brush,
		BW::vector<bool> &		maskLayers	);

	static MaskPixel
	fuzziness(
		Terrain::EditorBaseTerrainBlockPtr terrain,
		const Vector3 &			terrainPos,
		const Vector3 &			pos,
		const TerrainPaintBrush & brush,
		const BW::vector<bool>	&maskLayers );

	static void endFuzziness(
		Terrain::EditorBaseTerrainBlockPtr terrain,
		const TerrainPaintBrush & brush,
		const BW::vector<bool>	&maskLayers );

	void clear();

	MaskResult *mask( Terrain::EditorBaseTerrainBlockPtr terrain );

	void changedLayers( Terrain::EditorBaseTerrainBlockPtr terrain );

	TerrainPaintBrushPtr paintBrush() const;
	void paintBrush( TerrainPaintBrushPtr brush );

protected:
	typedef BW::map< Terrain::EditorBaseTerrainBlockPtr, MaskResult > CacheMap;

	MaskResult *generate( Terrain::EditorBaseTerrainBlockPtr terrain );

private:
	TerrainPaintBrushPtr	paintBrush_;
	CacheMap				cache_;
};

BW_END_NAMESPACE

#endif // TEXTURE_MASK_CACHE_HPP
