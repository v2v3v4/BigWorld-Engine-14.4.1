

#include "packers.hpp"
#include "packer_helper.hpp"
#include "resmgr/bwresource.hpp"
#include "cdata_packer.hpp"
#include "cstdmf/bw_util.hpp"

#ifndef MF_SERVER
#include "chunk/chunk.hpp"
#include "chunk/geometry_mapping.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/manager.hpp"
#include "chunk/chunk_item.hpp"
#include "terrain/terrain2/terrain_normal_map2.hpp"
#endif

#include "resmgr/auto_config.cpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

IMPLEMENT_PACKER( CDataPacker )

int CDataPacker_token = 0;

#ifndef MF_SERVER
using namespace Terrain;
#endif

namespace // anonymous
{
	typedef BW::vector< BW::string > SectionsToStrip;
	SectionsToStrip s_sectionsToStrip;

#ifndef MF_SERVER

	static const char * CDATA_PATH_EXT = ".cdata";

	/**
	 *    This class is used to hijack the loading of ChunkTerrain objects
	 */ 
	class DummyChunkTerrain : public ChunkItem
	{
		DECLARE_CHUNK_ITEM( DummyChunkTerrain )

	public:
		bool load( DataSectionPtr pSection, BW::string* errorString )
		{
			BW::string resName = pSection->readString( "resource" );	

			size_t cDataPathEnd = resName.rfind( CDATA_PATH_EXT );
			if (cDataPathEnd == string::npos)
			{
				return true;
			}

			cDataPathEnd += strlen( CDATA_PATH_EXT );
			BW::string cDataPath = resName.substr( 0, cDataPathEnd );
			size_t findPos = s_terrainChunkCDataFilename.find( cDataPath );
			if (findPos == string::npos)
			{
				return true;
			}
			resName = s_terrainChunkCDataFilename + resName.substr( cDataPathEnd );
			MF_ASSERT(s_terrainBlock == NULL);
			s_terrainBlock = BaseTerrainBlock::loadBlock( 
				resName,
				Matrix::identity,
				Vector3::zero(),
				s_terrainSettings,
				errorString );
			return true;
		}


		static void registerFactory()
		{
			Chunk::registerFactory( "terrain", DummyChunkTerrain::factory_ );
		}

		static BW::string s_terrainChunkCDataFilename;
		static BaseTerrainBlockPtr s_terrainBlock;
		static TerrainSettingsPtr s_terrainSettings;
	};

	TerrainSettingsPtr DummyChunkTerrain::s_terrainSettings = NULL;
	BaseTerrainBlockPtr DummyChunkTerrain::s_terrainBlock = NULL;
	BW::string DummyChunkTerrain::s_terrainChunkCDataFilename;

	/**
	 *    RAII class to handle setting up / cleaning up state
	 *    for loading terrain files.
	 */ 
	class InitTerrainDependencies
	{
	public:
		InitTerrainDependencies( DataSectionPtr terrainSettingsData )
		{
			Terrain::ResourceBase::defaultStreamType( RST_Syncronous );
			AutoConfig::configureAllFrom( AutoConfig::s_resourcesXML );
			DummyChunkTerrain::s_terrainSettings = new TerrainSettings();
			DummyChunkTerrain::s_terrainBlock = NULL;
			BaseTerrainBlock::s_disableStreaming_ = true;

			float gridSize = GeometryMapping::getGridSize(
				terrainSettingsData );

			DataSectionPtr terrainSettings =
				ChunkSpace::getTerrainSettingsDataSection(
				terrainSettingsData );

			DummyChunkTerrain::s_terrainSettings->init(
				gridSize, terrainSettings );
			backupFactories_ = *Chunk::getFactories();
			Chunk::clearFactories();
			DummyChunkTerrain::registerFactory();
		}


