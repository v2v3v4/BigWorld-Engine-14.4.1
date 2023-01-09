#include "pch.hpp"
#include "terrain_settings.hpp"
#include "resmgr/auto_config.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/watcher.hpp"
#include "manager.hpp"

#ifndef MF_SERVER
#include "terrain2/terrain_renderer2.hpp"
#include "terrain_graphics_options.hpp"
#endif

BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{
	// This function returns the smallest n such that 2^n >= x.
#ifndef MF_SERVER
	uint32 log2RoundUp( uint32 x )
	{
		uint32 result = 0;
		while ((x >> result) > 1)
			result++;
		return result;
	}
#endif // MF_SERVER

	// This helper method reads a int value from a datasection, using
	// a second section for defaults
	int readIntSetting( BW::string sectionName, DataSectionPtr pSection,
		DataSectionPtr pDefaultsSection )
	{
		if (pDefaultsSection)
			return pSection->readInt( sectionName, 
				pDefaultsSection->readInt( sectionName ) );
		return pSection->readInt( sectionName );
	}

#ifndef MF_SERVER
	// This helper method reads a float value from a datasection, using
	// a second section for defaults
	float readFloatSetting( BW::string sectionName, DataSectionPtr pSection,
		DataSectionPtr pDefaultsSection )
	{
		if (pDefaultsSection)
			return pSection->readFloat( sectionName, 
				pDefaultsSection->readFloat( sectionName ) );
		return pSection->readFloat( sectionName );
	}

	// This helper method reads a bool value from a datasection, using
	// a second section for defaults
	bool readBoolSetting( BW::string sectionName, DataSectionPtr pSection,
		DataSectionPtr pDefaultsSection )
	{
		if (pDefaultsSection)
			return pSection->readBool( sectionName, 
				pDefaultsSection->readBool( sectionName ) );
		return pSection->readBool( sectionName );
	}

	// This helper method reads the level distances from a datasection
	// using another datasection for default values
	void readLevelDistances( DataSectionPtr pSection, 
		DataSectionPtr pDefaultsSection, uint32 numTerrainLods,
		BW::vector<float>& distances )
	{
		// clear the distances
		distances.clear();

		// Distance count is numlods - 1 as the last distance does not lod out
		const uint32 numDistances = numTerrainLods - 1;

		// read each distance
		for ( uint32 i = 0; i < numDistances; i++ )
		{
			char buffer[64];
			bw_snprintf( buffer, ARRAY_SIZE(buffer), 
				"lodInfo/lodDistances/distance%u", i );
			distances.push_back( readFloatSetting( buffer, 
				pSection, pDefaultsSection ) );
		}
	}

	// This helper method writes the level distances to a datasection
	void saveLevelDistances( DataSectionPtr pSection, 
		BW::vector<float>& distances )
	{
		// read each distance
		for ( uint32 i = 0; i < distances.size(); i++ )
		{
			char buffer[64];
			bw_snprintf( buffer, ARRAY_SIZE(buffer), 
				"lodDistances/distance%u", i );
			pSection->writeFloat( buffer, distances[i] );
		}
	}
#endif // MF_SERVER

	// The last lod is set to be really far away, Not FLT_MAX as shaders may 
	// have different max limits to regular floats
	const float LAST_LOD_DIST = 1000000.f;
};


/// Terrain 1 has been deprecated
/*static*/ const uint32 TerrainSettings::TERRAIN1_VERSION = 100;
/*static*/ const uint32 TerrainSettings::TERRAIN2_VERSION = 200;


#ifndef MF_SERVER
float  TerrainSettings::s_zoomFactor_ = 1.0f;
float  TerrainSettings::s_invZoomFactor_ = 1.0f;
uint32 TerrainSettings::s_topVertexLod_ = 0;
bool TerrainSettings::s_useLodTexture_ = true;
bool TerrainSettings::s_constantLod_ = false;
bool TerrainSettings::s_doBlockSplit_ = true;
bool TerrainSettings::s_loadDominantTextureMaps_ = true;
#else
bool TerrainSettings::s_loadDominantTextureMaps_ = false;
#endif // MF_SERVER

