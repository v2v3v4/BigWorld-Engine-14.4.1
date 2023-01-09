#include "pch.hpp"

#include "umbra_config.hpp"

#if UMBRA_ENABLE

#include "umbra_chunk_item.hpp"
#include "moo/render_context.hpp"
#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "chunk_umbra.hpp"
#include "chunk_obstacle.hpp"
#include "moo/line_helper.hpp"
#include "moo/geometrics.hpp"

#include <ob/umbramodel.hpp>
#include <ob/umbraobject.hpp>


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );


BW_BEGIN_NAMESPACE

UmbraChunkItem::UmbraChunkItem()
{
}


UmbraChunkItem::~UmbraChunkItem()
{
}


/**
 *	This method draws the associated chunk item as part of the colour pass.
 *	@param pChunkContext the chunk that is currently set up 
 *	(i.e. the currently set lighting information)
 *	@return the Chunk that is set after the draw, this is in case the current
 *	chunk item sets up a different chunk
 */
/*virtual*/ Chunk* UmbraChunkItem::draw( Moo::DrawContext& drawContext, Chunk* pChunkContext )
{
	BW_GUARD;

	// Only draw if this chunk item has already been drawn
	if (pItem_->drawMark() != Chunk::s_nextMark_ &&
		pItem_->chunk() != NULL)
	{
		// Update the chunk
		pChunkContext = updateChunk( drawContext, pChunkContext );

		MF_ASSERT( pItem_->wantsDraw() );
		pItem_->draw( drawContext );

		// Set the draw mark
		pItem_->drawMark( Chunk::s_nextMark_ );
	}
	return pChunkContext;
}


/**
 *	This method draws the associated chunk item as part of the depth pass.
 *	It assumes that RenderContext::drawDepth has already been set up
 *	@param pChunkContext the chunk that is currently set up 
 *	(i.e. the currently set lighting information)
 *	@return the Chunk that is set after the draw, this is in case the current
 *	chunk item sets up a different chunk
 */
/*virtual*/ Chunk* UmbraChunkItem::drawDepth( Moo::DrawContext& drawContext, Chunk* pChunkContext )
{
	BW_GUARD;

	// Check if the depth mark has been set up, if not draw the
	// depth version of the item
	if (pItem_->depthMark() != Chunk::s_nextMark_ &&
		pItem_->chunk() != NULL)
	{
		// Update the chunk
		pChunkContext = updateChunk( drawContext, pChunkContext );

		MF_ASSERT( pItem_->wantsDraw() );
		pItem_->draw( drawContext );

		// Mark the item as being rendered in the depth pass
		pItem_->depthMark( Chunk::s_nextMark_ );

		// If the item does not know how to draw depth, mark it as drawn
		// in the colour pass as well
		if (!( pItem_->typeFlags() & ChunkItemBase::TYPE_DEPTH_ONLY ))
		{
			pItem_->drawMark( Chunk::s_nextMark_ );
		}
	}
	return pChunkContext;
}


/**
 *	This method inits the UmbraChunkItem
 *	It creates an umbra object based on the bounding box passed in
 *	@param pItem the chunk item to use
 *	@param bb the bounding box to use for visibility testing
 *	@param transform the transform of the bounding box
 *	@param pCell the umbra cell to place this item in
 */
void UmbraChunkItem::init( ChunkItem* pItem, const BoundingBox& bb, const Matrix& transform, Umbra::OB::Cell* pCell )
{
	BW_GUARD;

	UmbraModelProxyPtr pModel = UmbraModelProxy::getObbModel( bb.minBounds(), bb.maxBounds() );
	init( pItem, pModel, transform, pCell );
}


/**
 *	This method inits the UmbraChunkItem
 *	It creates an umbra object based on the vertices passed in
 *	@param pItem the chunk item to use
 *	@param pVertices the list of vertices to create the umbra object from
 *	@param nVertices the number of vertices in the list
 *	@param transform the transform of the bounding box
 *	@param pCell the umbra cell to place this item in
 */
void UmbraChunkItem::init( ChunkItem* pItem, const Vector3* pVertices, uint32 nVertices, const Matrix& transform, Umbra::OB::Cell* pCell )
{
	BW_GUARD;

	UmbraModelProxyPtr pModel = UmbraModelProxy::getObbModel( pVertices, nVertices );
	init( pItem, pModel, transform, pCell );
}


/**
 *	This method inits the UmbraChunkItem
 *	It creates an umbra object based on umbra model passed in
 *	@param pItem the chunk item to use
 *	@param pModel the model proxy to use when creating the umbra object
 *	@param transform the transform of the bounding box
 *	@param pCell the umbra cell to place this item in
 */
