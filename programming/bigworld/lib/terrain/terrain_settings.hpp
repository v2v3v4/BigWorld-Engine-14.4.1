#ifndef TERRAIN_TERRAIN_SETTINGS_HPP
#define TERRAIN_TERRAIN_SETTINGS_HPP

#include "resmgr/datasection.hpp"

#ifndef MF_SERVER
#include "terrain2/terrain_vertex_lod.hpp"
#endif

BW_BEGIN_NAMESPACE

//namespace Moo
//{
//	class GraphicsSetting;
//}

namespace Terrain
{

class BaseTerrainRenderer;
typedef SmartPointer<BaseTerrainRenderer> BaseTerrainRendererPtr;

#ifdef EDITOR_ENABLED
static const float DEFAULT_BLOCK_SIZE = 100.f;
#endif
/**
 *  This class keeps the current terrain settings, usually read from the file
 *  space.settings, keeps track of other runtime settings, and inits the
 *  appropriate terrain render in the 'init' method.
 */
class TerrainSettings : public SafeReferenceCount
{
public:
	static const uint32 TERRAIN1_VERSION;
	static const uint32 TERRAIN2_VERSION;

	TerrainSettings();
	~TerrainSettings();

	// called to set the settings (and initialise the renderer in the
	// client/tools)
	bool init( float blockSize, DataSectionPtr settings, float oldBlockSize = 0.f );

	// terrain space.settings methods
	uint32	version() const;

	float blockSize() const;

	// other terrain-related settings
	static bool	loadDominantTextureMaps();
	static void	loadDominantTextureMaps( bool doLoad );

#ifndef MF_SERVER
	uint32	lodMapSize() const						{ return lodMapSize_; }
	uint32  aoMapSize() const						{ return aoMapSize_; } 
	uint32	heightMapSize() const					{ return heightMapSize_; }
	uint32  heightMapEditorSize() const				{ return heightMapEditorSize_; }
	uint32	normalMapSize() const					{ return normalMapSize_; }
	uint32	holeMapSize() const						{ return holeMapSize_; }
	uint32	shadowMapSize() const					{ return shadowMapSize_; }
	uint32	blendMapSize() const					{ return blendMapSize_; }
	bool	uvProjections() const					{ return uvProjections_; }
	uint32	numVertexLods() const					{ return numVertexLods_; }

	float	vertexLodStartBias() const;
	void	vertexLodStartBias( float value );
    float	vertexLodEndBias() const;
    void	vertexLodEndBias( float value );

	float	lodTextureStart() const					{ return m_lodTextureParams.m_start * s_invZoomFactor_; }
	void	lodTextureStart( float value );
	float	lodTextureDistance() const				{ return m_lodTextureParams.m_distance; }
	void	lodTextureDistance( float value );
	float	blendPreloadDistance() const			{ return m_lodTextureParams.m_blendPreloadDistance * s_invZoomFactor_; }
	void	blendPreloadDistance( float value );

	float	lodNormalStart() const					{ return m_lodNormalParams.m_start * s_invZoomFactor_; }
	void	lodNormalStart( float value );
	float	lodNormalDistance() const				{ return m_lodNormalParams.m_distance; }
	void	lodNormalDistance( float value );
	float	normalPreloadDistance() const			{ return m_lodNormalParams.m_normalPreloadDistance * s_invZoomFactor_; }
	void	normalPreloadDistance( float value );

	bool	bumpMapping() const						{ return m_bumpParams.m_enabled; }
	void	bumpMapping(bool value);
	float	bumpFadingStart() const					{ return m_bumpParams.m_fadingStart * s_invZoomFactor_; }
	void	bumpFadingStart(float value);
	float	bumpFadingDistance() const				{ return m_bumpParams.m_fadingDistance; }
	void	bumpFadingDistance(float value);

	inline float 	absoluteBlendPreloadDistance() const;
	inline float	absoluteNormalPreloadDistance() const;

	inline uint32	defaultHeightMapLod() const;
	inline void		defaultHeightMapLod( uint32 value );

	inline float	detailHeightMapDistance() const;

	const TerrainVertexLod& vertexLod() const { return vertexLod_; }

	float	directSoundOcclusion() const;
	void	directSoundOcclusion( float value );
	float	reverbSoundOcclusion() const;
	void	reverbSoundOcclusion( float value );

	void	setActiveRenderer();

	BaseTerrainRendererPtr pRenderer();

	uint32	topVertexLod() const;

	// settings that are applied globally
	static void		topVertexLod(uint32 level);

	static bool useLodTexture();
	static void useLodTexture( bool state );

	static bool constantLod();
	static void constantLod( bool state );

	static bool doBlockSplit();

	static void	changeZoomFactor(float zoomFactor);

