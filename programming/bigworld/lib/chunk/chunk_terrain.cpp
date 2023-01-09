#include "pch.hpp"
#include "chunk_terrain.hpp"
#ifndef CODE_INLINE
#include "chunk_terrain.ipp"
#endif

#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "chunk_obstacle.hpp"
#include "chunk_space.hpp"
#include "chunk_terrain_common.hpp"

#include "fmodsound/sound_manager.hpp"

#include "geometry_mapping.hpp"
#include "grid_traversal.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/guard.hpp"

#include "scene/scene_object.hpp"

#include "terrain/base_terrain_block.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "terrain/terrain_collision_callback.hpp"
#include "terrain/terrain_data.hpp"
#include "terrain/terrain_finder.hpp"
#include "terrain/terrain_height_map.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain2/terrain_height_map2.hpp"

#include "moo/render_context.hpp"
#include "moo/visual.hpp"

#include "physics2/hulltree.hpp"

#include "resmgr/datasection.hpp"

#include "romp/water_scene_renderer.hpp"
#include "moo/line_helper.hpp"
#include "romp/time_of_day.hpp"

#include "umbra_config.hpp"

#if UMBRA_ENABLE
#include "umbra_chunk_item.hpp"
#include <ob/umbraObject.hpp>
#include "chunk_umbra.hpp"
#endif


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

int ChunkTerrain_token;

bool use_water_culling = true;

namespace
{
	//These are all helpers to debug / change the shadow calculation rays.
	bool                 s_debugShadowRays   = false;
	bool                 s_debugShadowPoles  = false;
	BW::vector<Vector3> s_lines;
	BW::vector<Vector3> s_lineEnds;
	int                  s_collType          =     1;
	float                s_raySubdivision    =  32.f;
	float                s_rayDisplacement   =  1.5f;

	const float SHADOW_RAY_ERROR_OFFSET	= 0.001f;
	const int	SHADOW_MAX_VALUE		= 65535;
	const int	SHADOW_MAX_CALC_RAYS	= 256;
	const int	SHADOW_MAX_CALC_RAY_VAL	= SHADOW_MAX_CALC_RAYS - 1;

	class ShadowObstacleCatcher : public CollisionCallback
	{
	public:
		ShadowObstacleCatcher(bool terrainOnly) :
			hit_( false ),
			terrainOnly_(terrainOnly),
			dist_()			
		{}

		void direction( const Vector3& dir )	{ dir_ = dir; }
		bool hit() { return hit_; }
		void hit( bool h ) { hit_ = h; }
		float dist() const	{	return dist_;	}
	private:
		virtual int operator()( const CollisionObstacle & obstacle,
			const WorldTriangle & triangle, float dist )
		{
			BW_GUARD;

			dist_ = dist;
			if(terrainOnly_) {
				if(triangle.flags() & TRIANGLE_TERRAIN) {
					hit_ = true;
					return COLLIDE_STOP;
				}
				return COLLIDE_ALL;
			}

			if (!(triangle.flags() & TRIANGLE_TERRAIN))
			{
				// TODO: Editor code, just assuming ChunkItem at least for now.
				// Should probably go via some kind of QueryOperation. 
				MF_ASSERT( obstacle.sceneObject().isValid() );
				if (!obstacle.sceneObject().flags().isCastingStaticShadow())
				{
					return COLLIDE_ALL;
				}
			}

			// Transparent BSP does not cast shadows
			if( triangle.flags() & TRIANGLE_TRANSPARENT )
				return COLLIDE_ALL;

			hit_ = true;

			// We only require the first collision for shadow.
			return COLLIDE_STOP;
		}

		bool hit_;
		bool terrainOnly_;
		float dist_;
		Vector3 dir_;
	};

	float getLandHeight( float x, float z )
	{
		ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
		if ( space )
		{
			Chunk *pChunk = space->findChunkFromPoint( Vector3( x, 0, z ) );
			if ( pChunk )
			{
				float fHeight;
				const Matrix &tr = pChunk->transform();
				x -= tr._41;
				z -= tr._43;
				if ( pChunk->getTerrainHeight( x, z, fHeight ) )
					return fHeight;
			}
		}
		return 20.0f;
	}

	// Returns two boolean values.
	// first  -- for all shadows (terrain and models)
	// second -- for terrain shadows only
	//
	// true  -- point on the light
	// false -- point in shadow
	std::pair<bool, bool> 
		getShade(const Chunk* pChunk, float chunkX, float chunkZ, const Vector3 lightDir)
	{
		BW_GUARD;

		Vector3 pos = pChunk->transform().applyPoint(
			Vector3(chunkX, 0.f, chunkZ));
		pos.y = getLandHeight(pos.x, pos.z);
		pos += Vector3(0.0f, 0.1f, 0.0001f); 

		Vector3 p2 = pos;
		Vector3 p1 = p2 - MAX_TERRAIN_SHADOW_RANGE * lightDir;

		// adding an offset to avoid errors in the collisions at chunk seams
		p1.z += SHADOW_RAY_ERROR_OFFSET;
		p2.z += SHADOW_RAY_ERROR_OFFSET;

		ShadowObstacleCatcher soc_all(false);
		ShadowObstacleCatcher soc_trr(true);

		ChunkManager::instance().cameraSpace()->collide(p1, p2, soc_all);
		ChunkManager::instance().cameraSpace()->collide(p1, p2, soc_trr);

		bool res_all = !soc_all.hit();
		bool res_trr = !soc_trr.hit();

		return std::pair<bool, bool> (res_all, res_trr);
	}

