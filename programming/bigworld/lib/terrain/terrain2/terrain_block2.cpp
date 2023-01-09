#include "pch.hpp"
#include "terrain_block2.hpp"

#include "../terrain_data.hpp"
#include "terrain_height_map2.hpp"
#include "terrain_texture_layer2.hpp"
#include "terrain_hole_map2.hpp"
#include "terrain/terrain_settings.hpp"
#include "dominant_texture_map2.hpp"
#include "aliased_height_map.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"
#include "resmgr/bwresource.hpp"

#ifndef MF_SERVER
#include "d3dx9mesh.h"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/profiler.hpp"
#include "../manager.hpp"
//#include "terrain_index_buffer.hpp"
#include "terrain_renderer2.hpp"
#include "horizon_shadow_map2.hpp"
#include "terrain_normal_map2.hpp"
#include "terrain_ao_map2.hpp"
#include "terrain_vertex_buffer.hpp"
#include "moo/effect_material.hpp"
#include "moo/png.hpp"
#include "terrain_lod_map2.hpp"
#include "terrain_lod_controller.hpp"
#include "../terrain_collision_callback.hpp"
#include "chunk/chunk_terrain_obstacle.hpp"
#ifdef EDITOR_ENABLED
	#include "editor_vertex_lod_manager.hpp"
#else
	#include "vertex_lod_manager.hpp"
#endif

#endif

DECLARE_DEBUG_COMPONENT2("Moo", 0)


#ifndef MF_SERVER

BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{

/*
 *	This class is a helper class to calculate the offsets for generating
 *	inclusive and exclusive meshes for the umbra mesh of the terrain.
 */
class TerrainHullHelper
{
public:
	/*
	 * Constructor for the hull helper
	 * We init the helper with the heights from four corners in a quad from our
	 * lower resolution mesh and the sense value of the quad, which is one if
	 * the diagonal goes from the top left to the bottom right corner, and 0
	 * otherwise.
	 */
	TerrainHullHelper( float bottomLeft, float bottomRight, 
		float topLeft, float topRight, uint32 sense ) :
	bottomLeft_( bottomLeft ),
	bottomRight_( bottomRight ),
	topLeft_( topLeft ),
	topRight_( topRight ),
	sense_( sense ),
	maxA_( 0 ),
	maxB_( 0 ),
	minA_( 0 ),
	minB_( 0 )
	{
		BW_GUARD;
	}

	/*
	 *	This method updates the min and max values for the two triangles in the
	 *	mesh. x and z describe the normalised location in the current quad.
	 *	height is the height in the source height map at that location.
	 */
	void updateMinMax( float x, float z, float height )
	{
		BW_GUARD;
		// get the difference between the height in the quad 
		// and the passed in height
		float diff = height - heightAt( x, z );

		// Choose which triangle to update depending on the sense value
		// and the location in the quad, then update the min and max
		// values for that triangle
		if ((sense_ && (1.f - x) > z ) ||
			(!sense_ && x > z) )
		{
			maxA_ = std::max( maxA_, diff);
			minA_ = std::min( minA_, diff);
		}
		else
		{
			maxB_ = std::max( maxB_, diff);
			minB_ = std::min( minB_, diff);
		}
	}

	/*
	 *	This method outputs the occlusion heights of the quad
	 */
	void occlusionHeights( float& bottomLeft, float& bottomRight, 
		float& topLeft, float& topRight )
	{
		BW_GUARD;
		// get the combined value
		float combined = std::min( minA_, minB_ );

		// Depending on the sense, we generate the corner positions
		// of the quad
		if (sense_)
		{
			bottomLeft = bottomLeft_ + minA_;
			topRight = topRight_ + minB_;
			topLeft = topLeft_ + combined;
			bottomRight = bottomRight_ + combined;
		}
		else
		{
			bottomRight = bottomRight_ + minA_;
			topLeft = topLeft_ + minB_;
			topRight = topRight_ + combined;
			bottomLeft = bottomLeft_ + combined;
		}
	}

	/*
	 *	This method outputs the occlusion heights of the quad
	 */
	void visibilityHeights( float& bottomLeft, float& bottomRight, 
		float& topLeft, float& topRight )
	{
		BW_GUARD;
		// get the combined value
		float combined = std::max( maxA_, maxB_ );

		// Depending on the sense, we generate the corner positions
		// of the quad
		if (sense_)
		{
			bottomLeft = bottomLeft_ + maxA_;
			topRight = topRight_ + maxB_;
			topLeft = topLeft_ + combined;
			bottomRight = bottomRight_ + combined;
		}
		else
		{
			bottomRight = bottomRight_ + maxA_;
			topLeft = topLeft_ + maxB_;
			topRight = topRight_ + combined;
			bottomLeft = bottomLeft_ + combined;
		}
	}

private:

	float heightAt( float x, float z )
	{
		BW_GUARD;
		float res;
		// Work out the diagonals for the height map cell, neighbouring cells
		// have opposing diagonals.
		if (sense_)
		{
			// Work out which triangle we are in and calculate the interpolated
			// height.
			if ((1.f - x) > z)
			{
				res = bottomLeft_ + 
					(bottomRight_ - bottomLeft_) * x + 
					(topLeft_ - bottomLeft_) * z;
			}
			else
			{
				res = topRight_ + 
					(topLeft_ - topRight_) * (1.f - x) + 
					(bottomRight_ - topRight_) * (1.f - z);
			}
		}
		else
		{
			// Work out which triangle we are in and calculate the interpolated
			// height.
			if (x > z)
			{
				res = bottomRight_ + 
					(bottomLeft_ - bottomRight_) * (1.f - x) + 
					(topRight_ - bottomRight_) * z;
			}
			else
			{
				res = topLeft_ + 
					(topRight_ - topLeft_) * x + 
					(bottomLeft_ - topLeft_) * (1.f - z);
			}
		}
		return res;

	}

	uint32 sense_;

	float maxA_;
	float maxB_;
	float minA_;
	float minB_;

	float bottomLeft_;
	float bottomRight_;
	float topLeft_;
	float topRight_;
};


} // anonymous namespace