// Constructor
TerrainSettings::TerrainSettings() :
	blockSize_( 0.f )
{
	BW_GUARD;	
#ifndef MF_SERVER
	static bool firstTime = true;
	if (firstTime)
	{
		firstTime = false;
		MF_WATCH( "Render/Terrain/Terrain2/lodStartLevel", s_topVertexLod_ );

		MF_WATCH( "Render/Terrain/Terrain2/Use lod texture", s_useLodTexture_ );
		MF_WATCH( "Render/Terrain/Terrain2/Do block split", s_doBlockSplit_ );
		MF_WATCH( "Render/Terrain/Terrain2/Do constant LOD", s_constantLod_ );
	}
#endif

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
	serverHeightMapLod_ = 0;
#endif
}

TerrainSettings::~TerrainSettings()
{
	BW_GUARD;	
#ifndef MF_SERVER
	if (Manager::pInstance() != NULL)
		TerrainGraphicsOptions::instance()->delSettings( this );
#endif
}

/**
 *	Initialises the settings according to the data section (usually the terrain
 *  section of the space.settings file) and inits the appropriate terrain
 *  renderer when not in the server.
 *
 *  @param blockSize	The new chunk edge length.
 *  @param settings		DataSection containing the desired terrain settings.
 *  @param oldBlockSize	The old chunk edge length. (if changed from the World Editor)
 *	@return true if successful
 */