		~InitTerrainDependencies()
		{
			DummyChunkTerrain::s_terrainBlock = NULL;
			DummyChunkTerrain::s_terrainSettings = NULL;
			BaseTerrainBlock::s_disableStreaming_ = false;
			Chunk::clearFactories();
			Chunk::setFactories( backupFactories_ );

			/*
			// mslater : work around a bug that exists in BgTaskManager::numBgTasksLeft. This has a race condition
			//           between tasks being added to the active list and removed from the task list that makes 
			//			 the loop in the prepare function below exit early and the function to exit before all
			//           background tasks are complete. This causes an assertion in Terrain::Manager as it has been
			//           destroyed by this object on destruction. Below preserves existing behaviour until a correct 
			//           fix can be introduced.
			BgTaskManager::instance().stopAll();
			BgTaskManager::instance().startThreads("Background Thread: res_packer", 1);
			*/
		}

	private:
		Chunk::Factories backupFactories_;
		Manager terrainManager_;
	};
#endif //MF_SERVER
}

CDataPacker::CDataPacker() :
	stripWorldNavMesh_( true ),
	stripLayers_( false ),
	stripLodTextures_( false ),
	stripQualityNormals_( false ),
	stripLodNormals_( false ),
	heightMapLodToPreserve_( -1 )
{

}

void CDataPacker::addStripSection( const char * sectionName )
{
	s_sectionsToStrip.push_back( sectionName );
}

#ifndef MF_SERVER

#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, &errorString )
IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD( DummyChunkTerrain, terrain, 0, false )

#endif

bool CDataPacker::prepare( const BW::string& src, const BW::string& dst )
{
	BW::StringRef ext = BWResource::getExtension( src );
	if (!ext.equals_lower( "cdata" ))
		return false;

	src_ = src;
	dst_ = dst;

	BW::string tempPath = BWUtil::getFilePath( src );
	MF_ASSERT( *tempPath.rbegin() == '/' );

	DataSectionPtr ds =
		BWResource::openSectionInTree( BWUtil::getFilePath( src ), "space.settings" );


	// If there is no space.setttings file associated with this cdata file
	// then return early.
	// This is the case with the helpers/blank_legacy.cdata file.
	if ( !ds )
	{
		WARNING_MSG("Couldn't find the space.settings file associated with '%s'."
					" Ignoring this .cdata file", src.c_str());
		return false;
	}
	stripWorldNavMesh_ = !ds->readBool( "clientNavigation/enabled", false );	

#ifndef MF_SERVER
	int terrainVersion = ds->readInt("terrain/version");
	if ( terrainVersion >= 200 )
	{
		DummyChunkTerrain::s_terrainChunkCDataFilename = 
			BWResolver::dissolveFilename( src );
		BW::StringRef identifier = BWResource::getFilename(
			DummyChunkTerrain::s_terrainChunkCDataFilename );
		identifier = BWResource::removeExtension( identifier );

		BW::string dissolvedChunkFilename = BWResource::changeExtension(
			DummyChunkTerrain::s_terrainChunkCDataFilename, ".chunk" );

		InitTerrainDependencies initTerrainDependencies( ds );

		if (DummyChunkTerrain::s_terrainSettings->pRenderer() == NULL)
		{
			src_.clear();
			dst_.clear();
			return true;
		}

		DataSectionPtr chunkSection = BWResource::openSection(
			dissolvedChunkFilename, false );
		if ( chunkSection == NULL )
		{
			return true;
		}

		BW::string errorString;
		BW::Matrix flattenMatrix;
		Chunk::loadInclude( chunkSection, flattenMatrix, &errorString, NULL,
			Chunk::isOutsideChunk( identifier ) );

		if (DummyChunkTerrain::s_terrainBlock == NULL)
		{
			return true;
		}

		// Make sure all the background loading tasks have been completed
		while (BgTaskManager::instance().numBgTasksLeft() > 0)
		{
			Yield();
		}

		stripLayers_ = false;
		stripLodTextures_ = false;
		stripLodNormals_ = false;
		stripQualityNormals_ = false;
		heightMapLodToPreserve_ = -1;
		heightMapLodToPreserve_ = DummyChunkTerrain::s_terrainBlock->getForcedLod();
		if (heightMapLodToPreserve_ < 0)
		{
			//Nothing to strip
			return true;
		}

		terrainDataSectionName_ =
			DummyChunkTerrain::s_terrainBlock->dataSectionName();

		float minDistance;
		float maxDistance;
		DummyChunkTerrain::s_terrainSettings->vertexLod().getDistanceForLod(
			heightMapLodToPreserve_, minDistance, maxDistance );

		uint8 renderTextureMask = TerrainRenderer2::getLoadFlag( 
			minDistance,
			DummyChunkTerrain::s_terrainSettings->absoluteBlendPreloadDistance(),
			DummyChunkTerrain::s_terrainSettings->absoluteNormalPreloadDistance());

		renderTextureMask = TerrainRenderer2::getDrawFlag( 
			renderTextureMask ,
			minDistance,
			maxDistance,
			DummyChunkTerrain::s_terrainSettings );

		stripLayers_ =
			( renderTextureMask & TerrainRenderer2::RTM_DrawBlend ) == false;

		stripLodTextures_ = !stripLayers_;
		stripLodNormals_ =
			( renderTextureMask & TerrainRenderer2::RTM_DrawLODNormals ) == false;

		stripQualityNormals_ = !stripLodNormals_;
	}
#endif //MF_SERVER
	return true;
}