const BW::string TerrainBlock2::TERRAIN_BLOCK_META_DATA_SECTION_NAME( "TerrainBlockMetaData" );
const BW::string TerrainBlock2::FORCED_LOD_ATTRIBUTE_NAME( "ForcedLod" );
const BW::string TerrainBlock2::LOD_TEXTURE_SECTION_NAME( "lodTexture.dds" );
const BW::string TerrainBlock2::LAYER_SECTION_NAME( "layer" );

/**
 *  This is the TerrainBlock2 constructor.
 */
TerrainBlock2::TerrainBlock2(TerrainSettingsPtr pSettings):
    CommonTerrainBlock2(pSettings),
	pDetailHeightMapResource_(NULL),
	pVerticesResource_( NULL ),
	pNormalMap_( NULL ),
	pHorizonMap_( NULL ),
	pLodMap_( NULL ),
	pAOMap_ ( NULL ),
	depthPassMark_( 0 ),
	preDrawMark_( 0 ),
	forcedLod_( - 1 ),
	postLoadProcessTask_( NULL )
{
	BW_GUARD;
#ifndef EDITOR_ENABLED
	pBlendsResource_ = new TerrainBlendsResource( *this, settings_ == NULL ? false : settings_->bumpMapping() );
#else
	pBlendsResource_ = new TerrainBlendsResource( *this, true );
#endif
}

/**
 *  This is the TerrainBlock2 destructor.
 */
/*virtual*/ TerrainBlock2::~TerrainBlock2()
{
	BW_GUARD;
	// remove ourselves from controller
	if (Manager::pInstance() != NULL)
		BasicTerrainLodController::instance().delBlock( this );
	postLoadProcessTask_ = NULL;
}

/**
 *  This function loads an BaseTerrainBlock from a DataSection.
 *
 *  @param filename         A file to load the terrain block from.
 *  @param worldPosition    The position that the terrain will placed.
 *	@param cameraPosition	The position of the camera (to work out LoD level).
 *  @param error            If not null and there was an error loading
 *                          the terrain then this will be set to a
 *                          description of the problem.
 *  @returns                True if the load was completely successful,
 *                          false if a problem occurred.
 */
/*virtual*/ bool
TerrainBlock2::load(BW::string const &filename,
					Matrix  const &worldTransform,
					Vector3 const &cameraPosition,
					BW::string *error/* = NULL*/)
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( TerrainBlock2_load );
	// initialise our filename record
	fileName_ = filename;

	// try to open our section
	DataSectionPtr pTerrain = BWResource::openSection( filename );
	if ( !pTerrain )
	{
		if ( error ) *error = "Can't open terrain section.";
		return false;
	}

	DataSectionPtr terrainBlockMetaData = pTerrain->openSection(
		TERRAIN_BLOCK_META_DATA_SECTION_NAME );

	int forcedLod = -1;
	if( terrainBlockMetaData != NULL )
	{
		forcedLod = terrainBlockMetaData->readInt(
			FORCED_LOD_ATTRIBUTE_NAME, forcedLod );
	}

#if !defined(EDITOR_ENABLED)
	const uint32 detailedHeightMapLod =
		forcedLod < 0 ? settings_->topVertexLod() : forcedLod;
#else
	const uint32 detailedHeightMapLod =
		settings_->topVertexLod(); // Lod 0 is most detailed
#endif

	// set up detail height map resource (needs filename)
	pDetailHeightMapResource_ = new HeightMapResource( blockSize(), fileName_, detailedHeightMapLod );

	// In editor we load the most detailed height lod available.
#if !defined(EDITOR_ENABLED)
	const uint32 defaultHMLod = forcedLod < 0 ? settings_->defaultHeightMapLod() : forcedLod;
