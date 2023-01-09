#ifndef TERRAIN_PAINT_BRUSH_HPP
#define TERRAIN_PAINT_BRUSH_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "worldeditor/import/import_image.hpp"

#include "math/simplex_noise.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to paint the terrain.  It includes the texture, 
 *	projection and mask options used to paint the terrain.
 */
class TerrainPaintBrush : public PyObjectPlus
{
    Py_Header( TerrainPaintBrush, PyObjectPlus )

public:
    explicit TerrainPaintBrush( PyTypeObject *pType = &s_type_ );

	bool load( DataSectionPtr pDataSection );
	bool save( DataSectionPtr pDataSection ) const;
	void assign( const TerrainPaintBrush& );

	BW::string suggestedFilename() const;

	static BW::string texture( DataSectionPtr pDataSection );
	static BW::string bumpTexture ( const DataSectionPtr& pDataSection );

	BW::string			paintTexture_;			// texture being painted
	BW::string			paintBumpTexture_;		// bump texture being painted
	float				strength_;				// strength of the brush (0 to 100)
	float				size_;					// size of the brush (metres)
	Vector4				paintUProj_;			// u projection of texture being painted
	Vector4				paintVProj_;			// v projection of texture being painted
	uint8				opacity_;				// opacity of the texture being painted
	bool				uvLocked_;				// are u and v sizes locked

	bool				heightMask_;			// height mask enabled?
	float				h1_;					// min. height (where mask = 0)
	float				h2_;					// min. height (where mask = 1)
	float				h3_;					// max. height (where mask = 1)
	float				h4_;					// max. height (where mask = 0)

	bool				slopeMask_;				// slope mask enabled?
	float				s1_;					// min. slope (where mask = 0)
	float				s2_;					// min. slope (where mask = 1)
	float				s3_;					// max. slope (where mask = 1)
	float				s4_;					// max. slope (where mask = 0)

	bool				textureMask_;			// texture mask enabled?
	bool				textureMaskIncludeProj_; // texture mask includes projections?
	bool				textureMaskIncludeBump_; // texture mask includes bump?
	Vector4				textureMaskUProj_;		// texture mask u-projection
	Vector4				textureMaskVProj_;		// texture mask v-projection
	BW::string			textureMaskTexture_;	// texture mask texture name
	BW::string			bumpTextureMaskTexture_; // bump texture mask texture name
	bool				textureMaskInvert_;		// invert texture mask?

    bool                noiseMask_;				// noise mask enabled?
    SimplexNoise        noise_;					// the noise to use in the noise mask
	float				noiseMinSat_;			// min. noise saturation value
	float				noiseMaxSat_;			// max. noise saturation value
	float				noiseMinStrength_;		// min. noise strength value
	float				noiseMaxStrength_;		// max. noise strength value

	bool				importMask_;			// import mask enabled?
	Vector2				importMaskTL_;			// top-left of import mask area
	Vector2				importMaskBR_;			// bottom-right of import mask area
	ImportImagePtr		importMaskImage_;		// import mask image
	float				importMaskAdd_;			// offset for the import mask strength
	float				importMaskMul_;			// multiplier for the import mask strength

	bool				maxLayerLimit_;			// limit the number of layers per chunk?
	uint32				maxLayers_;				// the maximum number of layers that can be in a chunk

	bool operator==( const TerrainPaintBrush & other ) const;
	bool operator!=( const TerrainPaintBrush & other ) const;

	bool isMaskLayer( const Terrain::TerrainTextureLayer & layer ) const;
};


typedef SmartPointer < TerrainPaintBrush > TerrainPaintBrushPtr;
PY_SCRIPT_CONVERTERS_DECLARE( TerrainPaintBrush )

BW_END_NAMESPACE

#endif // TERRAIN_PAINT_BRUSH_HPP