bool TerrainSettings::init( float blockSize, DataSectionPtr settings, float oldBlockSize )
{
	BW_GUARD;	
	blockSize_ = blockSize;

#ifndef MF_SERVER
	vertexLod_.setBlockSize( blockSize );
	TerrainGraphicsOptions& graphicsCfg = *TerrainGraphicsOptions::instance();
	graphicsCfg.delSettings( this );
#endif

	bool ret = false;
	if (settings == NULL)
	{
		// no settings, so assume terrain 1
		version_ = TerrainSettings::TERRAIN1_VERSION;
	}
	else
	{
		version_ = settings->readInt( "version", 0 );
#ifndef MF_SERVER
		uvProjections_ = true;
#endif // MF_SERVER
	}

#if !defined( MF_SERVER )
	// Init the values appropriate to the renderer.
	if ((version_ == TerrainSettings::TERRAIN2_VERSION) &&
		TerrainRenderer2::init())
	{
		// Load up version 2 specific terrain data
		const DataSectionPtr pTerrain2Defaults =
			Manager::instance().pTerrain2Defaults();

		pRenderer_ = TerrainRenderer2::instance();

		// Unit settings
		lodMapSize_		= readIntSetting( "lodMapSize",		settings, pTerrain2Defaults );
		aoMapSize_		= readIntSetting( "aoMapSize",		settings, pTerrain2Defaults );
		heightMapSize_ = readIntSetting( "heightMapSize",
			settings, pTerrain2Defaults ); 
		heightMapEditorSize_ = settings->readInt( "heightMapEditorSize", heightMapSize_ );
		normalMapSize_ = readIntSetting( "normalMapSize",
			settings, pTerrain2Defaults ); 
		holeMapSize_ = readIntSetting( "holeMapSize",
			settings, pTerrain2Defaults ); 
		shadowMapSize_ = readIntSetting( "shadowMapSize",
			settings, pTerrain2Defaults ); 
		blendMapSize_ = readIntSetting( "blendMapSize",
			settings, pTerrain2Defaults ); 

		// calculate the number of vertex lods
		numVertexLods_ = log2RoundUp( std::max( heightMapSize_, heightMapEditorSize_ ) );

		MF_ASSERT((1 << numVertexLods_) == std::max( heightMapSize_, heightMapEditorSize_ ) );

		// read the lod distances
		readLevelDistances( settings, pTerrain2Defaults, numVertexLods_,
			savedVertexLodValues_ );

#if EDITOR_ENABLED
		if (oldBlockSize != 0.f)
		{
			updateLodDistances( oldBlockSize );
		}
#endif

		// lod vertex information
		vertexLod_.startBias( readFloatSetting( "lodInfo/startBias",
			settings, pTerrain2Defaults ) ); 
		vertexLod_.endBias( readFloatSetting( "lodInfo/endBias", 
			settings, pTerrain2Defaults ) );

		// read lod texture information
		m_savedLodTextureParams.m_start = readFloatSetting( "lodInfo/lodTextureStart", settings, pTerrain2Defaults ); 
		m_savedLodTextureParams.m_distance = readFloatSetting("lodInfo/lodTextureDistance", settings, pTerrain2Defaults ); 
		m_savedLodTextureParams.m_blendPreloadDistance = readFloatSetting( "lodInfo/blendPreloadDistance", settings, pTerrain2Defaults );

		// normal map lod
		m_savedLodNormalParams.m_start = readFloatSetting( "lodInfo/lodNormalStart", settings, pTerrain2Defaults );
		m_savedLodNormalParams.m_start = std::max(m_savedLodNormalParams.m_start, m_savedLodTextureParams.m_start + m_savedLodTextureParams.m_distance );
		m_savedLodNormalParams.m_distance = readFloatSetting("lodInfo/lodNormalDistance", settings, pTerrain2Defaults ); 
		m_savedLodNormalParams.m_normalPreloadDistance = readFloatSetting( "lodInfo/normalPreloadDistance", settings, pTerrain2Defaults );

		// bump mapping
		m_savedBumpParams.m_fadingStart = readFloatSetting("lodInfo/bumpFadingStart", settings, pTerrain2Defaults ); 
		m_savedBumpParams.m_fadingDistance = readFloatSetting("lodInfo/bumpFadingDistance",	settings, pTerrain2Defaults ); 
		m_savedBumpParams.m_enabled = graphicsCfg.bumpMapping();

		// Height map lods
		defaultHeightMapLod_ = numVertexLods_ - log2RoundUp( std::min( heightMapSize_, heightMapEditorSize_ ) );

		detailHeightMapDistance_ = readFloatSetting(
			"lodInfo/detailHeightMapDistance", settings, pTerrain2Defaults );

		//--
		applyLodModifier(graphicsCfg.lodModifier());
		applyLodTextureModifier(graphicsCfg.lodTextureModifier());
		applyLodNormalModifier(graphicsCfg.lodNormalModifier());
		applyBumpMappingModifier(graphicsCfg.lodBumpModifier());
		ret = true;
	}
	else if (version_ != TerrainSettings::TERRAIN2_VERSION)
	{
		BaseTerrainRenderer::instance( NULL );
		ERROR_MSG( "Terrain renderer version %d is unsupported. "
			"Couldn't find a valid terrain renderer for this space.\n",
			version_ );
	}

	graphicsCfg.addSettings( this );

	directSoundOcclusion_ = settings->readFloat( "soundOcclusion/directOcclusion", 0.0f );
	reverbSoundOcclusion_ = settings->readFloat( "soundOcclusion/reverbOcclusion", 0.0f );

#if defined( EDITOR_ENABLED )
	// Editor specific settings for terrain 2
	if (version_ == TerrainSettings::TERRAIN2_VERSION)
	{
		// Load base height map lod setting
		serverHeightMapLod_ = defaultHeightMapLod_;
	}
#endif // defined( EDITOR_ENABLED )

#else // !defined( MF_SERVER )
	// Server specific settings for terrain 2
	if (version_ == TerrainSettings::TERRAIN2_VERSION)
	{
		// Load up version 2 specific terrain data
		const DataSectionPtr pTerrain2Defaults = Manager::instance().pTerrain2Defaults();

		//Load base height map lod setting
		serverHeightMapLod_ = readIntSetting( "lodInfo/server/heightMapLod",
			settings, Manager::instance().pTerrain2Defaults() );

		ret = true;
	}
	else
	{
		ERROR_MSG( "Terrain version %d is unsupported.", version_ );
	}
#endif // !defined( MF_SERVER )

	return ret;
}

#ifndef MF_SERVER