#else
	const uint32 defaultHMLod = 0; // Lod 0 is most detailed
#endif

	// call loading in the base class first
	if ( !CommonTerrainBlock2::internalLoad( filename, pTerrain, worldTransform, 
									cameraPosition, defaultHMLod, error ) )
	{
		return false;
	}

#ifndef EDITOR_ENABLED  // editor can't do async generation of loaded assets
	// set up a task to create the normalmap, lodmap and horizon shadow map
	// from the sections loaded here.
	MF_ASSERT( postLoadProcessTask_ == NULL );
	postLoadProcessTask_ = new PostLoadProcessTask( this );

	postLoadProcessTask_->pPendingLodSection_ 
		= pTerrain->openSection( LOD_TEXTURE_SECTION_NAME );

	postLoadProcessTask_->pPendingNormalSection_ 
		= pTerrain->openSection( TerrainNormalMap2::LOD_NORMALS_SECTION_NAME );
	
	postLoadProcessTask_->pPendingHorizonSection_ 
		= pTerrain->openSection( "horizonShadows" );

	postLoadProcessTask_->worldTransform_ = worldTransform;

	if (!postLoadProcessTask_->pPendingHorizonSection_)
	{
		if ( error ) *error = "No horizon shadows found in terrain chunk.";
		postLoadProcessTask_ = NULL;
		return false;
	}


	// Initialise vertex LOD cache
	if ( !initVerticesResource( error ) )
		return false;

	// Initialise normals
	pNormalMap_ = new TerrainNormalMap2;

	Matrix invWorld;
	invWorld.invert( worldTransform );

	// Needs to wait for VertexLodManager to be setup
	// but needs to be set before evaluate()
	setForcedLod( forcedLod );

	// Evaluate this block's visibility settings
	// needs pNormalMap, pBlendsResource, lodRenderInfo, 
	evaluate( invWorld.applyPoint(cameraPosition) );

	stream();

	if (ResourceBase::defaultStreamType() == RST_Syncronous)
	{
		postLoadProcessTask_->doSynchronousTask();
		bw_safe_delete(postLoadProcessTask_);
	}
	else
	{
		BgTaskManager::instance().addBackgroundTask(postLoadProcessTask_);
	}

	return true;

#else
	if (DataSectionPtr pLodMapData = pTerrain->openSection( LOD_TEXTURE_SECTION_NAME ))
	{
		pLodMap_ = new TerrainLodMap2;

		if(!pLodMap_->load(pLodMapData))
		{
			pLodMap_ = NULL;
		}

	}

	// Initialise vertex LOD cache
	if ( !initVerticesResource( error ) )
		return false;

	// Initialise normals
	pNormalMap_ = new TerrainNormalMap2;

	bool normalsLoaded = false;
	normalsLoaded = pNormalMap_->init( fileName_ , NULL);

	//-- initialize Ambient Occlusion
	pAOMap_ = new TerrainAOMap2;

	bool aoLoaded = false;
	aoLoaded = pAOMap_->init( fileName_ );

	Matrix invWorld;
	invWorld.invert( worldTransform );

	//Needs to wait for VertexLodManager to be setup
	//but needs to be set before evaluate()
	setForcedLod( forcedLod );

	// Evaluate this block's visibility settings
	evaluate( invWorld.applyPoint(cameraPosition) );

	stream();

	if (!normalsLoaded)
	{
#ifndef EDITOR_ENABLED
		if( getForcedLod() < 0 ||
			pNormalMap_->needsQualityMap() == false )
		{
			if (error) *error = "No valid normals found.";
			return false;
		}
#else
		// Editor may rebuild normals.
		rebuildNormalMap( NMQ_NICE );
#endif
	}

	if (!aoLoaded)
	{
#ifdef EDITOR_ENABLED
		rebuildAOMap( settings_->aoMapSize() );
#endif
	}

	if( s_disableStreaming_ == false )
	{
		PROFILE_FILE_SCOPED( HorizonShadowMap2_load );
		pHorizonMap_ = new HorizonShadowMap2(this);
		DataSectionPtr pHorizonShadows = pTerrain->openSection(
			"horizonShadows" );
		if (!pHorizonShadows)
		{
			if ( error ) *error = "No horizon shadows found in terrain chunk.";
			return false;
		}
		if ( !pHorizonMap_->load(pHorizonShadows) )
		{
			if ( error ) *error = "Couldn't load supplied horizon shadow data.";
			return false;
		}
	}

	// add ourselves to controller
	BasicTerrainLodController::instance().addBlock( this, worldTransform );
#endif

	return true;
}


/**
 * Task for generating the lodmap, normal map and horizon shadow maps from 
 * sections already loaded on the fileIO thread.
 *
 * @param  block terrainBlock that we are generating for.
 *
 */
