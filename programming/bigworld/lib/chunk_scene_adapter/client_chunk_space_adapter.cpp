#include "pch.hpp"

#include "client_chunk_space_adapter.hpp"

#ifdef EDITOR_ENABLED
#include <gizmo/tool_manager.hpp>
#endif

#include "math/convex_hull.hpp"

#include "space/space_manager.hpp"
#include "scene/scene.hpp"
#include "scene/scene_intersect_context.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/draw_operation.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_light.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_water.hpp"
#include "chunk/chunk_flare.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_umbra.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_tree.hpp"
#include "chunk/umbra_draw_item.hpp"
#include "duplo/chunk_attachment.hpp"
#include "duplo/chunk_embodiment.hpp"
#include "duplo/chunk_dynamic_obstacle.hpp"
#include "duplo/py_model_obstacle.hpp"
#include "moo/light_container.hpp"
#include "moo/render_target.hpp"
#include "moo/texture_streaming_scene_view.hpp"
#include "romp/chunk_omni_light_embodiment.hpp"
#include "romp/chunk_spot_light_embodiment.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"

BW_BEGIN_NAMESPACE

namespace ChunkItemOps
{

class ChunkItemScriptObjectQuery : 
	public IScriptObjectQueryOperationTypeHandler
{
public:
	virtual ScriptObject doGetScriptObject( const SceneObject & object )
	{
		ChunkItem* pItem = object.getAs<ChunkItem>();
		MF_ASSERT( pItem );

		if (pItem && (pItem->userFlags() & 
			ChunkObstacle::DYNAMIC_OBSTACLE_USER_FLAG))
		{
			ChunkEmbodiment * pCE = static_cast<ChunkEmbodiment*>(pItem);
			PyObject * pObj = &*(pCE->pPyObject());
			return ScriptObject(pObj);
		}
		else
		{
			return ScriptObject();
		}
	}
};

class ChunkItemDrawOperation : 
	public IDrawOperationTypeHandler
{
public:
	virtual void doDraw( const SceneDrawContext & drawContext,
		const SceneObject * pObjects, size_t count )
	{
		const Matrix& lodCameraView = drawContext.lodCameraView();
		Matrix lodInvView;
		lodInvView.invert(lodCameraView);

		ClientSpacePtr cameraSpace = DeprecatedSpaceHelpers::cameraSpace();
		MF_ASSERT( cameraSpace != NULL );
#if SPEEDTREE_SUPPORT
		speedtree::SpeedTreeRenderer::beginFrame( &cameraSpace->enviro(),
			drawContext.drawContext().renderingPassType(),
			lodInvView);
#endif
		for (size_t i = 0; i < count; ++i)
		{
			ChunkItem* pItem = pObjects[i].getAs<ChunkItem>();
			MF_ASSERT( pItem );

			if (!pItem->chunk())
			{
				continue;
			}

			Moo::rc().world( pItem->chunk()->transform() );

			pItem->draw( drawContext.drawContext() );
		}
#if SPEEDTREE_SUPPORT
		speedtree::SpeedTreeRenderer::endFrame();
#endif
	}
};

class ChunkItemTextureStreamingOperation : 
	public Moo::ITextureStreamingObjectOperationTypeHandler
{
public:
	virtual bool registerTextureUsage( 
		const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		ChunkItem* pItem = object.getAs<ChunkItem>();
		MF_ASSERT( pItem );

		// TODO: Ideally this pattern would be re-implemented
		// using a visitor pattern available on ChunkItem.
		// No such framework is available yet though.
		{
			ChunkModel* pModel = 
				dynamic_cast< ChunkModel* >( pItem );
			if (pModel)
			{
				updateModelBounds( pModel, usageGroup );
				pModel->generateTextureUsage( usageGroup );
				return true;
			}
		}
		{
			ChunkTree* pTree = 
				dynamic_cast< ChunkTree* >( pItem );
			if (pTree)
			{
				updateTreeBounds( pTree, usageGroup );
				pTree->generateTextureUsage( usageGroup );
				return true;
			}
		}
		{
			ChunkAttachment* pAttachment = 
				dynamic_cast< ChunkAttachment* >( pItem );
			if (pAttachment)
			{
				if (!updateAttachmentBounds( pAttachment, usageGroup ))
				{
					// No valid texture usages found
					return false;
				}
				pAttachment->generateTextureUsage( usageGroup );
				return true;
			}
		}
		{
			ChunkDynamicObstacle* pObstacle = 
				dynamic_cast< ChunkDynamicObstacle* >( pItem );
			if (pObstacle)
			{
				updateObstacleBounds( pObstacle, usageGroup );
				pObstacle->pObstacle()->superModel()->generateTextureUsage( usageGroup );
				return true;
			}
		}

		return false;
	}

	virtual bool updateTextureUsage( 
		const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		ChunkItem* pItem = object.getAs<ChunkItem>();
		MF_ASSERT( pItem );

		{
			ChunkAttachment* pAttachment = 
				dynamic_cast< ChunkAttachment* >( pItem );
			if (pAttachment)
			{
				return updateAttachmentBounds( pAttachment, usageGroup );
			}
		}
		{
			ChunkDynamicObstacle* pObstacle = 
				dynamic_cast< ChunkDynamicObstacle* >( pItem );
			if (pObstacle)
			{
				updateObstacleBounds( pObstacle, usageGroup );
				return true;
			}
		}

		return false;
	}

	void updateModelBounds( ChunkModel* pModel,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		Matrix worldXform = pModel->worldTransform();
		BoundingBox worldBB = pModel->localBB();
		worldBB.transformBy( worldXform );

		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( worldBB ) );
	}