void UmbraChunkItem::init( ChunkItem* pItem, UmbraModelProxyPtr pModel, const Matrix& transform, Umbra::OB::Cell* pCell )
{
	BW_GUARD;

	init( pItem, UmbraObjectProxy::get( pModel ), transform, pCell );
}

/**
 *	This method inits the UmbraChunkItem
 *	It uses the passed in umbra object
 *	@param pItem the chunk item to use
 *	@param pObject the umbra ubject to use
 *	@param transform the transform of the bounding box
 *	@param pCell the umbra cell to place this item in
 */
void UmbraChunkItem::init( ChunkItem* pItem, UmbraObjectProxyPtr pObject, const Matrix& transform, Umbra::OB::Cell* pCell )
{
	pItem_ = pItem;

	pObject_ = pObject;
	pObject_->object()->setCell( pCell );
	pObject_->object()->setObjectToCellMatrix( (const Umbra::Matrix4x4&)transform );
	pObject_->object()->setUserPointer( (void*)this );
	pObject_->object()->setBitmask( ChunkUmbra::SCENE_OBJECT );
}


/**
 *	This method updates the current chunk we are in
 *	@param pChunkContext the chunk we want to move to
 *	@return the chunk that has been inited
 */
Chunk* UmbraChunkItem::updateChunk( Moo::DrawContext& drawContext, Chunk* pChunkContext )
{
	BW_GUARD;
	Chunk* pChunk = pItem_->chunk();
	if (pChunkContext != pChunk)
	{
		MF_ASSERT( pChunk != NULL );
		pChunk->drawCaches( drawContext );
		Moo::rc().world( pChunk->transform() );

		// Update the drawmark for the outside chunks as this may
		// be needed post-traversal. Indoor chunk marks are updated 
		// when drawn through the regular portal culling
		if (pChunk->isOutsideChunk())
		{
			pChunk->drawMark(Chunk::s_nextMark_ );
		}
	}
	return pChunk;
}


namespace 
{
	class TerrainObstacle : public CollisionCallback
	{
	public:
		TerrainObstacle() :
			dist_( -1 ),
			collided_( false )
		{
		}

		int operator() ( 
			const CollisionObstacle & obstacle,
			const WorldTriangle & triangle, 
			float dist 
		)
		{
			if ( triangle.flags() & TRIANGLE_TERRAIN )
			{
    			// Set the distance if it's the first collision, or if the distance is
    			// less than the previous collision.
				if (!collided_ || dist < dist_)
				{
					dist_     = dist;
					collided_ = true;
				}

				return COLLIDE_BEFORE;
			}

			return COLLIDE_ALL;
		}

		void TerrainObstacle::reset() 
		{
			dist_ = -1;
			collided_ = false;
		}

		float dist() const { return dist_; }
		bool collided() const { return collided_; }

	private:
		float dist_;
		bool collided_;
	};


	void drawBBox( const Vector3 & pos, const Matrix & transform )
	{
		float boxSize = 0.1f;
		Vector3 maxPoint = Vector3( pos.x + boxSize, pos.y + boxSize, pos.z + boxSize );
		Vector3 minPoint = Vector3( pos.x - boxSize, pos.y - boxSize, pos.z - boxSize );
		BoundingBox box  = BoundingBox( minPoint, maxPoint );

		LineHelper::instance().drawBoundingBox( box, transform, 0x00FF0000 );
	}
}

UmbraChunkItemShadowCaster::UmbraChunkItemShadowCaster()
	: pItem_( NULL )
	, distVisible_( 0.f )
	, isUpdated_( false )
{
}


UmbraChunkItemShadowCaster::~UmbraChunkItemShadowCaster()
{
	// This object may be destroyed after umbra has been destroyed.
	// This can happen for example when BGResourceRefsLoader
	// is still running when we exit. It holds smart pointers
	// to all the things it loaded, and is destroyed after
	// ChunkManager::fini() so umbra will have been destroyed.
	ChunkUmbra*	pUmbra = ChunkManager::instance().umbra();
	if (pUmbra)
	{
		pUmbra->delUpdateItem( this );
	}
}

/**
 * 
 */