TerrainBlock2::PostLoadProcessTask::PostLoadProcessTask( 
	SmartPointer<TerrainBlock2> block) :
	BackgroundTask( "PostLoadProcess" )
{
	terrainBlock_ = block;
}

TerrainBlock2::PostLoadProcessTask::~PostLoadProcessTask()
{
	pLodMap_ = NULL;
	pHorizonMap_ = NULL;
	pPendingHorizonSection_ = NULL;
	pPendingLodSection_ = NULL;
	pPendingNormalSection_ = NULL;
	terrainBlock_ = NULL;
}


void TerrainBlock2::PostLoadProcessTask::doBackgroundTask( TaskManager & mgr )
{
	performBackgroundSteps();
	BgTaskManager::instance().addMainThreadTask( this );
}


/**
 * Once the maps have been generated, set them in the parent terrainBlock back
 * on the main thread (to avoid race conditions).
 *
 */
void TerrainBlock2::PostLoadProcessTask::doMainThreadTask( TaskManager & mgr )
{
	performForegroundSteps();
}


void TerrainBlock2::PostLoadProcessTask::doSynchronousTask()
{
	performBackgroundSteps();
	performForegroundSteps();
}


void TerrainBlock2::PostLoadProcessTask::performBackgroundSteps()
{
#ifdef EDITOR_ENABLED
	MF_ASSERT(false);
#endif
	if (pPendingLodSection_)
	{
		PROFILE_FILE_SCOPED( TerrainLodMap2_loading );
		pLodMap_ = new TerrainLodMap2;

		if (!pLodMap_->load( pPendingLodSection_ ))
		{
			pLodMap_ = NULL;
		}

	}

	bool normalsLoaded = false;
	{
		PROFILE_FILE_SCOPED( normalMap_load );
		normalsLoaded = terrainBlock_->pNormalMap_->init( 
			terrainBlock_->fileName_, pPendingNormalSection_ );
	}
	if (!normalsLoaded)
	{
		if (terrainBlock_->getForcedLod() < 0 ||
			terrainBlock_->pNormalMap_->needsQualityMap() == false)
		{
			error_ = "No valid normals found.";
		}
	}

	{
		PROFILE_FILE_SCOPED( HorizonShadowMap2_load );
		pHorizonMap_ = new HorizonShadowMap2( terrainBlock_.get() );
		if (!pHorizonMap_->load( pPendingHorizonSection_ ))
		{
			error_ = "Couldn't load supplied horizon shadow data.";
		}
	}

}

void TerrainBlock2::PostLoadProcessTask::performForegroundSteps()
{
	// add ourselves to controller
	if (error_.empty())
	{
		terrainBlock_->pLodMap_ = pLodMap_;
		terrainBlock_->pHorizonMap_ = pHorizonMap_;

		BasicTerrainLodController::instance().addBlock( terrainBlock_.get(), worldTransform_ );
	}
	else
	{
		DEBUG_MSG( "error loading terrain chunk %s\n", error_.c_str() );
	}

	// free up smart objects
	pLodMap_ = NULL;
	pHorizonMap_ = NULL;
	pPendingHorizonSection_ = NULL;
	pPendingLodSection_ = NULL;
	pPendingNormalSection_ = NULL;
	terrainBlock_ = NULL;
}


/**
 *	Initialises the vertex buffer holder for this terrain block.
 *
 *  @param error		    Optional output parameter that receives an error
 *							message if an error occurs.
 *  @returns                True if successful, false otherwise.
 */
bool TerrainBlock2::initVerticesResource( BW::string*	error/* = NULL*/ )
{
	BW_GUARD;
	pVerticesResource_ = new VerticesResource( 
		*this, settings_->numVertexLods() );

	if ( !pVerticesResource_ )
	{
		if ( error ) *error = "Couldn't allocate a VerticesResource.";
		return false;
	}

	return true;
}

/**
 *  This function gets the shadow map for the terrain.
 *
 *  @returns                The shadow map for the terrain.
 */
/*virtual*/ HorizonShadowMap &TerrainBlock2::shadowMap()
{
    return *pHorizonMap_;
}


/**
 *  This function gets the shadow map for the terrain.
 *
 *  @returns                The shadow map for the terrain.
 */
/*virtual*/ HorizonShadowMap const &TerrainBlock2::shadowMap() const
{
    return *pHorizonMap_;
}

/**
 *	This function prepares the terrain block for drawing.
 *	
 *	@param	doSubstitution		Substitute a missing vertex lod for best 
 *								available.
 *
 *	@returns true, if the block was successfully set up.
 */