	void updateTreeBounds( ChunkTree* pTree,
		Moo::ModelTextureUsageGroup& usageGroup  )
	{
		Matrix worldXform = pTree->chunk()->transform();
		worldXform.preMultiply( pTree->transform() );
		BoundingBox worldBB;
		pTree->localBB( worldBB );
		worldBB.transformBy( worldXform );

		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( worldBB ) );
	}

	bool updateAttachmentBounds( ChunkAttachment* pAttachment,
		Moo::ModelTextureUsageGroup& usageGroup  )
	{
		Matrix worldXform = pAttachment->worldTransform();
		BoundingBox textureUsageBB;
		pAttachment->pAttachment()->textureUsageWorldBox( textureUsageBB, worldXform );

		if (textureUsageBB.insideOut())
		{
			return false;
		}

		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( textureUsageBB ) );

		return true;
	}

	void updateObstacleBounds( ChunkDynamicObstacle* pObstacle,
		Moo::ModelTextureUsageGroup& usageGroup  )
	{
		Matrix worldXform = pObstacle->worldTransform();
		BoundingBox textureUsageBB;
		pObstacle->worldBoundingBox( textureUsageBB, worldXform );

		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( textureUsageBB ) );
	}
};

void registerHandlers( Scene& scene )
{
	ScriptObjectQueryOperation* pScriptObjectOp =
		scene.getObjectOperation<ScriptObjectQueryOperation>();
	MF_ASSERT( pScriptObjectOp != NULL );

	// Must add a handler for chunk item to fetch the script objects as well.
	// This is because collisions just return ChunkItems, and thus callers 
	// only really ever understand chunk items.
	pScriptObjectOp->addHandler<ChunkItem, ChunkItemScriptObjectQuery>();

	DrawOperation* pDrawOp = 
		scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );

	pDrawOp->addHandler<ChunkItem, ChunkItemDrawOperation>();

	Moo::TextureStreamingObjectOperation* pTexStreamOp = 
		scene.getObjectOperation<Moo::TextureStreamingObjectOperation>();
	MF_ASSERT( pTexStreamOp != NULL );

	pTexStreamOp->addHandler<ChunkItem, ChunkItemTextureStreamingOperation>();
}

} // End namespace ChunkItemOps

ClientChunkSpaceFactory::ClientChunkSpaceFactory()
{

}

ClientSpace * ClientChunkSpaceFactory::createSpace( SpaceID spaceID ) const
{
	ChunkSpace* pChunkSpace = new ChunkSpace( spaceID );
	return new ClientChunkSpaceAdapter( pChunkSpace );
}


IEntityEmbodimentPtr ClientChunkSpaceFactory::createEntityEmbodiment( 
	const ScriptObject& object ) const
{
	ChunkEmbodimentPtr pNewEmbodiment;
	int ret = Script::setData( object.get(), pNewEmbodiment, "Entity.model" );

	if (ret == 0 && pNewEmbodiment)
	{
		return new ClientChunkSpaceAdapter::EntityEmbodiment( pNewEmbodiment );
	}
	else
	{
		return NULL;
	}
}


