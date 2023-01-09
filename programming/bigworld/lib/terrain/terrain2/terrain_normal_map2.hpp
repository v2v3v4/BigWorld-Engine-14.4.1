#ifndef TERRAIN_NORMAL_MAP2_HPP
#define TERRAIN_NORMAL_MAP2_HPP

#include "../terrain_data.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "terrain_blends.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

typedef SmartPointer<class TerrainHeightMap2> TerrainHeightMap2Ptr;

/**
 *	This class implements the normal map resource for the terrain block 2
 *	It handles creation of normal maps and also streaming of hight quality
 *	maps when needed.
 */
class TerrainNormalMap2 : public SafeReferenceCount
{
public:

	TerrainNormalMap2();
	~TerrainNormalMap2();

	bool init( const BW::string& terrainResource, DataSectionPtr lodSection );

#ifdef EDITOR_ENABLED

	bool save( DataSectionPtr pTerrainSection );

	bool generate( const TerrainHeightMap2Ptr	heightMap,
		NormalMapQuality quality, uint32 size );
#endif

	inline void evaluate( uint8 renderTextureMask );

	bool isLoading() const
	{
		return pTask_ != NULL;
	}

	void stream();

	/**
	 * Get the highest detail available normal map
	 */
	ComObjectWrap<DX::Texture> pMap() const 
	{ 
		return pNormalMap_.hasComObject() ? pNormalMap_ : pLodNormalMap_;
	}

	/**
	 *  Get the lod normal map
	 */
	ComObjectWrap<DX::Texture> pLodMap() const 
	{
		return pLodNormalMap_;
	}


	/**
	 * Get the size of the highest detail available normal map
	 */
	uint32 size() const 
	{ 
		return pNormalMap_.hasComObject() ? size_ : lodSize_; 
	}

	/*
	 *	Get the lod normal map size
	 */
	uint32 lodSize() const 
	{ 
		return lodSize_; 
	}

	bool needsQualityMap() const { return wantQualityMap_; }

	static const BW::string LOD_NORMALS_SECTION_NAME;
	static const BW::string NORMALS_SECTION_NAME;

private:
	/**
	 *	This class is the background taks that streams the normal map
	 */
	class NormalMapTask : public BackgroundTask
	{
	public:
		NormalMapTask( SmartPointer<TerrainNormalMap2> pMap,
			const BW::string & terrainResource );
		~NormalMapTask();
		void doBackgroundTask( TaskManager & mgr );
		void doMainThreadTask( TaskManager & mgr );
	private:
		SmartPointer<TerrainNormalMap2> pMap_;
		BW::string					terrainResource_;
		ComObjectWrap<DX::Texture>  pNormalMap_;
		uint32						size_;
	};

	static ComObjectWrap<DX::Texture> loadMap( DataSectionPtr pMapSection, 
		uint32& size );

#ifdef EDITOR_ENABLED
	static ComObjectWrap<DX::Texture> generateMap( 
		const TerrainHeightMap2Ptr	heightMap, 
		NormalMapQuality quality, uint32 size );
	static bool saveMap( DataSectionPtr pSection, 
		ComObjectWrap<DX::Texture> pMap );
#endif

	ComObjectWrap<DX::Texture>  pNormalMap_;
	ComObjectWrap<DX::Texture>  pLodNormalMap_;
	uint32					    size_;
	uint32						lodSize_;

	bool						wantQualityMap_;

	BW::string					terrainResource_;
	NormalMapTask*				pTask_;
};

void TerrainNormalMap2::evaluate( uint8 renderTextureMask )
{
	wantQualityMap_ = false;
	if (renderTextureMask & TerrainRenderer2::RTM_PreloadNormals)
		wantQualityMap_ = true;
}

typedef SmartPointer<TerrainNormalMap2> TerrainNormalMap2Ptr;

} // namespace Terrain

BW_END_NAMESPACE


#endif // TERRAIN_NORMAL_MAP2_HPP