	//std::pair<int, Vector3> 
	//   getMinVisibleAngle0
	//   ( 
	//       Chunk          *pChunk, 
	//       Vector3        const &sPolePos, 
	//       Vector3        const &lastPos, 
	//       bool           reverse
	//   )
	//{
	//	BW_GUARD;

	//	// Cast up to SHADOW_RAYS_IN_CALCULATION rays, from horiz over a 180
	//	// deg range, finding the 1st one that doesn't collide (do it twice
	//	// starting from diff directions each time)

	//	// We've gotta add a lil to the z, or when checking along chunk boundaries
	//	// we don't hit the terrain block
	//	Vector3 polePos = sPolePos + Vector3(0.0f, 0.1f, 0.0001f);

	//	// apply the chunk's xform to polePos to get it's world pos
	//	polePos = pChunk->transform().applyPoint( polePos );

	//	// adding an offset to avoid errors in the collisions at chunk seams
	//	polePos.z += SHADOW_RAY_ERROR_OFFSET;

	//	Vector3 lastHit = lastPos;
	//	float xdiff = reverse ? polePos.x - lastPos.x : lastPos.x - polePos.x;
	//	float ydiff = lastPos.y - polePos.y;
	//	bool valid = lastPos.y > polePos.y;
	//	float lastAngle = atan2f( ydiff, xdiff );
	//	if( lastAngle < 0 )
	//		lastAngle += MATH_PI / 2;

	//	for (int i = 0; i < SHADOW_MAX_CALC_RAYS; i++)
	//	{
	//		// sun travels along the x axis (thus around the z axis)

	//		// Make a ray that points the in correct direction
	//		float angle = MATH_PI * (i / float(SHADOW_MAX_CALC_RAY_VAL) );

	//		if( valid && angle < lastAngle )
	//			continue;

	//		Vector3 ray ( cosf( angle ), sinf( angle ), 0.f );

	//		if (reverse)
	//			ray.x *= -1;

	//		if (s_debugShadowRays)
	//		{
	//			s_lines.push_back( polePos );
	//			s_lineEnds.push_back( polePos + (ray * MAX_TERRAIN_SHADOW_RANGE) );
	//		}

	//		ShadowObstacleCatcher soc;
	//		soc.direction(ray);

	//		Terrain::TerrainHeightMap2::optimiseCollisionAlongZAxis( true );

	//		ChunkManager::instance().cameraSpace()->collide( 
	//			polePos,
	//			polePos + (ray * MAX_TERRAIN_SHADOW_RANGE),
	//			soc );

	//		Terrain::TerrainHeightMap2::optimiseCollisionAlongZAxis( false );

	//		if (!soc.hit())
	//			return std::make_pair( i, lastHit );
	//		else
	//			lastHit = Vector3( polePos + soc.dist() * ray );

	//		WorldManager::instance().fiberPause();
	//	}

	//	return std::make_pair( 0xff, lastHit );
	//}

	//std::pair<int, Vector3> 
	//   getMinVisibleAngle1
	//   ( 
	//       Chunk           *pChunk, 
	//       Vector3         const &sPolePos,  
	//       Vector3         const &lastPos, 
	//       bool            reverse 
	//   )
	//{
	//	BW_GUARD;

	//	// cast up to SHADOW_RAYS_IN_CALCULATION rays, from horiz over a 90
	//	// deg range, finding the 1st one that doesn't collide (do it twice
	//	// starting from diff directions each time)

	//	// We've gotta add a lil to the z, or when checking along chunk boundaries
	//	// we don't hit the terrain block

	//	Vector3 polePos = sPolePos + Vector3(0.0f, 0.1f, 0.0001f);

	//	// apply the chunk's xform to polePos to get it's world pos
	//	polePos = pChunk->transform().applyPoint( polePos );

	//	// adding an offset to avoid errors in the collisions at chunk seams
	//	polePos.z += SHADOW_RAY_ERROR_OFFSET;

	//	Vector3 lastHit = lastPos;
	//	float xdiff = reverse ? polePos.x - lastPos.x : lastPos.x - polePos.x;
	//	float ydiff = lastPos.y - polePos.y;
	//	bool valid = lastPos.y > polePos.y;
	//	float lastAngle = atan2f( ydiff, xdiff );
	//	if( lastAngle < 0 )
	//		lastAngle += MATH_PI / 2;

	//	for (int i = 0; i < SHADOW_MAX_CALC_RAYS; i++)
	//	{			
	//		// sun travels along the x axis (thus around the z axis)