void TerrainSettings::setActiveRenderer()
{
	BW_GUARD;
	BaseTerrainRenderer::instance( pRenderer_.getObject() );
}

void TerrainSettings::changeZoomFactor(float zoomFactor)
{
	BW_GUARD;
	MF_ASSERT(!almostZero(zoomFactor));

	s_zoomFactor_	 = zoomFactor;
	s_invZoomFactor_ = 1.0f / zoomFactor;
}

BaseTerrainRendererPtr TerrainSettings::pRenderer()
{
	return pRenderer_;
}

void TerrainSettings::applyLodModifier(float modifier)
{
	BW_GUARD;
	vertexLod_.clear();

	// The minimum distance between lod levels is the distance from a blocks
	// corner to where the block starts geomorphing
	const float minDistance = calculateMinDistance();

	// The first lod distance does not matter it can lod out as early 
	// as it wants.
	float lastValue = -minDistance;

	// Iterate over our lod values and scale them by the lod modifier.
	for (uint32 i = 0; i < savedVertexLodValues_.size(); i++)
	{
		if (i <= topVertexLod())
		{
			lastValue = -minDistance;
		}
		// Make sure our lod level delta is big enough
		float value = floorf(savedVertexLodValues_[i] * modifier);
		if ((value - lastValue) < minDistance)
			value = lastValue + minDistance;

		vertexLod_.push_back( value );
		lastValue = value;
	}
}

void TerrainSettings::applyLodTextureModifier(float modifier)
{
	BW_GUARD;

	m_lodTextureParams.m_start					= m_savedLodTextureParams.m_start * modifier;
	m_lodTextureParams.m_distance				= m_savedLodTextureParams.m_distance * modifier;
	m_lodTextureParams.m_blendPreloadDistance	= m_savedLodTextureParams.m_blendPreloadDistance * modifier;
}

void TerrainSettings::applyLodNormalModifier(float modifier)
{
	BW_GUARD;

	m_lodNormalParams.m_start					= m_savedLodNormalParams.m_start * modifier;
	m_lodNormalParams.m_start					= max(m_lodNormalParams.m_start, m_lodTextureParams.m_start + m_lodTextureParams.m_distance);
	m_lodNormalParams.m_distance				= m_savedLodNormalParams.m_distance * modifier;
	m_lodNormalParams.m_normalPreloadDistance	= m_savedLodNormalParams.m_normalPreloadDistance * modifier;
}

void TerrainSettings::applyBumpMappingModifier(float modifier)
{
	BW_GUARD;

	m_bumpParams.m_fadingStart					= m_savedBumpParams.m_fadingStart * modifier;
	m_bumpParams.m_fadingDistance				= m_savedBumpParams.m_fadingDistance * modifier;
	m_bumpParams.m_enabled						= m_savedBumpParams.m_enabled;
}

float TerrainSettings::calculateMinDistance() const
{
	return ceilf( sqrtf( blockSize_ * blockSize_ * 2.f ) / vertexLod_.startBias() );
}

#endif

#ifdef EDITOR_ENABLED

namespace
{
	static TerrainSettingsPtr s_pDefaults;
}

/**
 *	Defaults getter
 *
 *  @returns		The TerrainSettings instance.
 */
/*static*/ TerrainSettingsPtr TerrainSettings::defaults()
{
	BW_GUARD;
	if (!s_pDefaults.exists())
	{
		s_pDefaults = new TerrainSettings;
		s_pDefaults->initDefaults(0);
	}
	return s_pDefaults;
}

void TerrainSettings::initDefaults( float blockSize, float oldBlockSize )
{
	BW_GUARD;
	this->init( blockSize, Manager::instance().pTerrain2Defaults(), oldBlockSize );
}