IOmniLightEmbodiment * ClientChunkSpaceFactory::createOmniLightEmbodiment(
	const PyOmniLight & pyOmniLight ) const
{
	return new ChunkOmniLightEmbodiment( pyOmniLight );
}


ISpotLightEmbodiment * ClientChunkSpaceFactory::createSpotLightEmbodiment(
	const PySpotLight & pySpotLight ) const
{
	return new ChunkSpotLightEmbodiment( pySpotLight );
}


ChunkSpacePtr ClientChunkSpaceAdapter::getChunkSpace( const ClientSpacePtr& space )
{
	if (space)
	{
		return ChunkManager::instance().space( space->id(), false );
	}
	else
	{
		return NULL;
	}
}


ClientSpacePtr ClientChunkSpaceAdapter::getSpace( const ChunkSpacePtr& space )
{
	if (space)
	{
		return SpaceManager::instance().space( space->id() );
	}
	else
	{
		return NULL;
	}
}

void ClientChunkSpaceAdapter::init()
{
	// If no factory has been set, let's default to chunk spaces.
	if (SpaceManager::instance().factory() == NULL)
	{
		SpaceManager::instance().factory( new ClientChunkSpaceFactory() );
		RompCollider::setDefaultCreationFuction( 
			&ChunkRompCollider::create );
	}
}


ClientChunkSpaceAdapter::ClientChunkSpaceAdapter( ChunkSpace * pChunkSpace ) :	
	ClientSpace( pChunkSpace->id() ),
	provider_( pChunkSpace ),
	pChunkSpace_( pChunkSpace )
{
	EntityEmbodiment::registerHandlers( this->scene() );
	ChunkItemOps::registerHandlers( this->scene() );
	MF_ASSERT( pChunkSpace_ );
	pChunkSpace_->setChunkSceneProvider( &provider_ );
	scene().addProvider( &provider_ );
}


ClientChunkSpaceAdapter::~ClientChunkSpaceAdapter()
{
	pChunkSpace_->setChunkSceneProvider( NULL );
	scene().removeProvider( &provider_ );
}

bool ClientChunkSpaceAdapter::doAddMapping( GeometryMappingID mappingID,
				  Matrix& transform, const BW::string & path, 
				  const SmartPointer< DataSection >& pSettings )
{
	return pChunkSpace_->addMapping( 
		*(SpaceEntryID*)&mappingID, 
		transform, path, pSettings ) != NULL;
}


void ClientChunkSpaceAdapter::doDelMapping( GeometryMappingID mappingID )
{
	return pChunkSpace_->delMapping( *(SpaceEntryID*)&mappingID );
}


float ClientChunkSpaceAdapter::doGetLoadStatus( float distance ) const
{
	if (distance < 0.f)
	{
		distance = Moo::rc().camera().farPlane();
	}
	float chunkDist = ChunkManager::instance().closestUnloadedChunk( pChunkSpace_ );
	float streamDist = Terrain::BasicTerrainLodController::instance().closestUnstreamedBlock();

	float dist = std::min( chunkDist, streamDist );

	return std::min( 1.f, dist / distance );
}


AABB ClientChunkSpaceAdapter::doGetBounds() const
{
	return pChunkSpace_->gridBounds();
}


Vector3 ClientChunkSpaceAdapter::doClampToBounds( const Vector3& position ) const
{
	Vector3 result = position;

	result.x = Math::clamp( pChunkSpace_->minCoordX() + 0.05f,
		result.x,
		pChunkSpace_->maxCoordX() - 0.05f );
	result.z = Math::clamp( pChunkSpace_->minCoordZ() + 0.05f,
		result.z,
		pChunkSpace_->maxCoordZ() - 0.05f );

	return result;
}


float ClientChunkSpaceAdapter::doCollide( const Vector3 & start, 
	const Vector3 & end,
	CollisionCallback & cc ) const
{
	return pChunkSpace_->collide( start, end, cc );
}


float ClientChunkSpaceAdapter::doCollide( const WorldTriangle & triangle, 
	const Vector3 & translation, 
	CollisionCallback & cc ) const
{
	return pChunkSpace_->collide( triangle, translation, cc );
}