	//		// Make a ray that points the in correct direction
	//		float angle = MATH_PI * (i / float( SHADOW_MAX_CALC_RAY_VAL ) );

	//		//if( valid && angle < lastAngle )
	//		//	continue;

	//		float nRays = max(1.f, (SHADOW_MAX_CALC_RAY_VAL - i) / s_raySubdivision);
	//		float dAngle = ((MATH_PI/2.f) - angle) / nRays;
	//		float dDispl = (MAX_TERRAIN_SHADOW_RANGE / nRays) * s_rayDisplacement;

	//		Vector3 start( polePos );
	//		ShadowObstacleCatcher soc;			

	//		for (int r=0; r < nRays; r++)
	//		{
	//			angle = (MATH_PI * (i/float(SHADOW_MAX_CALC_RAY_VAL))) + (r * dAngle);
	//			Vector3 ray ( cosf(angle), sinf(angle), 0.f );

	//			if (reverse)
	//				ray.x *= -1;				

	//			if (s_debugShadowRays)
	//			{
	//				s_lines.push_back( start );
	//				s_lineEnds.push_back( start + (ray * dDispl) );
	//			}

	//			soc.direction(ray);

	//			Terrain::TerrainHeightMap2::optimiseCollisionAlongZAxis( true );

	//			ChunkManager::instance().cameraSpace()->collide( 
	//				start,
	//				start + (ray * dDispl),
	//				soc );

	//			Terrain::TerrainHeightMap2::optimiseCollisionAlongZAxis( false );

	//			if (soc.hit())
	//			{
	//				lastHit = start + soc.dist() * ray;
	//				break;
	//			}

	//			//always start slightly back along the line, so as
	//			//not to create a miniscule break in the line (i mean curve)
	//			start += (ray * dDispl * 0.999f);
	//		}			

	//		if (!soc.hit())
	//		{
	//			return std::make_pair( i, lastHit );
	//		}

	//		WorldManager::instance().fiberPause();
	//	}

	//	return std::make_pair( 0xff, lastHit );
	//}
}


// -----------------------------------------------------------------------------
// Section: ChunkTerrain
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkTerrain::ChunkTerrain() :
	ChunkItem( WANTS_DRAW ),
	bb_( Vector3::zero(), Vector3::zero() )
{
	sceneObject_.flags().isCastingStaticShadow( true );
	sceneObject_.flags().isTerrain( true );

	BW_GUARD;
	static bool firstTime = true;
	if (firstTime)
	{
		MF_WATCH( "Render/Terrain/Use water culling",
				use_water_culling, Watcher::WT_READ_WRITE,
				"Perform extra culling for the terrain blocks in the water scene. " );
		MF_WATCH
        ( 
            "Render/Terrain Shadows/debug shadow rays",
			s_debugShadowRays,
			Watcher::WT_READ_WRITE,
			"Enabled, this draws a visual representation of the rays used to "
			"calculate the shadows on the terrain." 
        );
		MF_WATCH
        ( 
            "Render/Terrain Shadows/coll type", 
            s_collType,
			Watcher::WT_READ_WRITE,
			"0 - 500m straight line rays, 1 - <500m curved rays" 
        );
		MF_WATCH
        ( 
            "Render/Terrain Shadows/ray subdv",
			s_raySubdivision,
			Watcher::WT_READ_WRITE,
			"If using curved rays, this amount changes the subdivision.  A "
			"lower value indicates more rays will be used."
        );
		MF_WATCH
        ( 
            "Render/Terrain Shadows/ray displ",
			s_rayDisplacement,
			Watcher::WT_READ_WRITE,
			"This value changes the lenght of the ray after the subdivisions. "
			"Use it to tweak how long the resultant rays are." 
        );

		firstTime=false;
	}
}


/**
 *	Destructor.
 */
ChunkTerrain::~ChunkTerrain()
{
	// Note, we explicitly dereference pointers so ensuing destruction can be
	// profiled.
	PROFILER_SCOPED( ChunkTerrain_destruct );
	block_				= NULL;
}

/**
 *	Draw method
 */