bool TerrainBlock2::preDraw( bool doSubstitution )
{
	BW_GUARD_PROFILER( TerrainBlock2_predraw );

	// set up masks and morph range
	settings_->vertexLod().calculateMasks( distanceInfo_, lodRenderInfo_ );

#ifndef EDITOR_ENABLED
	// Set up lod texture mask - we have a partial result from evaluate()
	// step. Editor already has full result.
	lodRenderInfo_.renderTextureMask_  = TerrainRenderer2::getDrawFlag( 
											lodRenderInfo_.renderTextureMask_,
											distanceInfo_.minDistanceToCamera_, 
											distanceInfo_.maxDistanceToCamera_, 
											settings_ );
#endif

	// Return true if the vertices are available
	VertexLodManager* vlm = pVerticesResource_.getObject();
	MF_ASSERT( vlm && "VertexLodManager should be loaded!" );

	// Mark the block as drawn in this render target
	if ( preDrawMark_ != Moo::rc().renderTargetCount() )
	{
		int forcedLod = std::min( getForcedLod(), ( int ) vlm->getLowestLod() );
		uint32 currentLod = forcedLod < 0 ? distanceInfo_.currentVertexLod_ : forcedLod;
		uint32 nextLod = forcedLod < 0 ? distanceInfo_.nextVertexLod_ : forcedLod;
		currentDrawState_.currentVertexLodPtr_= 
			vlm->getLod( currentLod, doSubstitution );
		currentDrawState_.nextVertexLodPtr_	= 
			vlm->getLod( nextLod, doSubstitution );
		currentDrawState_.blendsPtr_			= pBlendsResource_->getObject();
		currentDrawState_.blendsRendered_		= false;
		currentDrawState_.lodsRendered_		= false;

		preDrawMark_ = Moo::rc().renderTargetCount();
	}

	return currentDrawState_.currentVertexLodPtr_.exists() 
		&& currentDrawState_.nextVertexLodPtr_.exists();
}

bool TerrainBlock2::draw(const Moo::EffectMaterialPtr& pMaterial)
{
	BW_GUARD_PROFILER( TerrainBlock2_draw );

	bool ret = false;

	// Draw each block with in two stages. On the first pass we draw
	// appropriate sub-blocks at current lod, and degenerate triangles.
	// on second pass, draw the *other* sub-blocks at next lod.
	const VertexLodEntryPtr& currentLod	= currentDrawState_.currentVertexLodPtr_;
	const VertexLodEntryPtr& nextLod	= currentDrawState_.nextVertexLodPtr_;

	// If the current lod exists ...
	if ( currentLod.exists() )
	{
		// If we don't have the next lod, then just draw the whole thing.
		if ( !nextLod.exists() )
		{
			// Warn developer, player is either moving too fast or streaming is
			// too slow.
			//WARNING_MSG("Didn't load vertex lod in time,"
			//			" terrain cracks may appear.\n");
			lodRenderInfo_.subBlockMask_ = 0xF;
		}

		ret = currentLod->draw( pMaterial, lodRenderInfo_.morphRanges_.main_,
											lodRenderInfo_.neighbourMasks_, 
											lodRenderInfo_.subBlockMask_ );

		// If we didn't draw everything above then draw the other parts.
		if ( lodRenderInfo_.subBlockMask_ != 0xF )
		{
			MF_ASSERT_DEBUG( nextLod.exists() );
			ret = nextLod->draw( pMaterial, lodRenderInfo_.morphRanges_.subblock_,
											lodRenderInfo_.neighbourMasks_, 
											lodRenderInfo_.subBlockMask_ ^ 0xF );
		}
	}
	else
	{
		WARNING_MSG("TerrainBlock2::draw(), currentLodEntry doesn't exist, not "
					"drawing anything.\n" );
	}

	return ret;
}

/**
 *	This function manages resources dependencies for the block. It calculates 
 *	visibility settings, which are later used to stream or discard resources as 
 *	appropriate. The actual streaming occurs in the stream() function.
 *
 *	@param relativeCameraPos	The camera position relative to the block
 */