bool CDataPacker::print()
{
	if ( src_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	printf( "CDataFile: %s\n", src_.c_str() );

	return true;
}

bool CDataPacker::pack()
{
	if ( src_.empty() || dst_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	if ( !PackerHelper::copyFile( src_, dst_ ) )
		return false;

	DataSectionPtr root = BWResource::openSection(
		BWResolver::dissolveFilename( dst_ ) );
	if ( !root )
	{
		printf( "Error opening as a DataSection\n" );
		return false;
	}

	// delete the thumbnail section
	root->delChild( "thumbnail.dds" );

	root->delChild( "navmesh" );

	if (stripWorldNavMesh_)
	{
		root->delChild( "worldNavmesh" );
	}

	SectionsToStrip::iterator iter = s_sectionsToStrip.begin();

	while (iter != s_sectionsToStrip.end())
	{
		root->delChild( *iter );

		++iter;
	}

#ifndef MF_SERVER
	DataSectionPtr dataSectionPtr = root->openSection(
		terrainDataSectionName_, false );

	if (dataSectionPtr != NULL)
	{
		if (stripLayers_)
		{
			dataSectionPtr->deleteSections( TerrainBlock2::LAYER_SECTION_NAME );
		}

		if (stripLodTextures_)
		{
			dataSectionPtr->deleteSections(
				TerrainBlock2::LOD_TEXTURE_SECTION_NAME );
		}

		if (stripQualityNormals_)
		{
			dataSectionPtr->deleteSections(
				TerrainNormalMap2::NORMALS_SECTION_NAME );
		}
		
		if (stripLodNormals_)
		{
			dataSectionPtr->deleteSections(
				TerrainNormalMap2::LOD_NORMALS_SECTION_NAME );
		}

		CommonTerrainBlock2::stripUnusedHeightSections(
			dataSectionPtr, heightMapLodToPreserve_ );
	}
#endif //MF_SERVER

#if 1
	{
		DataSection::iterator sectionIter = root->begin();
		while (sectionIter != root->end())
		{
			printf( "%s\n", (*sectionIter)->sectionName().c_str() );
			++sectionIter;
		}
	}
#endif

	// save changes on the destination file
	if ( !root->save() )
	{
		printf( "Error saving DataSection\n" );
		return false;
	}

	return true;
}

BW_END_NAMESPACE