void ChunkTerrain::draw( Moo::DrawContext& drawContext )
{
	// draw should not be called upon an item that is
	// not in the world.
	MF_ASSERT(pChunk_);

	BW_GUARD_PROFILER( ChunkTerrain_draw );

	static DogWatch drawWatch( "ChunkTerrain" );
	ScopedDogWatch watcher( drawWatch );

    Terrain::TerrainHoleMap &thm = block_->holeMap();
	if (!thm.allHoles())
	{
        Matrix world( pChunk_->transform() );

		if (Moo::rc().reflectionScene() && use_water_culling)
		{
			float height = WaterSceneRenderer::currentScene()->waterHeight();

			BoundingBox bounds( this->bb() );

			//TODO: check to see if this transform is needed at all to get the height range info..
			bounds.transformBy( world );

			bool onPlane = bounds.minBounds().y == height || bounds.maxBounds().y == height;
			bool underWater = ( WaterSceneRenderer::currentCamHeight() < height);

			bool minAbovePlane = bounds.minBounds().y > height;
			bool maxAbovePlane = bounds.maxBounds().y > height;

			bool abovePlane = minAbovePlane && maxAbovePlane;
			bool belowPlane = !minAbovePlane && !maxAbovePlane;

			if (!onPlane)
			{
				if (underWater)
				{
					if (Moo::rc().mirroredTransform() && abovePlane) //reflection
						return;
					if (!Moo::rc().mirroredTransform() && belowPlane) //refraction
						return;
				}
				else
				{
					if (Moo::rc().mirroredTransform() && belowPlane) //reflection
						return;
					if (!Moo::rc().mirroredTransform() && abovePlane) //refraction
						return;
				}
			}
		}
		// Add the terrain block to the terrain's drawlist.
		Terrain::BaseTerrainRenderer::instance()->addBlock( block_.getObject(), world );
	}

	// draw debug lines
	if (!s_lines.empty())
	{
		for (uint32 i=0; i<s_lines.size(); i++)
		{
			LineHelper::instance().drawLine(s_lines[i], s_lineEnds[i], i % 2 ? 0xffffffff : 0xffff8000 );
		}
		LineHelper::instance().purge();
		s_lines.clear();
		s_lineEnds.clear();
	}
}

uint32 ChunkTerrain::typeFlags() const
{
	BW_GUARD;
	if (Terrain::BaseTerrainRenderer::instance()->version() ==
		Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		return ChunkItemBase::TYPE_DEPTH_ONLY;
	}
	else
	{
		return 0;
	}
}


/**
 *	This method calculates the block's bounding box, and sets into bb_.
 */
void ChunkTerrain::calculateBB()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( block_ )
	{
		return;
	}

    bb_ = block_->boundingBox();

	// regenerate the collision scene since our bb is different now
	if (pChunk_ != NULL)
	{
		pChunk_->updateBoundingBoxes(this);
	}
}

namespace 
{
	uint32 charToHex( char c )
	{
		uint32 res = 0;
		if (c >= '0' && c <= '9')
		{
			res = uint32( c - '0' );
		}
		else if (c >= 'a' && c <= 'f')
		{
			res = uint32( c - 'a' ) + 0xa;
		}

		return res;
	}
}


/**
 *	Convert from chunk identifier to grid positions.
 *	For the opposite, see ChunkFormat::outsideChunkIdentifier().
 *	@param chunkID the outside chunk ID *without* an 'o' on the end.
 *		eg. "xxxxzzzz".
 *	@param x set to the x grid position of outside chunk.
 *	@param z set to the z grid position of outside chunk.
 *	@return true if the chunk ID could be converted, false otherwise.
 */
bool ChunkTerrain::outsideChunkIDToGrid( const BW::string& chunkID, 
											int32& x, int32& z )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( chunkID.size() == 8 )
	{
		return false;
	}

	x = int32( charToHex( chunkID[0] ) ) << 12;
	x |= int32( charToHex( chunkID[1] ) ) << 8;
	x |= int32( charToHex( chunkID[2] ) ) << 4;
	x |= int32( charToHex( chunkID[3] ) );

	if (x & 0x8000)
		x |= 0xffff0000;

	z = int32( charToHex( chunkID[4] ) ) << 12;
	z |= int32( charToHex( chunkID[5] ) ) << 8;
	z |= int32( charToHex( chunkID[6] ) ) << 4;
	z |= int32( charToHex( chunkID[7] ) );

	if (z & 0x8000)
		z |= 0xffff0000;

	return true;
}


#if EDITOR_ENABLED

/*static*/
Terrain::BaseTerrainBlockPtr ChunkTerrain::loadTerrainBlockFromChunk( Chunk * pChunk )
{
	if (!pChunk)
	{
		return NULL;
	}

	DataSectionPtr pChunkDS = BWResource::openSection(
						pChunk->mapping()->path() + pChunk->identifier() + ".chunk");
	if (!pChunkDS)
	{
		return NULL;
	}

	DataSectionPtr pTerrainDS = pChunkDS->openSection( "terrain" );
	if (!pTerrainDS)
	{
		return NULL;
	}

	BW::string resName = pTerrainDS->readString( "resource" );	

	Terrain::TerrainSettingsPtr terrainSettings = pChunk->space()->terrainSettings();

	Terrain::BaseTerrainBlockPtr pBlock =
		Terrain::BaseTerrainBlock::loadBlock( 
					pChunk->mapping()->path() + resName,
					pChunk->transform(),
					ChunkManager::instance().cameraTrans().applyToOrigin(),
					terrainSettings,
					NULL );

	return pBlock;
}

#endif


bool ChunkTerrain::doingBackgroundTask() const
{
	return block_ ? block_->doingBackgroundTask() : false;
}


/**
 *  This is called to recalculate the self-shadowing of the terrain.
 *
 *	@param canExitEarly		If true then early exit due to a change in the 
 *							working chunk is allowed.  If false then the shadow 
 *							must be fully calculated.
 */