	void	applyLodModifier(float modifier);
	void	applyLodTextureModifier(float modifier);
	void	applyLodNormalModifier(float modifier);
	void	applyBumpMappingModifier(float modifier);

#endif // !MF_SERVER

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
	uint32 serverHeightMapLod() const	{ return serverHeightMapLod_; }
	void   serverHeightMapLod( uint32 serverHeightMapLod ) { serverHeightMapLod_ = serverHeightMapLod; }

#endif // MF_SERVER || EDITOR_ENABLED

#ifdef EDITOR_ENABLED
	// Save the settings
	void	save( DataSectionPtr pSpaceTerrainSection );
	void	initDefaults( float blockSize, float oldBlockSize = 0.f );

	uint32  heightMapGameSize() const { return heightMapSize_; }

	void	version(uint32 value);
	void	lodMapSize(uint32 value);
	void	aoMapSize(uint32 value);
	void    heightMapEditorSize( uint32 value );
	void	heightMapSize(uint32 value);
	void	normalMapSize(uint32 value);
	void	holeMapSize(uint32 value);
	void	shadowMapSize(uint32 value);
	void	blendMapSize(uint32 value);

	static  void fini();

	static SmartPointer<TerrainSettings> defaults();

	float	vertexLodDistance( uint32 lod ) const;
	void	vertexLodDistance( uint32 lod, float value );
	float	getMinDistance() const;
	void	updateLodDistances( float oldBlockSize );
#endif

private:
	// terrain space.settings values
	uint32 version_;
	float blockSize_;

	// other terrain settings
	static bool s_loadDominantTextureMaps_;
#ifndef MF_SERVER

	float calculateMinDistance() const;

	static float s_zoomFactor_;
	static float s_invZoomFactor_;

	uint32 lodMapSize_;
	uint32 aoMapSize_;
	uint32 heightMapEditorSize_;
	uint32 heightMapSize_;
	uint32 normalMapSize_;
	uint32 holeMapSize_;
	uint32 shadowMapSize_;
	uint32 blendMapSize_;

	TerrainVertexLod	vertexLod_;
	BW::vector<float>	savedVertexLodValues_;

	bool uvProjections_;

	uint32 numVertexLods_;

	//--
	struct BumpParams
	{
		BumpParams() : m_enabled(false), m_fadingStart(0), m_fadingDistance(0) { }

		bool  m_enabled;
		float m_fadingStart;
		float m_fadingDistance;
	};

	//--
	struct LodTextureParams
	{
		LodTextureParams() : m_start(0), m_distance(0), m_blendPreloadDistance(0) { }

		float m_start;					//-- lod texture start draw
		float m_distance;				//-- lod texture distance till 100% blended
		float m_blendPreloadDistance;	//-- extra buffer to load blends before they are rendered.
	};

	//--
	struct LodNormalParams
	{
		float m_start;
		float m_distance;
		float m_normalPreloadDistance;
	};

	BumpParams			m_bumpParams;
	BumpParams			m_savedBumpParams;

	LodTextureParams	m_lodTextureParams;
	LodTextureParams	m_savedLodTextureParams;

	LodNormalParams		m_lodNormalParams;
	LodNormalParams		m_savedLodNormalParams;

	// The first lod level to render
    static uint32	s_topVertexLod_;

	// Enable or disable lod texture feature
	static bool s_useLodTexture_;

	// Enable to have every block at the same LOD.
	static bool s_constantLod_;

	// Do vertex lod on half terrain blocks
	static bool s_doBlockSplit_;

	// Lod of the default height map that gets loaded with every block.
	uint32	defaultHeightMapLod_;

	// Detail height map will be loaded when less than this distance from camera.
	float detailHeightMapDistance_;

	// sound properties
	float	directSoundOcclusion_;
	float	reverbSoundOcclusion_;

	BaseTerrainRendererPtr pRenderer_;
#endif // !MF_SERVER

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
	// The height map load that the server should load.
	uint32 serverHeightMapLod_;
#endif // MF_SERVER || EDITOR_ENABLED
};

#ifndef MF_SERVER
/**
*	Get the default height map lod. 
*/
uint32 TerrainSettings::defaultHeightMapLod() const
{
	return defaultHeightMapLod_;
}

/**
*	Set the default height map lod. 
*	@param	value	The new value of default height map LOD.
*/
void TerrainSettings::defaultHeightMapLod( uint32 value )
{
	defaultHeightMapLod_ = value;
}

/**
* Detail height map will be loaded when less than this distance from camera.
*/
float TerrainSettings::detailHeightMapDistance() const
{
	return detailHeightMapDistance_;
}

/**
 * Returns the absolute (total) blend pre load distance
 */
inline float TerrainSettings::absoluteBlendPreloadDistance() const
{
	return lodTextureStart() + lodTextureDistance() + blendPreloadDistance();
}

/**
 *	Returns the absolute (total) normal preload distance
 */

inline float TerrainSettings::absoluteNormalPreloadDistance() const
{
	return lodNormalStart() + lodNormalDistance() + normalPreloadDistance();
}

#endif

} // namespace Terrain

BW_END_NAMESPACE

#endif //  TERRAIN_TERRAIN_SETTINGS_HPP