Chunk* UmbraChunkItemShadowCaster::draw( Moo::DrawContext& drawContext, Chunk* pChunkContext )
{
	BW_GUARD;
	
	if ( pItem_->chunk() != NULL )
	{
		Chunk* pChunk = pItem_->chunk();

		// This object is added to the list of objects whose shadows we
		// potentially see.
		pChunk->addUmbraShadowCasterItem( pItem_ );

		if ( distVisible_ != ChunkManager::instance().umbra()->shadowDistanceVisible() )
		{
			distVisible_ = ChunkManager::instance().umbra()->shadowDistanceVisible();
			pObject_->object()->setCullDistance( 0.f, distVisible_ );
		}

		if ( ChunkManager::instance().umbra()->drawShadowBoxes() )
		{
			if ( !isUpdated_ )
			{
				LineHelper::instance().drawBoundingBox( shadowBB_, shadowTr_, 0x00FF0000 );
				LineHelper::instance().drawBoundingBox( objectBB_, objectTr_, 0x00FF0000 );
			}
			else
			{
				// Boxing shadows
				Moo::rc().push();
				Moo::rc().world( shadowTr_ );
				Moo::Material::setVertexColour();
				Geometrics::wireBox( shadowBB_, Moo::Colour( 1.0, 0.0, 0.0, 1.0 ) );
				Moo::rc().pop();

				// Boxing objects
				Moo::rc().push();
				Moo::rc().world( objectTr_ );
				Moo::Material::setVertexColour();
				Geometrics::wireBox( objectBB_, Moo::Colour( 0.0, 1.0, 0.0, 1.0 ) );
				Moo::rc().pop();
			}
		}
	}

	return pChunkContext;
}

/**
 * 
 */
Chunk* UmbraChunkItemShadowCaster::drawDepth( Moo::DrawContext& drawContext, Chunk* pChunkContext )
{
	BW_GUARD;
	return this->draw( drawContext, pChunkContext );
}

BW_END_NAMESPACE

#include "terrain/base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"
#include "chunk_terrain.hpp"

BW_BEGIN_NAMESPACE

/*
namespace 
{
	float getLandHeight( float x, float z )
	{
		ChunkSpace* pSpace = ChunkManager::instance().cameraSpace().get();
		if ( pSpace )
		{
			Chunk *pChunk = pSpace->findChunkFromPoint( Vector3( x, 0, z ) );
			if ( pChunk )
			{
				if ( !pChunk->isBound() )
				{
					NOTICE_MSG( "Chunk unbounded !!!!!" );
				}

				float fHeight;
				const Matrix &tr = pChunk->transform();
				x -= tr._41;
				z -= tr._43;
				if ( pChunk->getTerrainHeight( x, z, fHeight ) )
					return fHeight;
			}
		}

		return -1.f;
	}

	float collideTerrain( ChunkItem* pItem, Vector3 startPos, const Vector3 & direction )
	{
		Chunk* pChunk  = pItem->chunk();
		Vector3 endPos = startPos + ( direction * 300.0f );
		Vector3 rayStartPos = startPos;
		ChunkTerrain* pTerrain = pChunk->getTerrain();

		if ( pTerrain )
		{
			for ( int i = 0; i < 32; ++i )
			{
				Vector3 middle = ( startPos + endPos ) * 0.5f;
				float height = getLandHeight( middle.x, middle.z );
				
				if ( height == -1.0f )
					break;

				if ( middle.y < height )
					endPos = middle;
				else
					startPos = middle;
			}

			Vector3 collisionPos = ( startPos + endPos ) * 0.5f;
			return ( rayStartPos - collisionPos ).length();
		}

		return -1.0f;
	}

	bool isChunkExist( const Vector3 & point )
	{
		ChunkSpace* pSpace = ChunkManager::instance().cameraSpace().get();
		if ( pSpace )
		{
			return pSpace->findChunkFromPoint( Vector3( point.x, 0, point.z ) ) != NULL;
		}
		return false;
	}
}
*/