bool ChunkTerrain::calculateShadows(
	Terrain::HorizonShadowMap::HorizonShadowImage& image,
	uint32 width, uint32 height, const BW::vector<float>& heights )
{
	BW_GUARD;

#ifdef EDITOR_ENABLED
	static bool isRunFirstTime = true;
	static float centerWeight = 2.f;

	const Chunk* pChunk = chunk();
	const float blockSize = this->block_->blockSize();

	MF_ASSERT( pChunk->isBound() );

	struct ShadeTexel {
		float shade_all;
		float shade_trr;
	};

	// 1 pixel width margin for PCF
	const int32 shadowMapW = image.width() + 2;  // X
	const int32 shadowMapH = image.height() + 2; // Z
	Moo::Image<ShadeTexel> shadeMap( shadowMapW, shadowMapH );

	const Chunk* cc = ChunkManager::instance().cameraChunk();
	const ChunkSpacePtr cs = ChunkManager::instance().cameraSpace();
	if (cc && cs && cc->isOutsideChunk()) 
	{
		// calculate light's direction and position.
		EnviroMinder& enviro = cs->enviro();
		Vector3 lightDir = enviro.timeOfDay()->lighting().mainLightDir();
		lightDir.normalise();

		for (int32 i = 0; i < shadowMapW; ++i) 
		{
			for (int32 j = 0; j < shadowMapH; ++j) 
			{
				//float shiftX = 1.f / (float) (2 * hsm.width());
				//float shiftZ = 1.f / (float) (2 * hsm.height());
				//float chunkX = (float) (i - 1) / (float) hsm.width() + shiftX;
				//float chunkZ = (float) (j - 1) / (float) hsm.height() + shiftZ;

				//// current point in the coordinate chunk [0 ... 100]
				//chunkX *= Terrain::BLOCK_SIZE_METRES;
				//chunkZ *= Terrain::BLOCK_SIZE_METRES; 

				float chunkX = blockSize * (float)(i - 1) / 
					(float)(image.width() - 1);
				float chunkZ = blockSize * (float)(j - 1) / 
					(float)(image.height() - 1);

				// shading for the current point
				std::pair<bool, bool> ret = 
					getShade( pChunk, chunkX, chunkZ, lightDir ); 

				// write result to shadeMap
				ShadeTexel st = shadeMap.get( i, j );
				st.shade_all = ret.first ? 1.f : 0.f;
				st.shade_trr = ret.second ? 1.f : 0.f;
				shadeMap.set( i, j, st );

				// multitasking
				if (BgTaskManager::shouldAbortTask())
				{
					return false;
				}
			}
		}

		// 3x3 PCF around (x, z) point.
		for (uint32 x = 0; x < image.width(); ++x)
		{
			for (uint32 z = 0; z < image.height(); ++z)
			{
				float shade_all = 0.f;
				float shade_trr = 0.f; 
				for (uint32 i = x; i <= x + 2; ++i)
				{
					for (uint32 j = z; j <= z + 2; ++j)
					{
						float weight = 1.f;
						if (i == (x + 1) && j == (z + 1))
							weight = centerWeight;

						ShadeTexel st = shadeMap.get( i, j );
						shade_all += weight * st.shade_all;
						shade_trr += weight * st.shade_trr;
					}
				}
				const float mlt = 1.f / (8.f + centerWeight);
				shade_all *= mlt;
				shade_trr *= mlt;

				// write result to image
				Terrain::HorizonShadowPixel res;
				res.east = (uint16) ((float) _UI16_MAX * shade_all);
				res.west = (uint16) ((float) _UI16_MAX * shade_trr);

				image.set( x, z, res );
			}
		}
	}
#endif // #ifdef EDITOR_ENABLED

	return true;
}


/**
 *	This method loads this terrain block.
 */
bool ChunkTerrain::load( DataSectionPtr pSection, 
						Chunk * pChunk, BW::string* errorString )
{	
	BW_GUARD;
	PROFILE_FILE_SCOPED( TerrainLoad );
	if (!Terrain::BaseTerrainRenderer::instance()->version())
	{// terrain setting initialisation failure
		return false;
	}

	BW::string resName = pSection->readString( "resource" );	

	Terrain::TerrainSettingsPtr terrainSettings = pChunk->space()->terrainSettings();

	// Allocate the terrainblock.
	block_ = Terrain::BaseTerrainBlock::loadBlock( 
					pChunk->mapping()->path() + resName,
					pChunk->transform(),
					ChunkManager::instance().cameraTrans().applyToOrigin(),
					terrainSettings,
					errorString );

	if (!block_)
	{	
		if ( errorString )
		{
			*errorString = "Could not load terrain block " + resName + 
				" Reason: " + *errorString + "\n";
			return false;
		}
	}

	this->calculateBB();

#if FMOD_SUPPORT
	if (SoundManager::instance().terrainOcclusionEnabled())
	{
		soundOccluder_.construct(	this->block_->heightMap(),
									pChunk->space()->gridSize(),
									terrainSettings->directSoundOcclusion(),
									terrainSettings->reverbSoundOcclusion() );

		soundOccluder_.update( pChunk->transform() );
	}
#endif

	return true;
}