float ClientChunkSpaceAdapter::doFindTerrainHeight( const Vector3 & position ) const
{
	ClientSpacePtr pSpace;
	ChunkSpace::Column * csCol;
	Chunk * oCh;
	ChunkTerrain * pTer;

	if ((csCol = pChunkSpace_->column( position, false )) != NULL &&
		(oCh = csCol->pOutsideChunk()) != NULL &&
		(pTer = ChunkTerrainCache::instance( *oCh ).pTerrain()) != NULL)
	{
		const Matrix & trInv = oCh->transformInverse();
		Vector3 terPos = trInv.applyPoint( position );
		float terrainHeight = pTer->block()->heightAt( terPos[0], terPos[2] ) -
			trInv.applyToOrigin()[1];
		return terrainHeight;
	}
	else
	{
		return FLT_MIN;
	}
}


void ClientChunkSpaceAdapter::doClear()
{
	return ChunkManager::instance().clearSpace( pChunkSpace_.get() );
}

EnviroMinder & ClientChunkSpaceAdapter::doEnviro()
{
	return pChunkSpace_->enviro();
}


void ClientChunkSpaceAdapter::doTick( float dTime )
{
	// Do nothing. Chunks tick via ChunkManager::tick.
}


void ClientChunkSpaceAdapter::doUpdateAnimations( float /* dTime */ )
{
	pChunkSpace_->updateAnimations();
	pChunkSpace_->enviro().updateAnimations();
}


const Moo::DirectionalLightPtr & ClientChunkSpaceAdapter::doGetSunLight() const
{
	return pChunkSpace_->sunLight();
}


const Moo::Colour & ClientChunkSpaceAdapter::doGetAmbientLight() const
{
	return pChunkSpace_->ambientLight();
}

const Moo::LightContainerPtr & ClientChunkSpaceAdapter::doGetLights() const
{
	return pChunkSpace_->lights();
}


bool ClientChunkSpaceAdapter::doGetLightsInArea( const AABB& worldBB, 
	Moo::LightContainerPtr* allLights )
{
	Chunk * pChunk = pChunkSpace_->findChunkFromPoint( worldBB.centre() );

	if (pChunk == NULL)
	{
		return false;
	}

	ChunkLightCache & clc = ChunkLightCache::instance( *pChunk );
	
	clc.update();
	if (allLights)
	{
		*allLights = clc.pAllLights();
	}

	return true;
}


ClientChunkSpaceAdapter::SceneProvider::SceneProvider(ChunkSpace* pChunkSpace) : 
	pChunkSpace_( pChunkSpace )
{

}


bool ClientChunkSpaceAdapter::SceneProvider::setClosestPortalState( 
	const Vector3 & point,
	bool isPermissive, WorldTriangle::Flags collisionFlags )
{
	return pChunkSpace_->setClosestPortalState( 
		point, isPermissive, collisionFlags );
}


class ClientChunkSpaceAdapter::SceneProvider::Detail
{
public:
#if UMBRA_ENABLE
	template <typename IntersectionTest>
	static uint32 intersectUmbraCasters( const SceneProvider* pThis,
		IntersectionTest& test,
		const SceneIntersectContext& context,
		const ConvexHull& hull, 
		IntersectionSet & intersection )
	{
		BW_GUARD;
		const BW::vector<Chunk*>& allChunk = 
			ChunkManager::instance().umbraChunkShadowCasters();

		uint32 totalItems = 0;
		BW::vector<Chunk*>::const_iterator chunkIt = allChunk.begin();
		for ( ; chunkIt != allChunk.end(); ++chunkIt )
		{
			Chunk& c = **chunkIt;

			if (!c.isBound() ) 
			{
				continue;
			}

			BoundingBox bb = c.visibilityBox();
			if (!hull.intersects( bb ))
			{
				continue;
			}

			totalItems += cullItems( test, c.shadowItems(), c.transform(), 
				hull, intersection );
		}
		return totalItems;
	}
#endif // UMBRA_ENABLE