void TerrainSettings::save(DataSectionPtr pSection)
{
	BW_GUARD;
	MF_ASSERT( pSection );
	pSection->delChildren();

	pSection->writeInt( "version", version_ );

	// Only write out other settings for terrain2
	if (version_ == TerrainSettings::TERRAIN2_VERSION)
	{
		// Write unit setup
		pSection->writeInt( "lodMapSize", lodMapSize_ );
		pSection->writeInt( "aoMapSize", aoMapSize_ );

		if ( heightMapEditorSize_ != heightMapSize_ )
		{
			pSection->writeInt( "heightMapEditorSize", heightMapEditorSize_ );
		}

		pSection->writeInt( "heightMapSize", heightMapSize_ );
		pSection->writeInt( "normalMapSize", normalMapSize_ );
		pSection->writeInt( "holeMapSize", holeMapSize_ );
		pSection->writeInt( "shadowMapSize", shadowMapSize_ );
		pSection->writeInt( "blendMapSize", blendMapSize_ );

		// Write lod information
		DataSectionPtr pLodSection = pSection->newSection( "lodInfo" );
		if (pLodSection)
		{
			pLodSection->writeFloat( "startBias", vertexLod_.startBias() );
			pLodSection->writeFloat( "endBias", vertexLod_.endBias() );
			pLodSection->writeFloat( "lodTextureStart", m_savedLodTextureParams.m_start );
			pLodSection->writeFloat( "lodTextureDistance", m_savedLodTextureParams.m_distance );
			pLodSection->writeFloat( "blendPreloadDistance", m_savedLodTextureParams.m_blendPreloadDistance );
			pLodSection->writeFloat( "bumpFadingStart", m_savedBumpParams.m_fadingStart );
			pLodSection->writeFloat( "bumpFadingDistance", m_savedBumpParams.m_fadingDistance );

			// normal map lod
			pLodSection->writeFloat( "lodNormalStart", m_savedLodNormalParams.m_start );
			pLodSection->writeFloat( "lodNormalDistance", m_savedLodNormalParams.m_distance );
			pLodSection->writeFloat( "normalPreloadDistance", m_savedLodNormalParams.m_normalPreloadDistance );
			pLodSection->writeFloat("detailHeightMapDistance", detailHeightMapDistance_ );

			saveLevelDistances( pLodSection, savedVertexLodValues_ );

			// TODO: It would be good to save serverHeightMapLod_ too.
			pLodSection->writeInt( "server/heightMapLod", serverHeightMapLod_ );
		}

	}

	pSection->writeFloat( "soundOcclusion/directOcclusion", directSoundOcclusion_ );
	pSection->writeFloat( "soundOcclusion/reverbOcclusion", reverbSoundOcclusion_ );
}

void TerrainSettings::version(uint32 value)
{
	version_ = value;
}

void TerrainSettings::lodMapSize(uint32 value)
{
	lodMapSize_ = value;
}

void TerrainSettings::aoMapSize(uint32 value)
{
	aoMapSize_ = value;
}

void TerrainSettings::heightMapSize(uint32 value)
{
	heightMapSize_ = value;

	// recalculate the number of vertex lods
	numVertexLods_ = log2RoundUp( std::max( heightMapSize_, heightMapEditorSize_ ) );
	updateLodDistances( blockSize_ );
}

void TerrainSettings::heightMapEditorSize(uint32 value)
{
	heightMapEditorSize_ = value;
	
	// recalculate the number of vertex lods
	numVertexLods_ = log2RoundUp( std::max( heightMapSize_, heightMapEditorSize_ ) );
	updateLodDistances( blockSize_ );
}

void TerrainSettings::normalMapSize(uint32 value)
{
	normalMapSize_ = value;
}

void TerrainSettings::holeMapSize(uint32 value)
{
	holeMapSize_ = value;
}

void TerrainSettings::shadowMapSize(uint32 value)
{
	shadowMapSize_ = value;
}

void TerrainSettings::blendMapSize(uint32 value)
{
	blendMapSize_ = value;
}

void TerrainSettings::fini()
{
	s_pDefaults = NULL;
}

float TerrainSettings::vertexLodDistance( uint32 lod ) const
{
	if (lod < savedVertexLodValues_.size())
	{
		return savedVertexLodValues_[lod];
	}
	return LAST_LOD_DIST;
}