void TerrainBlock2::evaluate(	const Vector3& relativeCameraPos )
{
	BW_GUARD;
	//
	// Calculate distance info for this block
	//
	{
		BW_GUARD_PROFILER( TerrainBlock2_evaluate_calculate );

		// Position of this block relative to camera 
		distanceInfo_.relativeCameraPos_ = relativeCameraPos;

		// Maximum and minimum distances (relative to camera) of the block's
		// corners.
		TerrainVertexLod::minMaxXZDistance( distanceInfo_.relativeCameraPos_ , 
											blockSize(), 
											distanceInfo_.minDistanceToCamera_, 
											distanceInfo_.maxDistanceToCamera_ );

		// The LOD level of this block.
		uint32 lastLOD = settings_->numVertexLods();
		if ( lastLOD > 0 ) // in case there are no LODs
		{
			--lastLOD;
		}
		distanceInfo_.currentVertexLod_ = 
			settings_->vertexLod().calculateLodLevel( 
			distanceInfo_.minDistanceToCamera_, settings_.get() );
		distanceInfo_.nextVertexLod_ = 
			std::min( distanceInfo_.currentVertexLod_ + 1U, lastLOD);

		const int forcedLOD = std::min( getForcedLod(), (int) lastLOD );
		if (forcedLOD >= 0)
		{
			distanceInfo_.currentVertexLod_	= 
				distanceInfo_.nextVertexLod_ = forcedLOD;
			settings_->vertexLod().getDistanceForLod( 
				distanceInfo_.currentVertexLod_, 
				distanceInfo_.minDistanceToCamera_, 
				distanceInfo_.maxDistanceToCamera_ );
		}
	}

	//
	// Set up streaming - given dependencies work out what to stream.
	//
	{
		BW_GUARD_PROFILER( TerrainBlock2_evaluate_evaluate );

		// Evaluate whether or not we need the detail height map to generate 
		// the vertices, or for collisions.
		uint32 requiredVertexGridSize = 
			VertexLodManager::getLodSize(	distanceInfo_.currentVertexLod_, 
											settings_->numVertexLods() );

		// If we're in the editor, we never need detail height maps due to 
		// distance requirements (grid size requirement should also pass 
		// because default height map should be maximum size in load() above).
#if !defined(EDITOR_ENABLED)
		float detailDistance = settings_->detailHeightMapDistance();
#else
		float detailDistance = 0;
#endif
		pDetailHeightMapResource_->evaluate(
									requiredVertexGridSize, 
									heightMap().blocksWidth(),
									detailDistance, 
									distanceInfo_.minDistanceToCamera_,
									settings_->topVertexLod());

		// Vertex evaluation
		pVerticesResource_->evaluate( distanceInfo_.currentVertexLod_, settings_->topVertexLod() );

		// Blends evaluation

		// Get a partial result of the render texture mask, just enough to tell
		// whether we need to load blends or not. This partial result will be
		// expanded further in preDraw().
		lodRenderInfo_.renderTextureMask_ = TerrainRenderer2::getLoadFlag( 
									distanceInfo_.minDistanceToCamera_,
									settings_->absoluteBlendPreloadDistance(),
									settings_->absoluteNormalPreloadDistance());
#ifdef EDITOR_ENABLED
		// Editor needs full result here for blends
		lodRenderInfo_.renderTextureMask_ = TerrainRenderer2::getDrawFlag( 
									lodRenderInfo_.renderTextureMask_ ,
									distanceInfo_.minDistanceToCamera_,
									distanceInfo_.maxDistanceToCamera_,
									settings_ );
#endif
		pBlendsResource_->evaluate( lodRenderInfo_.renderTextureMask_ );
		pNormalMap_->evaluate( lodRenderInfo_.renderTextureMask_ );
	}

	currentDrawState_.currentVertexLodPtr_	= NULL;
	currentDrawState_.nextVertexLodPtr_		= NULL;
	currentDrawState_.blendsPtr_				= NULL;
}

void TerrainBlock2::stream()
{
	BW_GUARD;
	//
	// Do streaming
	//
	pDetailHeightMapResource_->stream();
	pVerticesResource_->stream();

	// Stream blends if we are not in a reflection. We override it here rather 
	// than in visibility test to stop reflection scene from unloading blends
	// required by main scene render.
	if ( !Moo::rc().reflectionScene() )
	{
		pBlendsResource_->stream();
	}

	pNormalMap_->stream(); 
}

/**
 * This function returns true if we are able to draw the lod texture.
 */
bool TerrainBlock2::canDrawLodTexture() const
{
	BW_GUARD;
	return settings_->useLodTexture() && ( pLodMap_ != NULL );
}

/**
 * This function returns true if we are doing background tasks
 */
bool TerrainBlock2::doingBackgroundTask() const
{
	BW_GUARD;
	return pVerticesResource_->getState() == RS_Loading ||
		pBlendsResource_->getState() == RS_Loading ||
		pDetailHeightMapResource_->getState() == RS_Loading ||
		pNormalMap_->isLoading();
}

/**
 * This functions returns true if at least one LOD has been loaded.
 */
bool TerrainBlock2::readyToDraw() const
{
	VertexLodManager* vlm = pVerticesResource_.getObject();	
	return vlm && vlm->getLod(0, true);
}

/**
 * This function returns the highest lod height map available for this block,
 * and its lod level (if requested).
 */