void UmbraChunkItemShadowCaster::updateTransform( const Matrix& transform )
{
	BW_GUARD;
	
	ChunkSpace* pSpace   = ChunkManager::instance().cameraSpace().get();
	Chunk*      pChunk   = pItem_->chunk();
	BoundingBox bb       = objectBB_;
	float       distance = 0.f;

	if ( pSpace )
	{
		Vector3 sunDirection = pSpace->sunLight()->worldDirection();
		float   sunDirPitch  = sunDirection.pitch();
		float   sunDirYaw    = sunDirection.yaw();

		// Calculate projection shadow boundingbox size.
		{
			Matrix scaleMt = Matrix::identity;
			Matrix rotate0 = Matrix::identity;
			Matrix rotate1 = Matrix::identity;

			scaleMt.setScale( transform[0].length(), transform[1].length(), transform[2].length() );
			rotate0.setRotate( transform.yaw(), transform.pitch(), transform.roll() );
			rotate1.lookAt( Vector3(0, 0, 0), sunDirection, Vector3( 0.f, 1.f, 0.f ) );

			scaleMt *= rotate0;
			scaleMt *= rotate1;

			bb.transformBy( scaleMt );
		}

		// Shadow BoundingBox transform
		{
			Matrix mt = Matrix::identity;
			mt.setRotate( sunDirYaw, sunDirPitch, 0 );
			mt.translation( transform[3] );

			shadowTr_ = mt;
		}

		// Calculate size shadow boundingbox.
		Vector3 positions[4];
		Vector3 maxBounds = bb.maxBounds();
		Vector3 minBounds = bb.minBounds();

		Matrix msbt = Matrix::identity;
		msbt.translation( transform[3] );

		//positions[0] = Vector3( maxBounds.x,              maxBounds.y,               maxBounds.z - bb.depth() );
		//positions[1] = Vector3( maxBounds.x - bb.width(), maxBounds.y,               maxBounds.z - bb.depth() );
		//positions[2] = Vector3( maxBounds.x,              maxBounds.y - bb.height(), maxBounds.z - bb.depth() );
		//positions[3] = Vector3( maxBounds.x - bb.width(), maxBounds.y - bb.height(), maxBounds.z - bb.depth() );

		positions[0] = Vector3( maxBounds.x,              maxBounds.y, maxBounds.z );
		positions[1] = Vector3( maxBounds.x - bb.width(), maxBounds.y, maxBounds.z );
		positions[2] = Vector3( maxBounds.x,              maxBounds.y, maxBounds.z - bb.depth() );
		positions[3] = Vector3( maxBounds.x - bb.width(), maxBounds.y, maxBounds.z - bb.depth() );

		positions[0] = shadowTr_.applyPoint( positions[0] );
		positions[1] = shadowTr_.applyPoint( positions[1] );
		positions[2] = shadowTr_.applyPoint( positions[2] );
		positions[3] = shadowTr_.applyPoint( positions[3] );

		if ( pChunk )
		{
			ChunkTerrain* pTerrain = pChunk->getTerrain();

			if ( pTerrain )
			{
				for ( int i = 0; i < 4; ++i )
				{
					TerrainObstacle to;
					float dst = 0.0f;
					
					dst = pSpace->collide( positions[i], positions[i] + ( sunDirection * 800.f ), to );
					dst = to.dist();
										
					if ( dst > distance )
						distance = dst;
				}

				if ( distance == 0.f )
				{
					float maxCoordX = pSpace->maxCoordX() - pSpace->gridSize();
					float minCoordX = pSpace->minCoordX() + pSpace->gridSize();
					float maxCoordZ = pSpace->maxCoordZ() - pSpace->gridSize();
					float minCoordZ = pSpace->minCoordZ() + pSpace->gridSize();

					int isExist = 4;
					for ( int i = 0; i < 4; ++i )
					{
						Vector3 pos = positions[i] + (sunDirection * pSpace->gridSize());
						
						if ( pos.x > maxCoordX || pos.x < minCoordX || pos.z > maxCoordZ || pos.z < minCoordZ )
						{
							--isExist;
						}
					}

					isUpdated_  = true;
					maxBounds.z = pSpace->gridSize();

					if ( isExist > 0 && !isDynamicObject_ )
					{
						isUpdated_ = false;
						ChunkManager::instance().umbra()->addUpdateItem( this );
					}
				}
				else
				{
					isUpdated_  = true;
					maxBounds.z = distance;
				}
			}
		}

		bb.setBounds( minBounds, maxBounds );
	}
	
	shadowBB_ = bb;
	objectTr_ = transform;
	distVisible_ = ChunkManager::instance().umbra()->shadowDistanceVisible();
	
	UmbraModelProxy* pModel = UmbraModelProxy::getObbModel( bb.minBounds(), bb.maxBounds() );

	if ( pObject_ )
	{
		pObject_->pModelProxy( pModel );
	}
	else
	{
		UmbraObjectProxy* pObject = UmbraObjectProxy::get( pModel );

		pObject_ = pObject;
		
		if ( pItem_->chunk() )
		{
			pObject_->object()->setCell( pItem_->chunk()->getUmbraCell() );
		}

		pObject_->object()->setUserPointer( (void*)this );
		pObject_->object()->set( Umbra::OB::Object::OCCLUDER, false );
		pObject_->object()->setRenderCost( Umbra::OB::Object::CHEAP );
		pObject_->object()->setCullDistance( 0.f, distVisible_ );
		pObject_->object()->setBitmask( ChunkUmbra::SHADOW_OBJECT );
	}

	pObject_->object()->setObjectToCellMatrix( (const Umbra::Matrix4x4&)shadowTr_ );
}


void UmbraChunkItemShadowCaster::init( ChunkItem* pItem, const BoundingBox& box, const Matrix& transform, Umbra::OB::Cell* pCell, bool isDynamicObject )
{
	BW_GUARD;
	
	objectBB_ = box;
	pItem_    = pItem;
	isDynamicObject_ = isDynamicObject;

	this->updateTransform( transform );

	pObject_->object()->setCell( pCell );
}

void UmbraChunkItemShadowCaster::update()
{
	updateTransform( objectTr_ );
}

BW_END_NAMESPACE

#endif // UMBRA_ENABLE