/*
 *	This method gets called when the chunk is bound, this is a good place
 *	to create our umbra objects.
 */
void ChunkTerrain::syncInit()
{
	BW_GUARD;	
#if UMBRA_ENABLE
	// Delete any old umbra draw item
	bw_safe_delete(pUmbraDrawItem_);

	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init( this, bb(), this->pChunk_->transform(), pChunk_->getUmbraCell() );
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
#endif
}


class HeightMapCollisionSceneProvider : public CollisionSceneProvider
{
	Terrain::TerrainHeightMap::ImageType heightMap_;
	Terrain::TerrainHoleMap::ImageType holeMap_;
	Vector3 origin_;
	float spacingX_;
	float spacingZ_;
	uint32 blockWidth_;
	uint32 blockHeight_;
	uint32 xVisibleOffset_;
	uint32 zVisibleOffset_;
	float holeMapWidth_;
	float holeMapHeight_;
	bool allHoles_;
	bool noHoles_;
	float blockSize_;

public:
	HeightMapCollisionSceneProvider( const Terrain::TerrainHeightMap& heightMap,
		const Terrain::TerrainHoleMap& holeMap,
		const Vector3& origin )
		: heightMap_( heightMap.image() ),
		holeMap_( holeMap.image() ),
		origin_( origin ),
		spacingX_( heightMap.spacingX() ),
		spacingZ_( heightMap.spacingX() ),
		blockWidth_( heightMap.blocksWidth() ),
		blockHeight_( heightMap.blocksHeight() ),
		xVisibleOffset_( heightMap.xVisibleOffset() ),
		zVisibleOffset_( heightMap.zVisibleOffset() ),
		holeMapWidth_( (float)holeMap.width() ),
		holeMapHeight_( (float)holeMap.height() ),
		allHoles_( holeMap.allHoles() ),
		noHoles_( holeMap.noHoles() ),
		blockSize_( heightMap.blockSize() )
	{}