const TerrainHeightMap2Ptr 
	TerrainBlock2::getHighestLodHeightMap(
#ifndef MF_SERVER
			bool useRealHighestLod
#endif
	) const
{
	BW_GUARD;

#if !defined(EDITOR_ENABLED)

#ifdef MF_SERVER
	TerrainHeightMap2Ptr returnValue = pDetailHeightMapResource_->getObject();
	if ( returnValue.exists() )
	{
		// detail lod is loaded, return it
		return returnValue;
	}
#else
	// HACK !!!!!!!!!!!!!!!!! POTENTIALLY DANGEROUS CHANGE !!!!!!!!!!!!!!!!!!!!!
	// USE CASE: low graphics settings should have terrain physics with highest LOD available in terrain data.
	// CHANGE DESCRIPTION: Here we perform additional check of heightmap stored in heightMap2()
	// (which is in turn CommonTerrainBlock2::pHeightMap_ for client app) and compare its 
	// LOD with pDetailHeightMapResource_->lodLevel to make sure that on low graphics settings
	// (where pDetailHeightMapResource_ has LOD=1 etc) we still can use height map loaded 
	// with <defaultHeightMapLod> (which is what heightMap2() returns) if that one has higher lod level.
	uint32 highestLod = 0xffffffff;
	if (useRealHighestLod)
	{
		TerrainHeightMap2Ptr hm2 = heightMap2();
		if (hm2.exists())
			highestLod = hm2->lodLevel();
	}

	TerrainHeightMap2Ptr returnValue = pDetailHeightMapResource_->getObject();
	if (returnValue.exists() && returnValue->lodLevel() < highestLod)
	{
		// detail lod is loaded, return it
		return returnValue;
	}	
#endif

#endif

	return heightMap2();
}

/**
 * Override to use best heightmap loaded. 
 */
BoundingBox const & TerrainBlock2::boundingBox() const
{
	BW_GUARD;
	TerrainHeightMap2Ptr hm = getHighestLodHeightMap();

	// HACK: bbox of highmaps for differnt lods may differ slightly,
	// so beacause of using detail heightmap for physics,
	// bbox should bound both detail and current lods.
	// Get detail lod heighmap
	TerrainHeightMap2Ptr dhm = heightMap2();

	// extend bounds
	float maxh = std::max(hm->maxHeight(), dhm->maxHeight());
	float minh = std::min(hm->minHeight(), dhm->minHeight());

	bb_.setBounds( Vector3( 0.f, minh, 0.f ), 
		Vector3( blockSize(), maxh, blockSize() ) );

	return bb_;
}

/**
 * Override to collide with best height map loaded.
 */
bool TerrainBlock2::collide
(
	 Vector3                 const &start, 
	 Vector3                 const &end,
	 TerrainCollisionCallback *callback
) const
{
	BW_GUARD;
	return getHighestLodHeightMap(
#ifndef MF_SERVER
		(callback != NULL) ? callback->ccb_.useHighestAvailableLod() : false
#endif
		)->collide( start, end, callback );
}

/**
* Override to collide with best height map loaded.
*/
bool TerrainBlock2::collide
(
	 WorldTriangle           const &start, 
	 Vector3                 const &end,
	 TerrainCollisionCallback *callback
 ) const
{
	BW_GUARD;
	return getHighestLodHeightMap(
#ifndef MF_SERVER
		(callback != NULL) ? callback->ccb_.useHighestAvailableLod() : false
#endif
	)->collide( start, end, callback );
}

/**
 * Override to get height with best height map loaded
 */
float TerrainBlock2::heightAt(float x, float z) const
{
	BW_GUARD;
	float res = NO_TERRAIN;

	if (!holeMap().holeAt( x, z ))
	{
		res = getHighestLodHeightMap()->heightAt(x, z);
	}

	return res;
}

/**
* Override to get normal with best height map loaded
*/
Vector3 TerrainBlock2::normalAt(float x, float z) const
{
	return getHighestLodHeightMap()->normalAt(x, z);
}


/**
 *
 */
void TerrainBlock2::aoMap2(const TerrainAOMap2Ptr& aoMap2)
{
	pAOMap_ = aoMap2;
}


/**
 *
 */
TerrainAOMap2Ptr TerrainBlock2::aoMap2() const
{
	return pAOMap_;
}


/**
 *	Get the terrain version.
 */
uint32 TerrainBlock2::getTerrainVersion() const
{
	return TerrainSettings::TERRAIN2_VERSION;
}


/**
 *	This function allows access to the normal map.  This can be used by
 *	derived classes.
 *
 *  @param normalMap2			The TerrainNormalMap2.
 */
void TerrainBlock2::normalMap2( TerrainNormalMap2Ptr normalMap2 )
{
	pNormalMap_ = normalMap2;
}


/**
 *	This function allows access to the normal map.  This can be used by
 *	derived classes.
 *
 *  @returns			The TerrainNormalMap2.
 */
TerrainNormalMap2Ptr TerrainBlock2::normalMap2() const
{
	return pNormalMap_;
}


/**
 *	This function allows access to the shadow map.  This can be used by
 *	derived classes.
 *
 *  @param horizonMap2			The HorizonShadowMap2.
 */
void TerrainBlock2::horizonMap2( HorizonShadowMap2Ptr horizonMap2 )
{
	pHorizonMap_ = horizonMap2;
}


/**
 *	This function allows access to the shadow map.  This can be used by
 *	derived classes.
 *
 *  @returns			The HorizonShadowMap2.
 */
HorizonShadowMap2Ptr TerrainBlock2::horizonMap2() const
{
	return pHorizonMap_;
}

