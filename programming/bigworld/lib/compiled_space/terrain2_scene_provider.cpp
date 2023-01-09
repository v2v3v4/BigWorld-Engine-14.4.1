#include "pch.hpp"
#include "terrain2_scene_provider.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/intersection_set.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/draw_operation.hpp"
#include "scene/light_scene_view.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/scene_intersect_context.hpp"
#include "scene/spatial_query_operation.hpp"

#include "terrain/terrain_settings.hpp"
#if defined( EDITOR_ENABLED )
#include "terrain/terrain2/editor_terrain_block2.hpp"
#endif
#include "terrain/terrain2/terrain_block2.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain_ray_collision_callback.hpp"
#include "terrain/terrain_prism_collision_callback.hpp"
#include "terrain/terrain2/terrain_renderer2.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"
#include "math/convex_hull.hpp"
#include "physics2/sweep_helpers.hpp"

// NSS TODO: remove these dependencies
#include "chunk/chunk_manager.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"


namespace BW {

namespace CompiledSpace {

class Terrain2SceneProvider::Detail
{
public:
	static bool xzIntersect( const AABB & bb, const Vector3 & p )
	{
		return p.x >= bb.minBounds().x && p.x <= bb.maxBounds().x &&
			p.z >= bb.minBounds().z && p.z <= bb.maxBounds().z;
	}