	virtual void appendCollisionTriangleList( BW::vector<Vector3>& triangleList ) const
	{
		if (!allHoles_)
		{
			size_t size = triangleList.size();

			triangleList.resize( triangleList.size() + blockHeight_ * blockWidth_ * 6 );

			float* f = (float*)&triangleList[ size ];
			size_t holes = 0;

			for (uint32 z = 0; z < blockHeight_; ++z)
			{
				float localBottomZ = z * spacingZ_;
				float localTopZ = localBottomZ + spacingZ_;
				float bottomZ = origin_.z + localBottomZ;
				float topZ = bottomZ + spacingZ_;
				const Terrain::TerrainHeightMap::ImageType::PixelType* row = heightMap_.getRow( z + zVisibleOffset_ ) + xVisibleOffset_;
				const Terrain::TerrainHeightMap::ImageType::PixelType* rowNext = heightMap_.getRow( z + 1 + zVisibleOffset_ ) + xVisibleOffset_;
				float localLeftX = 0.f;
				float leftX = origin_.x;

				for (uint32 x = 0; x < blockWidth_; ++x)
				{
					bool hole = false;

					if (!noHoles_)
					{
						hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
							int32( localBottomZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
							int32( localBottomZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
							int32( localTopZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
							int32( localTopZ / blockSize_ * holeMapHeight_ ));
					}

					if (hole)
					{
						++holes;
					}
					else
					{
						if (( x + z ) % 2)
						{
							*f++ = leftX;
							*f++ = *rowNext;
							*f++ = topZ;
							*f++ = leftX + spacingX_;
							*f++ = row[1];
							*f++ = bottomZ;
							*f++ = leftX;
							*f++ = *row;
							*f++ = bottomZ;
							*f++ = leftX;
							*f++ = *rowNext;
							*f++ = topZ;
							*f++ = leftX + spacingX_;
							*f++ = rowNext[1];
							*f++ = topZ;
							*f++ = leftX + spacingX_;
							*f++ = row[1];
							*f++ = bottomZ;
						}
						else
						{
							*f++ = leftX;
							*f++ = *row;
							*f++ = bottomZ;
							*f++ = leftX;
							*f++ = *rowNext;
							*f++ = topZ;
							*f++ = leftX + spacingX_;
							*f++ = rowNext[1];
							*f++ = topZ;
							*f++ = leftX;
							*f++ = *row;
							*f++ = bottomZ;
							*f++ = leftX + spacingX_;
							*f++ = rowNext[1];
							*f++ = topZ;
							*f++ = leftX + spacingX_;
							*f++ = row[1];
							*f++ = bottomZ;
						}
					}

					++row;
					++rowNext;
					leftX += spacingX_;
					localLeftX += spacingX_;
				}
			}

			triangleList.resize( triangleList.size() - holes * 6 );
		}
	}

	virtual size_t collisionTriangleCount() const
	{
		if (allHoles_)
		{
			return 0;
		}

		if (noHoles_)
		{
			return blockHeight_ * blockWidth_ * 2;
		}

		size_t holes = 0;

		for (uint32 z = 0; z < blockHeight_; ++z)
		{
			float localBottomZ = z * spacingZ_;
			float localTopZ = localBottomZ + spacingZ_;
			float localLeftX = 0.f;

			for (uint32 x = 0; x < blockWidth_; ++x)
			{
				bool hole = false;

				if (!noHoles_)
				{
					hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
						int32( localBottomZ / blockSize_ * holeMapHeight_ ));

					hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
						int32( localBottomZ / blockSize_ * holeMapHeight_ ));

					hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
						int32( localTopZ / blockSize_ * holeMapHeight_ ));

					hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
						int32( localTopZ / blockSize_ * holeMapHeight_ ));
				}

				if (hole)
				{
					++holes;
				}

				localLeftX += spacingX_;
			}
		}

		return blockHeight_ * blockWidth_ * 2 - holes * 2;
	}

	virtual void feed( CollisionSceneConsumer* consumer ) const
	{
		if (!allHoles_)
		{
			for (uint32 z = 0; z < blockHeight_ && !consumer->stopped(); ++z)
			{
				float localBottomZ = z * spacingZ_;
				float localTopZ = localBottomZ + spacingZ_;
				float bottomZ = origin_.z + localBottomZ;
				float topZ = bottomZ + spacingZ_;
				const Terrain::TerrainHeightMap::ImageType::PixelType* row = heightMap_.getRow( z + zVisibleOffset_ ) + xVisibleOffset_;
				const Terrain::TerrainHeightMap::ImageType::PixelType* rowNext = heightMap_.getRow( z + 1 + zVisibleOffset_ ) + xVisibleOffset_;
				float localLeftX = 0.f;
				float leftX = origin_.x;
				Vector3 v;

				for (uint32 x = 0; x < blockWidth_; ++x)
				{
					bool hole = false;

					if (!noHoles_)
					{
						hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
							int32( localBottomZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
							int32( localBottomZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( localLeftX / blockSize_ * holeMapWidth_ ),
							int32( localTopZ / blockSize_ * holeMapHeight_ ));

						hole |= holeMap_.get( int32( ( localLeftX + spacingX_ ) / blockSize_ * holeMapWidth_ ),
							int32( localTopZ / blockSize_ * holeMapHeight_ ));
					}

					if (!hole)
					{
						if (( x + z ) % 2)
						{
							v.x = leftX;
							v.y = *rowNext;
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = row[1];
							v.z = bottomZ;
							consumer->consume( v );
							v.x = leftX;
							v.y = *row;
							v.z = bottomZ;
							consumer->consume( v );
							v.x = leftX;
							v.y = *rowNext;
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = rowNext[1];
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = row[1];
							v.z = bottomZ;
							consumer->consume( v );
						}
						else
						{
							v.x = leftX;
							v.y = *row;
							v.z = bottomZ;
							consumer->consume( v );
							v.x = leftX;
							v.y = *rowNext;
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = rowNext[1];
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX;
							v.y = *row;
							v.z = bottomZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = rowNext[1];
							v.z = topZ;
							consumer->consume( v );
							v.x = leftX + spacingX_;
							v.y = row[1];
							v.z = bottomZ;
							consumer->consume( v );
						}
					}

					++row;
					++rowNext;
					leftX += spacingX_;
					localLeftX += spacingX_;
				}
			}
		}
	}
};


CollisionSceneProviderPtr ChunkTerrain::getCollisionSceneProvider( Chunk* ) const
{
	Terrain::TerrainHoleMapHolder thmh( &block()->holeMap(), true );

	return new HeightMapCollisionSceneProvider( block()->heightMap(), block()->holeMap(),
		chunk()->transform().applyToOrigin() );
}


/**
 *	Called when we are put in or taken out of a chunk
 */
void ChunkTerrain::toss( Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk_ != NULL)
	{
		ChunkTerrainCache::instance( *pChunk_ ).pTerrain( NULL );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		ChunkTerrainCache::instance( *pChunk ).pTerrain( this );
	}
}

bool ChunkTerrain::addYBounds( BoundingBox& bb ) const
{
	bb.addYBounds( this->bb().minBounds().y );
	bb.addYBounds( this->bb().maxBounds().y );
	return true;
}

#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( ChunkTerrain, terrain, 0 )


// -----------------------------------------------------------------------------
// Section: ChunkTerrainCache
// -----------------------------------------------------------------------------

BW_END_NAMESPACE

#include "chunk_terrain_obstacle.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ChunkTerrainCache::ChunkTerrainCache( Chunk & chunk ) :
	pChunk_( &chunk ),
	pTerrain_( NULL ),
	pObstacle_( NULL )
{
}

/**
 *	Destructor
 */
ChunkTerrainCache::~ChunkTerrainCache()
{
}