uint32 TerrainBlock2::aoMapSize() const
{
	return (pAOMap_ != NULL) ? pAOMap_->size() : 0;
}

DX::Texture* TerrainBlock2::pAOMap() const
{
    return (pAOMap_ != NULL) ? pAOMap_->texture() : NULL;
}

uint32 TerrainBlock2::normalMapSize() const
{
    return pNormalMap_->size();
}


DX::Texture* TerrainBlock2::pNormalMap() const
{
    return pNormalMap_->pMap().pComObject();
}


uint32 TerrainBlock2::horizonMapSize() const
{
    return (pHorizonMap_ != NULL) ? pHorizonMap_->width() : 0;
}


DX::Texture* TerrainBlock2::pHorizonMap() const
{
    return (pHorizonMap_ != NULL) ? pHorizonMap_->texture() : NULL;
}


DX::Texture* TerrainBlock2::pHolesMap() const
{
    return holeMap2() != NULL ? holeMap2()->texture() : NULL;
}


uint32 TerrainBlock2::nLayers() const
{
	size_t layers = pBlendsResource_->getObject() ? 
		pBlendsResource_->getObject()->combinedLayers_.size() : 0;
	MF_ASSERT( layers <= UINT_MAX );
	return ( uint32 ) layers;
}


const CombinedLayer* TerrainBlock2::layer( uint32 index ) const
{
	return pBlendsResource_->getObject() ? 
		&(pBlendsResource_->getObject()->combinedLayers_[index]) : NULL;
}


/**
 *  This method accesses the texture layers.
 */
TextureLayers* TerrainBlock2::textureLayers()
{
	return pBlendsResource_->getObject() ? 
		&(pBlendsResource_->getObject()->textureLayers_) : NULL;
}

/**
 *  This method accesses the texture layers.
 */
TextureLayers const * TerrainBlock2::textureLayers() const
{
	return pBlendsResource_->getObject() ? 
		&(pBlendsResource_->getObject()->textureLayers_) : NULL;
}

TerrainLodMap2Ptr TerrainBlock2::lodMap() const
{
	return pLodMap_;
}

/*virtual */void TerrainBlock2::setForcedLod( int forcedLod )
{
	VertexLodManager* vlm = pVerticesResource_.getObject();
	MF_ASSERT( vlm && "VertexLodManager should be loaded!" );

	int clampedForcedLod = std::max( -1, std::min( forcedLod, ( int ) vlm->getNumLods() - 1 ) );
	forcedLod_ = clampedForcedLod;
}

int Terrain::TerrainBlock2::getTextureMemory() const
{
	int memory = 0;
	int numLayers = this->nLayers();
	for( int i = 0; i < numLayers; ++i)
	{
		const CombinedLayer* pLayer = layer(i);
		if(pLayer)
			memory += DX::textureSize(pLayer->pBlendTexture_.pComObject(), ResourceCounters::MT_GPU);
	}
	const TextureLayers* pTexLayers = textureLayers();
	if(pTexLayers)
		for (size_t i = 0; i < pTexLayers->size(); ++i)
		{
			memory += pTexLayers->at(i)->texture()->textureMemoryUsed();
		}

	return memory;
}

int Terrain::TerrainBlock2::getVertexMemory() const
{
	int memory = 0;
	if(pVerticesResource_.getObject())
	{
		uint32 numLods = pVerticesResource_->getNumLods();
		for(uint32 i = 0; i < numLods; ++i)
		{
			VertexLodEntry* pLod = pVerticesResource_->getLod(i).get();
			if(pLod)
				memory += pLod->sizeInBytes_;
		}
	}
	return memory;
}

void Terrain::TerrainBlock2::getTexturesAndVertices( BW::map<void*, int>& textures, BW::map<void*, int>& vertices )
{
	//get textures
	int numLayers = this->nLayers();
	for( int i = 0; i < numLayers; ++i)
	{
		const CombinedLayer* pLayer = layer(i);
		if(pLayer)
			textures.insert(std::make_pair(pLayer->pBlendTexture_.pComObject(), DX::textureSize(pLayer->pBlendTexture_.pComObject(), ResourceCounters::MT_GPU)));
	}
	TextureLayers* pTexLayers = textureLayers();
	if(pTexLayers)
		for (size_t i = 0; i < pTexLayers->size(); ++i)
			textures.insert(std::make_pair(pTexLayers->at(i)->texture().get(), pTexLayers->at(i)->texture()->textureMemoryUsed()));

	//get vertices
	if(pVerticesResource_.getObject())
	{
		uint32 numLods = pVerticesResource_->getNumLods();
		for(uint32 i = 0; i < numLods; ++i)
		{
			VertexLodEntry* pLod = pVerticesResource_->getLod(i).get();
			if(pLod)
				pLod->getVerticesMemoryMap(vertices);
		}
	}
}

BW_END_NAMESPACE

#endif // MF_SERVER

// terrain_block2.cpp