	template <typename IntersectionTest>
	static uint32 intersect( const SceneProvider* pThis,
		IntersectionTest& test,
		const SceneIntersectContext& context,
		const ConvexHull& hull, 
		IntersectionSet & intersection )
	{
		GridChunkMap& gcm = pThis->pChunkSpace_->gridChunks();

		int min_x = pThis->pChunkSpace_->minGridX();
		int min_y = pThis->pChunkSpace_->minGridY();
		int max_x = pThis->pChunkSpace_->maxGridX();
		int max_y = pThis->pChunkSpace_->maxGridY();

		uint32 totalObjects = 0;
		for (int i = min_x; i <= max_x; ++i ) 
		{
			for (int j = min_y; j <= max_y; ++j ) 
			{
				BW::vector<Chunk*>& chunkList = gcm[ std::make_pair(i, j) ];

				for (size_t ci = 0; ci < chunkList.size(); ++ci ) 
				{
					Chunk& c = *chunkList[ci];

					if (!c.isBound() ) 
					{
						continue;
					}

					BoundingBox bb = c.visibilityBox();

					if (bb.insideOut())
					{
						continue;
					}

					if (!hull.intersects( bb ))
					{
						continue;
					}

					if (context.includeStaticObjects())
					{
						totalObjects += Detail::cullItems( test,
							c.selfItems(), c.transform(), 
							hull, intersection );
					}
					if (context.includeDynamicObjects())
					{
						totalObjects += Detail::cullItems( test,
							c.dynoItems(), c.transform(), 
							hull, intersection );
					}
				}
			}
		}

		return totalObjects;
	}

	template <typename IntersectionTest>
	static uint32 cullItems( IntersectionTest& test,
		const Chunk::Items& itemList, 
		const Matrix & chunkTransform, 
		const ConvexHull& hull, 
		IntersectionSet & intersection )
	{
		Chunk::Items::const_iterator itemIt = itemList.begin();

		uint32 total = 0;
		for ( ; itemIt != itemList.end(); ++itemIt )
		{
			ChunkItem* item = (*itemIt).get();

			if (test( item, hull ))
			{
				intersection.insert( item->sceneObject() );
				++total;
			}
		}
		return total;
	}
};

size_t ClientChunkSpaceAdapter::SceneProvider::intersect( 
	const SceneIntersectContext& context,
	const ConvexHull& hull, 
	IntersectionSet & intersection) const
{
	struct VisibleBoundsTest
	{
		bool operator()( ChunkItem* pItem, const ConvexHull& hull )
		{
			if (!pItem->wantsDraw())
			{
				return false;
			}

#if UMBRA_ENABLE
			UmbraDrawShadowItem* pShadowItem = pItem->pUmbraDrawShadowItem();
			if ( !pShadowItem || pShadowItem->isDynamic() )
			{
				return true;
			}
			else
			{
				BoundingBox bb = pShadowItem->bbox();
				bb.transformBy( pShadowItem->transform() );

				if( hull.intersects( bb ) ) 
				{
					return true;
				}
			}
#endif

			return false;
		}
	};

#if UMBRA_ENABLE
	if (context.onlyShadowCasters() && 
		ChunkManager::instance().umbra()->castShadowCullingEnabled())
	{
		VisibleBoundsTest test;
		return Detail::intersectUmbraCasters( this, test, context, hull, intersection );
	}
	else
#endif // UMBRA_ENABLE
	{
		VisibleBoundsTest test;
		return Detail::intersect( this, test, context, hull, intersection );
	}
}


void ClientChunkSpaceAdapter::SceneProvider::updateDivisionConfig( 
	Moo::IntBoundBox2& bounds, 
	float& divisionSize )
{
	divisionSize = pChunkSpace_->gridSize();
	bounds.max_.x = std::max( bounds.max_.x, pChunkSpace_->maxGridX() );
	bounds.max_.y = std::max( bounds.max_.y, pChunkSpace_->maxGridY() );
	bounds.min_.x = std::min( bounds.min_.x, pChunkSpace_->minGridX() );
	bounds.min_.y = std::min( bounds.min_.y, pChunkSpace_->minGridY() );
}


void ClientChunkSpaceAdapter::SceneProvider::addDivisionBounds( 
	const Moo::IntVector2& divisionCoords, 
	AABB& divisionBounds )
{
	std::pair<int, int> gridCoord(divisionCoords.x, divisionCoords.y);
	GridChunkMap& gcm = pChunkSpace_->gridChunks();
	BW::vector<Chunk*>& chunks = gcm[gridCoord];

	if (chunks.size() > 0 && chunks[0]->isBound())
	{
		divisionBounds.addBounds( chunks[0]->visibilityBox() );
	}
}