/**
 *	This method is called when our chunk is focussed. We add ourselves to the
 *	chunk space's obstacles at that point.
 */
int ChunkTerrainCache::focus()
{
	BW_GUARD;
	if (!pTerrain_ || !pObstacle_) return 0;

	const Vector3 & mb = pObstacle_->bb_.minBounds();
	const Vector3 & Mb = pObstacle_->bb_.maxBounds();

/*
	// figure out the border
	HullBorder	border;
	for (int i = 0; i < 6; i++)
	{
		// calculate the normal and d of this plane of the bb
		int ax = i >> 1;

		Vector3 normal( 0, 0, 0 );
		normal[ ax ] = (i&1) ? -1.f : 1.f;;

		float d = (i&1) ? -Mb[ ax ] : mb[ ax ];

		// now apply the transform to the plane
		Vector3 ndtr = pObstacle_->transform_.applyPoint( normal * d );
		Vector3 ntr = pObstacle_->transform_.applyVector( normal );
		border.push_back( PlaneEq( ntr, ntr.dotProduct( ndtr ) ) );
	}
*/

	// we assume that we'll be in only one column
	Vector3 midPt = pObstacle_->transform_.applyPoint( (mb + Mb) / 2.f );

	ChunkSpace::Column * pCol = pChunk_->space()->column( midPt );
	MF_ASSERT_DEV( pCol );

	// ok, just add the obstacle then
	if( pCol )
		pCol->addObstacle( *pObstacle_ );

	//dprintf( "ChunkTerrainCache::focus: "
	//	"Adding hull of terrain (%f,%f,%f)-(%f,%f,%f)\n",
	//	mb[0],mb[1],mb[2], Mb[0],Mb[1],Mb[2] );

	// which counts for just one
	return 1;
}


/**
 *	This method sets the terrain pointer
 */
void ChunkTerrainCache::pTerrain( ChunkTerrain * pT )
{
	BW_GUARD;
	if (pT != pTerrain_)
	{
		if (pObstacle_)
		{
			// flag column as stale first
			const Vector3 & mb = pObstacle_->bb_.minBounds();
			const Vector3 & Mb = pObstacle_->bb_.maxBounds();
			Vector3 midPt = pObstacle_->transform_.applyPoint( (mb + Mb) / 2.f );
			ChunkSpace::Column * pCol = pChunk_->space()->column( midPt, false );
			if (pCol != NULL) pCol->stale();

			pObstacle_ = NULL;
		}

		pTerrain_ = pT;

		if (pTerrain_ != NULL)
		{
            // Completely flat terrain will not work with the collision system.
            // In this case offset the y coordinates a little.
            if (pTerrain_->bb_.minBounds().y == pTerrain_->bb_.maxBounds().y)
            {
                pTerrain_->bb_.addYBounds(pTerrain_->bb_.minBounds().y + 1.0f);
            }
			pObstacle_ = new ChunkTerrainObstacle( *pTerrain_->block_,
				pChunk_->transform(), &pTerrain_->bb_, pT );

			if (pChunk_->focussed()) this->focus();
		}
	}
}


// -----------------------------------------------------------------------------
// Section: Static initialisers
// -----------------------------------------------------------------------------

/// Static cache instance accessor initialiser
ChunkCache::Instance<ChunkTerrainCache> ChunkTerrainCache::instance;



// -----------------------------------------------------------------------------
// Section: TerrainFinder
// -----------------------------------------------------------------------------

BW_END_NAMESPACE
#include "chunk_manager.hpp"
BW_BEGIN_NAMESPACE

/**
 *	This class implements the TerrainFinder interface. Its purpose is to be an
 *	object that Moo can use to access the terrain. It is implemented like this
 *	so that other libraries do not need to know about the Chunk library.
 */
class TerrainFinderInstance : public Terrain::TerrainFinder
{
public:
	TerrainFinderInstance()
	{
		Terrain::BaseTerrainBlock::setTerrainFinder( *this );
	}

	virtual Terrain::TerrainFinder::Details findOutsideBlock( const Vector3 & pos )
	{
		BW_GUARD;
		Terrain::TerrainFinder::Details details;

		// TODO: At the moment, assuming the space the camera is in.
		ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();

		//find the chunk
		if (pSpace)
		{
			ChunkSpace::Column* pColumn = pSpace->column( pos, false );

			if ( pColumn != NULL )
			{
				Chunk * pChunk = pColumn->pOutsideChunk();

				if (pChunk != NULL)
				{
					//find the terrain block
					ChunkTerrain * pChunkTerrain =
						ChunkTerrainCache::instance( *pChunk ).pTerrain();

					if (pChunkTerrain != NULL)
					{
						details.pBlock_ = pChunkTerrain->block().getObject();
						details.pInvMatrix_ = &pChunk->transformInverse();
						details.pMatrix_ = &pChunk->transform();
					}
				}
			}
		}

		return details;
	}
};


static TerrainFinderInstance s_terrainFinder;

BW_END_NAMESPACE

// chunk_terrain.cpp