void TerrainSettings::vertexLodDistance( uint32 lod, float value )
{
	if (lod < savedVertexLodValues_.size())
	{
		savedVertexLodValues_[lod] = value;
		applyLodModifier(TerrainGraphicsOptions::instance()->lodModifier());
	}
}

float TerrainSettings::getMinDistance() const
{
	return calculateMinDistance();
}

/**
 *	Updates the savedVertexLodValues_ collection with lod distances,
 *	based on the height map resolution. It multiplies the original
 *	values by a ratio of (new block size) / (old block size).
 *	The old block size will be the default for newly added rows,
 *	as the default values were calculated for the default size.
 *
 *  @param oldBlockSize	Chunk edge length to convert from.
 */
void TerrainSettings::updateLodDistances( float oldBlockSize )
{
	if (oldBlockSize == 0.f)
	{
		oldBlockSize = DEFAULT_BLOCK_SIZE;
	}

	float newBlockSize = blockSize_ > 0.f ? blockSize_ : DEFAULT_BLOCK_SIZE;
	float ratio = newBlockSize / oldBlockSize;
	uint32 oldSavedLods = uint32( savedVertexLodValues_.size() );
	uint32 savedLods =
		std::min( numVertexLods_ - 1, oldSavedLods );

	for (uint32 i = 0; i < savedLods; ++i)
	{
		savedVertexLodValues_[i] *= ratio;
	}

	savedLods = numVertexLods_ - 1;
	savedVertexLodValues_.resize( savedLods );

	if (oldSavedLods < savedLods)
	{
		DataSectionPtr defaults = Manager::instance().pTerrain2Defaults();
		ratio = newBlockSize / DEFAULT_BLOCK_SIZE;

		for (uint32 i = oldSavedLods; i < savedLods; ++i)
		{
			char buffer[64];
			bw_snprintf( buffer, ARRAY_SIZE(buffer), 
				"lodInfo/lodDistances/distance%u", i );
			
			float lodValue = readFloatSetting( buffer, defaults, defaults );
			lodValue *= ratio;
			savedVertexLodValues_[i] = lodValue;
		}
	}
}

#endif

/**
 *	Current terrain version getter.
 *
 *  @returns		Current terrain version.
 */
uint32 TerrainSettings::version() const
{
	return version_;
}

/**
 *	Length of the edges of the blocks
 *
 *  @returns		Block edge length
 */
float TerrainSettings::blockSize() const
{
	return blockSize_;
}

/**
 *	Returns whether loading dominant texture maps is enabled or not.
 *
 *  @returns		True if loading dominant texture maps is enabled, false otherwise.
 */
bool TerrainSettings::loadDominantTextureMaps()
{
	return s_loadDominantTextureMaps_;
}

/**
 *	Sets whether dominant texture maps should be loaded or not.
 *
 *  @param doLoad		True to enable loading dominant texture maps, false to disable.
 */
void TerrainSettings::loadDominantTextureMaps( bool doLoad )
{
	s_loadDominantTextureMaps_ = doLoad;
}

#ifndef MF_SERVER

/**	
 *	Get the vertex lod start bias
 *	@return the bias
 */
float TerrainSettings::vertexLodStartBias() const
{
	return vertexLod_.startBias();
}

/**	
 *	Set the vertex lod start bias
 *	@param value the start bias
 */
void TerrainSettings::vertexLodStartBias( float value )
{
	vertexLod_.startBias( value );
}

/**	
 *	Get the vertex lod end bias
 *	@return the bias
 */
float TerrainSettings::vertexLodEndBias() const
{
	return vertexLod_.endBias();
}

/**	
 *	Set the vertex lod end bias
 *	@param value the end bias
 */
void TerrainSettings::vertexLodEndBias( float value )
{
	vertexLod_.endBias( value );
}

/**
 *	Get the top vertex lod, i.e. the first lod level to consider
 *	@return the top vertex lod
 */
uint32 TerrainSettings::topVertexLod() const
{
#ifdef EDITOR_ENABLED
	return s_topVertexLod_;
#else 
	return ((heightMapSize_ > MINIMUM_TERRAIN_HEIGHT_MAP) ? s_topVertexLod_ : 0) + defaultHeightMapLod_;
#endif // EDITOR_ENABLED
}