	static const BlockInstance * findBlockInstance( const Vector3& position, 
		const Terrain2SceneProvider& provider )
	{
		uint32 gridIndex = 
			Terrain2Types::blockIndexFromPoint( position, *provider.pHeader_ );

		// Check if we have a block to handle this query
		if (gridIndex >= provider.blockGridLookup_.size())
		{
			return NULL;
		}

		uint32 index = provider.blockGridLookup_[gridIndex];
		if (index == Terrain2Types::INVALID_BLOCK_REFERENCE)
		{
			return NULL;
		}

		const BlockInstance& instance = provider.blockInstances_[index];
		if (!instance.pBlock_)
		{
			return NULL;
		}
		MF_ASSERT( xzIntersect( instance.worldBB_, position ) );

		return &instance;
	}
};

// ----------------------------------------------------------------------------
Terrain2SceneProvider::Terrain2SceneProvider() : 
	pReader_( NULL ),
	pStream_( NULL ),
	pHeader_( NULL ),
	numBlocksLoaded_( 0 )
{
	// Doing this here to make sure renderer is initialised on the main thread
	// rather than in the background loader thread (needs to load device 
	// resources which can't happen on a BG thread).
	MF_VERIFY( Terrain::TerrainRenderer2::init() );
}


// ----------------------------------------------------------------------------
Terrain2SceneProvider::~Terrain2SceneProvider()
{

}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::doLoadFromSpace( ClientSpace * pSpace,
								 BinaryFormat& reader,
								 const DataSectionPtr& pSpaceSettings,
								 const Matrix& transform, 
								 const StringTable& strings )
{
	PROFILER_SCOPED( Terrain2SceneProvider_doLoadFromSpace );

	typedef Terrain2Types::Header Header;
	typedef Terrain2Types::BlockData BlockData;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		Terrain2Types::FORMAT_MAGIC,
		Terrain2Types::FORMAT_VERSION,
		"Terrain2SceneProvider" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map '%s' into memory.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	if (!pStream_->read( pHeader_ ) ||
		!pStream_->read( blockData_ ) ||
		!pStream_->read( blockGridLookup_ ) ||
		!pStream_->read( sceneOctree_ ) ||
		!pStream_->read( sceneOctreeContents_.storage() ) ||
		!pStream_->read( sceneContents_ ))
	{
		ASSET_MSG( "Terrain2SceneProvider in '%s' has incomplete data.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	// TODO: pack and load settings from binary data
	pTerrainSettings_ = new Terrain::TerrainSettings();
	if (!pTerrainSettings_->init( pHeader_->blockSize_, 
		pSpaceSettings->openSection( "terrain" ) ) )
	{
		ASSET_MSG( "Could not read terrain settings from space.settings\n" );
		this->unload();
		return false;
	}

	if (pTerrainSettings_->version() != 200)
	{
		ASSET_MSG( "Incorrect terrain version in space.settings\n" );
		this->unload();
		return false;
	}
	
	// Look for cdata files inside the the same folder as the space binary
	BW::string spaceDir = BWUtil::getFilePath( pReader_->resourceID() );

	if (!this->loadBlockInstances( spaceDir.c_str(), transform, strings ))
	{
		this->unload();
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::doBind()
{
	if (isValid())
	{
		space().scene().addProvider( this );
	}

	return true;
}


// ----------------------------------------------------------------------------
void Terrain2SceneProvider::doUnload()
{
	space().scene().removeProvider( this );

	this->freeBlockInstances();
	blockData_.reset();
	pTerrainSettings_ = NULL;

	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::isValid() const
{
	return blockData_.valid() && pHeader_ != NULL;
}


// ----------------------------------------------------------------------------
float Terrain2SceneProvider::blockSize() const
{
	MF_ASSERT( this->isValid() );
	return pHeader_->blockSize_;
}


// ----------------------------------------------------------------------------
size_t Terrain2SceneProvider::numBlocks() const
{
	MF_ASSERT( this->isValid() );
	return blockData_.size();
}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::loadBlockInstances( const char* spaceDir,
											   const Matrix& transform,
											   const StringTable& strings )
{
	PROFILER_SCOPED( Terrain2SceneProvider_loadBlockInstances );

	MF_ASSERT( blockInstances_.empty() );
	blockInstances_.resize( this->numBlocks() );

	const Vector3 cameraPosition =
		ChunkManager::instance().cameraTrans().applyToOrigin();

	Terrain::ResourceStreamType oldStreamType =
		Terrain::ResourceBase::defaultStreamType();
	Terrain::ResourceBase::defaultStreamType( Terrain::RST_Syncronous );

	SceneObject objTemplate;
	objTemplate.type<BlockInstance>();
	objTemplate.flags().isDynamic( false );
	objTemplate.flags().isTerrain( true );
	objTemplate.flags().isCastingStaticShadow( true );

	float blockSize = this->blockSize();
	for (size_t blockIdx = 0; blockIdx < this->numBlocks(); ++blockIdx)
	{
		Terrain2Types::BlockData& data = blockData_[blockIdx];
		BlockInstance& instance = blockInstances_[blockIdx];
		instance.transform_.setTranslate(
			float(data.x_) * blockSize, 0.f, float(data.z_) * blockSize );

		instance.inverseTransform_.invert( instance.transform_ );

		const char* localResID = strings.entry( data.resourceID_ );
		if (!localResID)
		{
			ERROR_MSG( "Terrain2SceneProvider: invalid string table index %d.\n",
				data.resourceID_ );
			continue;
		}

		char buffer[BW_MAX_PATH];
		StringBuilder resourceID(buffer, ARRAY_SIZE(buffer) );
		resourceID.append( spaceDir ); // Has trailing slash
		resourceID.append( localResID );


        instance.pBlock_ = new Terrain::TERRAINBLOCK2( pTerrainSettings_ );
		bool success = instance.pBlock_->load( resourceID.string(),
			instance.transform_, 
			cameraPosition);
		++numBlocksLoaded_;

		if (!success)
		{
			ERROR_MSG( "Terrain2SceneProvider: could not load '%s'.\n", resourceID.string() );
			instance.pBlock_ = NULL;
			continue;
		}

		instance.localBB_ = instance.pBlock_->boundingBox();

		// Completely flat terrain will not work with the collision system.
		// In this case offset the y coordinates a little.
		if (instance.localBB_.height() == 0.f)
		{
			instance.localBB_.addYBounds( instance.localBB_.minBounds().y + 1.0f );
		}

		instance.worldBB_ = instance.localBB_;
		instance.worldBB_.transformBy( instance.transform_ );

		instance.sceneObject_ = objTemplate;
		instance.sceneObject_.handle( &instance );
	}

	Terrain::ResourceBase::defaultStreamType( oldStreamType );

	return true;
}


// ----------------------------------------------------------------------------
void Terrain2SceneProvider::freeBlockInstances()
{
	for (size_t blockIdx = 0; blockIdx < blockInstances_.size(); ++blockIdx)
	{
		BlockInstance& instance = blockInstances_[blockIdx];

		instance.pBlock_ = NULL;
	}

	blockInstances_.clear();
	numBlocksLoaded_ = 0;
}


// ----------------------------------------------------------------------------
float Terrain2SceneProvider::percentLoaded() const
{
	if (blockInstances_.empty())
	{
		return 1.f;
	}
	else
	{
		// Determine the number of blocks we have processed and asked to load
		size_t numBlocksLoaded = numBlocksLoaded_;

		// If we've asked everything to load, check on each block's progress
		if (numBlocksLoaded_ == blockInstances_.size())
		{
			for (size_t blockIdx = 0; blockIdx < blockInstances_.size(); ++blockIdx)
			{
				const BlockInstance& instance = blockInstances_[blockIdx];

				if (instance.pBlock_ && !instance.pBlock_->doingBackgroundTask())
				{
					++numBlocksLoaded;
				}
			}
		}

		// Divide by 2 because each block has two stages, 1 requesting, 2 loading
		return (float)numBlocksLoaded / ((float)blockInstances_.size() * 2.0f);
	}
}


// ----------------------------------------------------------------------------
void * Terrain2SceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID )
{
	void * result = NULL;

	exposeView<ICollisionSceneViewProvider>( this, viewTypeID, result );
	exposeView<IIntersectSceneViewProvider>( this, viewTypeID, result );
	exposeView<Terrain::ITerrainSceneViewProvider>( this, viewTypeID, result );

	return result;
}


// ----------------------------------------------------------------------------
size_t Terrain2SceneProvider::intersect( const SceneIntersectContext& context,
	const ConvexHull& hull, 
	IntersectionSet & intersection ) const
{
	// Make sure these objects are contextually interesting.
	if (!context.includeStaticObjects())
	{
		return 0;
	}

	MF_ASSERT( this->isValid() );

	uint32 numIntersectedObjects = 0;

	Octree::RuntimeNodeDataReferences references;
	sceneOctree_.intersect( hull, references );

	for (size_t i = 0; i < references.size(); ++i)
	{
		const DataSpan & span = sceneOctreeContents_[references[i]];

		// Potentially no bounds check required here since 
		// intention is to have max one block per node. 
		const size_t numContents = span.range();
		if (numContents == 1)
		{
			const BlockInstance& instance = 
				blockInstances_[sceneContents_[span.first()]];
			
			intersection.insert( instance.sceneObject_ );
			++numIntersectedObjects;
		}
		else
		{
			for (size_t j = span.first(); j <= span.last(); ++j)
			{
				const BlockInstance & instance = 
					blockInstances_[sceneContents_[j]];

				if (hull.intersects( instance.worldBB_ ))
				{
					intersection.insert( instance.sceneObject_ );
					++numIntersectedObjects;
				}
			}
		}
	}

	return numIntersectedObjects;
}


namespace {

	class TerrainCollisionInterface : public CollisionInterface
	{
	public:
		TerrainCollisionInterface(
					const SceneObject & sceneObject,
					const Terrain::BaseTerrainBlock& block,
					const Matrix& transform, const Matrix& inverseTransform ) :
			sceneObject_( sceneObject ),
			block_( block ),
			transform_( transform ),
			inverseTransform_( inverseTransform )
		{
		}

		virtual bool collide( const Vector3 & source, const Vector3 & extent,
			CollisionState & cs ) const
		{
			Terrain::TerrainRayCollisionCallback toc(
				cs, transform_,
				inverseTransform_,
				block_, sceneObject_,
				source, extent );

			block_.collide( source, extent, &toc ); 

			return toc.finishedColliding();
		}

		virtual bool collide( const WorldTriangle & source, const Vector3 & extent,
			CollisionState & cs ) const
		{
			Terrain::TerrainPrismCollisionCallback toc(
				cs, transform_,
				inverseTransform_,
				block_, sceneObject_,
				source, extent );

			return block_.collide( source, extent, &toc );
		}

	private:
		const SceneObject & sceneObject_;
		const Terrain::BaseTerrainBlock & block_;
		Matrix transform_;
		Matrix inverseTransform_;
	};

} // anonymous namespace
// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::collide( const Vector3 & source,
									 const Vector3 & extent,
									 const SweepParams& sp,
									 CollisionState & cs ) const
{
	MF_ASSERT( this->isValid() );

	uint32 numIntersectedObjects = 0;

	Octree::RuntimeNodeDataReferences references;
	sceneOctree_.intersect( source, extent, references );

	for (size_t i = 0; i < references.size(); ++i)
	{
		const DataSpan & span = sceneOctreeContents_[references[i]];

		for (size_t j = span.first(); j <= span.last(); ++j)
		{
			const BlockInstance & instance = 
				blockInstances_[sceneContents_[j]];

			if (!instance.pBlock_)
			{
				continue;
			}

			TerrainCollisionInterface collisionInterface(
				instance.sceneObject_,
				*instance.pBlock_.get(),
				instance.transform_, instance.inverseTransform_ );

			if (SweepHelpers::collideIntoObject<Vector3>(
				source, sp, collisionInterface,
				instance.localBB_, instance.inverseTransform_,
				cs ))
			{
				return true;
			}
		}
	}

	return false;
}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::collide( const WorldTriangle & source,
									const Vector3 & extent,
									const SweepParams& sp,
									CollisionState & cs ) const
{
	MF_ASSERT( this->isValid() );

	uint32 numIntersectedObjects = 0;

	const AABB & shapeBB = sp.shapeBox();
	Octree::RuntimeNodeDataReferences references;
	sceneOctree_.intersect( shapeBB, references );

	for (size_t i = 0; i < references.size(); ++i)
	{
		const DataSpan & span = sceneOctreeContents_[references[i]];

		for (size_t j = span.first(); j <= span.last(); ++j)
		{
			const BlockInstance & instance = 
				blockInstances_[sceneContents_[j]];

			if (!instance.pBlock_)
			{
				continue;
			}

			TerrainCollisionInterface collisionInterface(
				instance.sceneObject_,
				*instance.pBlock_.get(),
				instance.transform_, instance.inverseTransform_ );

			if (SweepHelpers::collideIntoObject<WorldTriangle>(
				source, sp, collisionInterface,
				instance.localBB_, instance.inverseTransform_,
				cs ))
			{
				return true;
			}
		}
	}

	return false;
}


// ----------------------------------------------------------------------------
float Terrain2SceneProvider::findTerrainHeight(
	const Vector3 & position ) const
{
	float result = FLT_MAX;
	
	const BlockInstance * pInstance = Detail::findBlockInstance( position, *this );
	if (!pInstance)
	{
		return result;
	}

	Vector3 terPos = pInstance->inverseTransform_.applyPoint( position );
	result = pInstance->pBlock_->heightAt( terPos[0], terPos[2] ) -
		pInstance->inverseTransform_.applyToOrigin()[1];
	return result;
}


// ----------------------------------------------------------------------------
bool Terrain2SceneProvider::findTerrainBlock( Vector3 const & position,
		Terrain::TerrainFinder::Details & result )
{
	const BlockInstance * pInstance = Detail::findBlockInstance( position, *this );
	if (!pInstance)
	{
		return false;
	}
	
	result.pBlock_ = pInstance->pBlock_.get();
	result.pMatrix_ = &pInstance->transform_;
	result.pInvMatrix_ = &pInstance->inverseTransform_;
	return true;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class Terrain2SceneProvider::DrawHandler : public IDrawOperationTypeHandler
{
public:
	void doDraw( const SceneDrawContext & context,
		const SceneObject * pObjects, size_t count )
	{
		//TRACE_MSG( "Terrain2DrawOperation::doDraw: %d blocks.\n", count );
		for (size_t i = 0; i < count; ++i)
		{
			using namespace CompiledSpace;
			Terrain2SceneProvider::BlockInstance* pInst = 
				pObjects[i].getAs<Terrain2SceneProvider::BlockInstance>();

			Terrain::TerrainHoleMap &thm = pInst->pBlock_->holeMap();
			if (!thm.allHoles())
			{
				Terrain::BaseTerrainRenderer::instance()->addBlock(
					pInst->pBlock_.getObject(), pInst->transform_ );
			}
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class Terrain2SceneProvider::SpatialQueryHandler : 
	public ISpatialQueryOperationTypeHandler
{
public:
	virtual void getWorldTransform( const SceneObject& obj, 
		Matrix* xform )
	{
		BlockInstance* pItem = obj.getAs<BlockInstance>();
		MF_ASSERT( pItem );
		*xform = pItem->transform_;
	}

	virtual void getWorldVisibilityBoundingBox( 
		const SceneObject& obj, 
		AABB* bb )
	{
		BlockInstance* pItem = obj.getAs<BlockInstance>();
		MF_ASSERT( pItem );
		*bb = pItem->worldBB_;
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class Terrain2SceneProvider::TerrainFinder : public Terrain::TerrainFinder
{
public:
	static TerrainFinder s_terrainFinder;

	virtual Details findOutsideBlock( const Vector3 & pos )
	{
		BW_GUARD;
		Details details;

		// TODO: At the moment, assuming the space the camera is in.
		ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();

		if (pSpace)
		{
			Scene& scene = pSpace->scene();

			Terrain::TerrainSceneView* pView =
				scene.getView<Terrain::TerrainSceneView>();
			MF_ASSERT( pView != NULL );

			pView->findTerrainBlock( pos, details );
		}

		return details;
	}
};

//static
Terrain2SceneProvider::TerrainFinder
	Terrain2SceneProvider::TerrainFinder::s_terrainFinder;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void Terrain2SceneProvider::registerHandlers( Scene & scene )
{
	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<BlockInstance,
		Terrain2SceneProvider::DrawHandler>();

	SpatialQueryOperation * pSpatOp = 
		scene.getObjectOperation<SpatialQueryOperation>();
	MF_ASSERT( pSpatOp != NULL );
	pSpatOp->addHandler<BlockInstance,
		Terrain2SceneProvider::SpatialQueryHandler>();

	Terrain::BaseTerrainBlock::setTerrainFinder(
		Terrain2SceneProvider::TerrainFinder::s_terrainFinder );
}

} // namespace CompiledSpace
} // namespace BW