void * ClientChunkSpaceAdapter::SceneProvider::getView( 
	const SceneTypeSystem::RuntimeTypeID & viewTypeID )
{
	void * result = NULL;

	exposeView<IIntersectSceneViewProvider>(this, viewTypeID, result);
	exposeView<IPortalSceneViewProvider>(this, viewTypeID, result);
	exposeView<BW::Moo::ISemiDynamicShadowSceneViewProvider>(
		this, viewTypeID, result);

	return result;
}


//static
void ClientChunkSpaceAdapter::EntityEmbodiment::registerHandlers( Scene & scene )
{
	ScriptObjectQueryOperation* pScriptObjectOp =
		scene.getObjectOperation<ScriptObjectQueryOperation>();
	MF_ASSERT( pScriptObjectOp != NULL );

	pScriptObjectOp->addHandler<ClientChunkSpaceAdapter::EntityEmbodiment,
		ScriptObjectQuery<ClientChunkSpaceAdapter::EntityEmbodiment> >();
}


ClientChunkSpaceAdapter::EntityEmbodiment::EntityEmbodiment( 
	const ChunkEmbodimentPtr& object ) :
		IEntityEmbodiment( ScriptObject( object->pPyObject() ) ),
		embodiment_( object )
{

}


ClientChunkSpaceAdapter::EntityEmbodiment::~EntityEmbodiment()
{

}


void ClientChunkSpaceAdapter::EntityEmbodiment::doEnterSpace( 
	ClientSpacePtr pSpace, bool transient )
{
	embodiment_->enterSpace( getChunkSpace( pSpace ), transient );
}


void ClientChunkSpaceAdapter::EntityEmbodiment::doLeaveSpace( 
	bool transient )
{
	embodiment_->leaveSpace( transient );
}


void ClientChunkSpaceAdapter::EntityEmbodiment::doMove( float dTime )
{
	embodiment_->move( dTime );
}


void ClientChunkSpaceAdapter::EntityEmbodiment::doWorldTransform( const Matrix & transform )
{
	ChunkAttachment * pAttachment = 
		dynamic_cast< ChunkAttachment * >( embodiment_.get() );
	MF_ASSERT( pAttachment );
	pAttachment->setMatrix( transform );
}


const Matrix & ClientChunkSpaceAdapter::EntityEmbodiment::doWorldTransform() const
{
	return embodiment_->worldTransform();
}


const AABB& ClientChunkSpaceAdapter::EntityEmbodiment::doLocalBoundingBox() 
	const
{
	embodiment_->localBoundingBox( localBB_, true );
	return localBB_;
}


bool ClientChunkSpaceAdapter::EntityEmbodiment::doIsOutside() const
{
	return embodiment_->chunk() == NULL ||
		embodiment_->chunk()->isOutsideChunk();
}


bool ClientChunkSpaceAdapter::EntityEmbodiment::doIsRegionLoaded( Vector3 testPos, 
	float radius ) const
{
	// no idea of chunk, so not loaded
	if (embodiment_->chunk() == NULL)
	{
		return false;
	}

	if (embodiment_->chunk()->isOutsideChunk())
	{
		ChunkOverlappers& o = ChunkOverlappers::instance( 
			*(embodiment_->chunk()) );
		const ChunkOverlappers::Overlappers& overlappers = o.overlappers();
		
		// Have all overlappers loaded?
		if (!o.complete())
		{
			return false;
		}

		// Check each overlapper if it is ready
		for (uint i = 0; i < overlappers.size(); i++)
		{
			ChunkOverlapper * pOverlapper = &*overlappers[i];
			if (pOverlapper->focussed()) 
			{
				continue;
			}

			// near an unloaded overlapper, so not loaded
			if (!pOverlapper->bbReady() || 
				pOverlapper->bb().intersects( testPos, radius ))
			{
				return false;
			}
		}
	}

	return true;
}


void ClientChunkSpaceAdapter::EntityEmbodiment::doDraw( Moo::DrawContext & drawContext )
{
	return embodiment_->draw( drawContext );
}


BW_END_NAMESPACE

// chunk_space_adapter.cpp