void TerrainSettings::topVertexLod(uint32 state)
{
	//This setting does not support any other value than 0 or 1. 
	// If you look in the code in the getter, the check to make sure the 
	// heightmap size is correct, only makes sense if this value can only 
	// be 0 or 1, and currently that is all that is possible to be set. 
	// If more values are needed, then adjust the logic in the getter to 
	// be more robust.
	MF_ASSERT( state <= 1 );

	s_topVertexLod_ = state;
}

/**
 *	Set the distance to where the lod texture blends in
 *	@param value distance to where the lod texture blends in
 */
void TerrainSettings::lodTextureStart( float value )
{
	m_savedLodTextureParams.m_start = value;
	applyLodTextureModifier(TerrainGraphicsOptions::instance()->lodTextureModifier());
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

/**
 *	Set the distance over which the lod texture is blended in
 *	This value is relative to the lod texture start
 *	@param value the distance over which the lod texture is blended in
 */
void TerrainSettings::lodTextureDistance( float value )
{
	m_savedLodTextureParams.m_distance = value;
	applyLodTextureModifier(TerrainGraphicsOptions::instance()->lodTextureModifier());
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

/**
 *	Set the distance at which we preload blends
 *	This distance is relative to the lod texture start and lod texture distance
 *	@param value the distance at which we preload blends
 */
void TerrainSettings::blendPreloadDistance( float value )
{
	m_savedLodTextureParams.m_blendPreloadDistance = value;
	applyLodTextureModifier(TerrainGraphicsOptions::instance()->lodTextureModifier());
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

/**
 *	Get whether we want to use lod texture or not
 *	@return true if we want to use the lod texture
 */
bool TerrainSettings::useLodTexture()
{
	return s_useLodTexture_;
}

void TerrainSettings::useLodTexture(bool state)
{
	s_useLodTexture_ = state;
}

/**
 *	Get whether we want to use a constant lod or not
 *	@return true if we want to use constant lod
 */
bool TerrainSettings::constantLod()
{
	return s_constantLod_;
}

void TerrainSettings::constantLod(bool state)
{
	s_constantLod_ = state;
}


bool TerrainSettings::doBlockSplit()
{
	return s_doBlockSplit_;
}

void TerrainSettings::lodNormalStart( float value )
{
	m_savedLodNormalParams.m_start = std::max(value, m_savedLodTextureParams.m_start + m_savedLodTextureParams.m_distance);
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

void TerrainSettings::lodNormalDistance( float value )
{
	m_savedLodNormalParams.m_distance = value;
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

void TerrainSettings::normalPreloadDistance( float value )
{
	m_savedLodNormalParams.m_normalPreloadDistance = value;
	applyLodNormalModifier(TerrainGraphicsOptions::instance()->lodNormalModifier());
}

void TerrainSettings::bumpMapping(bool value)
{
	m_savedBumpParams.m_enabled = value;
	applyBumpMappingModifier(TerrainGraphicsOptions::instance()->lodBumpModifier());
}

void TerrainSettings::bumpFadingStart(float value)
{
	m_savedBumpParams.m_fadingStart = value;
	applyBumpMappingModifier(TerrainGraphicsOptions::instance()->lodBumpModifier());
}

void TerrainSettings::bumpFadingDistance(float value)
{
	m_savedBumpParams.m_fadingDistance = value;
	applyBumpMappingModifier(TerrainGraphicsOptions::instance()->lodBumpModifier());
}

float TerrainSettings::directSoundOcclusion() const
{
	return directSoundOcclusion_;
}

void TerrainSettings::directSoundOcclusion( float value )
{
	directSoundOcclusion_ = value;
}

float TerrainSettings::reverbSoundOcclusion() const
{
	return reverbSoundOcclusion_;
}

void TerrainSettings::reverbSoundOcclusion( float value )
{
	reverbSoundOcclusion_ = value;
}

#endif // EDITOR_ENABLED

BW_END_NAMESPACE

// terrain_settings.cpp
