#include "pch.hpp"
#include "chunk_dynamic_obstacle.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_cache.hpp"
#include "chunk/chunk.hpp"

#include "model/super_model.hpp"
#include "model/super_model_dye.hpp"

#include "moo/node.hpp"
#include "moo/render_context.hpp"

#include "cstdmf/debug.hpp"

#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/umbra_config.hpp"

#include "py_model_obstacle.hpp"

#if UMBRA_ENABLE
#include "chunk/umbra_chunk_item.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

int ChunkDynamicObstacle_token = 0;

namespace {

	void collectObstacles( SuperModel* pSuperModel,
		const Matrix& worldTransform,
		ChunkItem* pItem,
		BW::vector<ChunkObstaclePtr>& obstacles )
	{
		// extract and create all the obstacles
		int nModels = pSuperModel->nModels();
		obstacles.reserve( nModels );

		for (int i = 0; i < nModels; i++)
		{
			ModelPtr pModel = pSuperModel->topModel( i );
			// Mutually exclusive of pModel reloading, start
			pModel->beginRead();

			const BSPTree * pBSPTree = pModel->decompose();
			if (pBSPTree != NULL)
			{
				ChunkBSPObstacle* pObstacle = new ChunkBSPObstacle(
					pModel.get(), worldTransform,
					&pModel->boundingBox(), pItem );
				obstacles.push_back( pObstacle );
			}

			// Mutually exclusive of pModel reloading, end
			pModel->endRead();
		}
	}

} // anonymous namespace

// -----------------------------------------------------------------------------
// Section: ChunkDynamicObstacles declaration
// -----------------------------------------------------------------------------


class ChunkDynamicObstacles : public ChunkCache
{
public:
	ChunkDynamicObstacles( Chunk & chunk ) : pChunk_( &chunk ) { }
	~ChunkDynamicObstacles() { MF_ASSERT_DEV( cdos_.empty() ); }

	virtual int focus();

	void add( ChunkDynamicObstacle * cdo );
	void mod( ChunkDynamicObstacle * cdo, const Matrix & oldWorld );
	void del( ChunkDynamicObstaclePtr cdo );

	static Instance<ChunkDynamicObstacles> instance;

private:
	void addOne( const ChunkObstacle & cso );
	void modOne( const ChunkObstacle & cso, const Matrix & oldWorld );
	void delOne( const ChunkObstacle & cso );

	Chunk *				pChunk_;
	BW::vector<ChunkDynamicObstaclePtr>	cdos_;
};

ChunkCache::Instance<ChunkDynamicObstacles> ChunkDynamicObstacles::instance;


// -----------------------------------------------------------------------------
// Section: ChunkDynamicObstacle
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkDynamicObstacle::ChunkDynamicObstacle( PyModelObstacle * pObstacle ) :
	ChunkDynamicEmbodiment( pObstacle, 
		(WantFlags)(
		WANTS_UPDATE | WANTS_DRAW | WANTS_TICK |
			(ChunkObstacle::DYNAMIC_OBSTACLE_USER_FLAG << USER_FLAG_SHIFT)) )
{
	BW_GUARD;
	this->pObstacle()->attach();
#if UMBRA_ENABLE
	// Set up our unit boundingbox for the obstacle, we use a unit bounding box
	// and scale it using transforms since this may be a dynamic object.

	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init( this, BoundingBox( Vector3(0,0,0), Vector3(1,1,1)), Matrix::identity, NULL );
	pUmbraDrawItem_ = pUmbraChunkItem;

#endif
}


/**
 *	Destructor.
 */
ChunkDynamicObstacle::~ChunkDynamicObstacle()
{
	BW_GUARD;
	this->pObstacle()->detach();
}


/**
 * Chunk item tick method
 */
void ChunkDynamicObstacle::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkDynamicObstacle_tick );

	this->pObstacle()->tick( dTime );

	// Update the visibility bounds of our parent chunk
	if (pChunk_)
	{
		pChunk_->space()->updateOutsideChunkInQuadTree( pChunk_ );
	}

#if UMBRA_ENABLE
	// Update umbra object
	if (pChunk_ != NULL)
	{
		// Get the object to cell transform
		Matrix m = Matrix::identity;
		m.preMultiply( worldTransform() );

		BoundingBox bb;
		this->localBoundingBox( bb, true );

		// Create the scale transform from the bounding box
		Vector3 scale = bb.maxBounds() - bb.minBounds();

		if ((scale.x != 0.f && scale.y != 0.f && scale.z != 0.f) &&
			!(bb == BoundingBox::s_insideOut_) &&
			(scale.length() < 1000000.f))
		{
			// Set up our transform, the transform includes the scale of the bounding box
			Matrix m2;
			m2.setTranslate( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
			m.preMultiply( m2 );

			m2.setScale( scale.x, scale.y, scale.z );
			m.preMultiply( m2 );

			pUmbraDrawItem_->updateCell( pChunk_->getUmbraCell() );
			pUmbraDrawItem_->updateTransform( m );
		}
		else
		{
			pUmbraDrawItem_->updateCell( NULL );
		}
	}
	else
	{
		pUmbraDrawItem_->updateCell( NULL );
	}
#endif
}


/**
 *	Re-calculate LOD and apply animations to this obstacle for this frame.
 *	Should be called once per frame.
 */
void ChunkDynamicObstacle::updateAnimations()
{
	BW_GUARD;
	this->pObstacle()->updateAnimations( this->worldTransform() );
}


/**
 * Chunk item draw method
 */
void ChunkDynamicObstacle::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	this->pObstacle()->draw( drawContext, this->worldTransform() );
}

/**
 *	Chunk item toss method
 */
void ChunkDynamicObstacle::toss( Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk_ != NULL)
		ChunkDynamicObstacles::instance( *pChunk_ ).del( this );

	this->ChunkDynamicEmbodiment::toss( pChunk );

	if (pChunk_ != NULL)
		ChunkDynamicObstacles::instance( *pChunk_ ).add( this );
}

void ChunkDynamicObstacle::enterSpace( ChunkSpacePtr pSpace, bool transient )
{
	BW_GUARD;
	if (!transient)
	{
		this->pObstacle()->enterWorld();

		const Matrix& worldTransform = this->pObstacle()->worldTransform();
		SuperModel* pSuperModel = this->pObstacle()->superModel();

		collectObstacles( pSuperModel, worldTransform, this, obstacles_ );
	}

	this->ChunkDynamicEmbodiment::enterSpace( pSpace, transient );
}

void ChunkDynamicObstacle::leaveSpace( bool transient )
{
	BW_GUARD;
	this->ChunkDynamicEmbodiment::leaveSpace( transient );
	if (!transient)
	{
		this->pObstacle()->leaveWorld();

		// have to clear these when leaving world or else references to
		// chunk item will never disappear and we will never be detached
		obstacles_.clear();
	}
}

void ChunkDynamicObstacle::move( float dTime )
{
	BW_GUARD;
	// get in the right chunk
	this->sync();

	// update the positions of all our obstacles
	PyModelObstacle * pObstacle = this->pObstacle();
	Obstacles & obs = this->obstacles();
	if (!obs.empty())
	{
		Matrix oldWorld = obs[0]->transform_;

		Matrix world = pObstacle->worldTransform();
		Matrix worldInverse; worldInverse.invert( world );
		for (uint i = 0; i < obs.size(); i++)
		{
			obs[i]->transform_ = world;
			obs[i]->transformInverse_ = worldInverse;
		}

		// make sure they are in the right columns
		if (pChunk_ != NULL)
			ChunkDynamicObstacles::instance( *pChunk_ ).mod( this, oldWorld );
	}

	// and call our base class method
	this->ChunkDynamicEmbodiment::move( dTime );
}


const Matrix & ChunkDynamicObstacle::worldTransform() const
{
	BW_GUARD;
	return this->pObstacle()->worldTransform();
}


void ChunkDynamicObstacle::localBoundingBox( BoundingBox & bb, bool skinny ) const
{
	BW_GUARD;
	pObstacle()->localBoundingBox( bb, skinny );
}


void ChunkDynamicObstacle::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny ) const
{
	BW_GUARD;
	pObstacle()->worldBoundingBox( bb, world, skinny );
}


/**
 *	Static method to convert from a PyObject to a ChunkEmbodiment
 */
int ChunkDynamicObstacle::convert( PyObject * pObj, ChunkEmbodimentPtr & pNew,
	const char * varName )
{
	BW_GUARD;
	if (!PyModelObstacle::Check( pObj )) return 0;

	PyModelObstacle * pModelObstacle = (PyModelObstacle*)pObj;
	if (!pModelObstacle->dynamic()) return 0;

	if (pModelObstacle->attached())
	{
		PyErr_Format( PyExc_TypeError, "%s must be set to a PyModelObstacle "
			"that is not attached elsewhere", varName );
		return -1;
	}

	pNew = new ChunkDynamicObstacle( pModelObstacle );
	return 0;
}

PyModelObstacle * ChunkDynamicObstacle::pObstacle() const
{
	return (PyModelObstacle*)&*pPyObject_;
}

/// registerer for our type of ChunkEmbodiment
static ChunkEmbodimentRegisterer<ChunkDynamicObstacle> s_chunkDynamicObstacleRegisterer;

// -----------------------------------------------------------------------------
// Section: ChunkDynamicObstacles
// -----------------------------------------------------------------------------

int ChunkDynamicObstacles::focus()
{
	BW_GUARD;
	// add all
	for (uint i = 0; i < cdos_.size(); i++)
	{
		ChunkDynamicObstacle::Obstacles & obs = cdos_[i]->obstacles();
		for (uint j = 0; j < obs.size(); j++)
		{
			this->addOne( *obs[j] );
		}
	}
	return static_cast<int>(cdos_.size());
}

void ChunkDynamicObstacles::add( ChunkDynamicObstacle * cdo )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pChunk_->isBound() )
	{
		return;
	}

	cdos_.push_back( cdo );

	if (pChunk_->focussed())
	{
		ChunkDynamicObstacle::Obstacles & obs = cdo->obstacles();
		for (uint j = 0; j < obs.size(); j++)
		{
			this->addOne( *obs[j] );
		}
	}
}

void ChunkDynamicObstacles::mod( ChunkDynamicObstacle * cdo,
	const Matrix & oldWorld )
{
	BW_GUARD;
	if (pChunk_->focussed())
	{
		ChunkDynamicObstacle::Obstacles & obs = cdo->obstacles();
		for (uint j = 0; j < obs.size(); j++)
		{
			this->modOne( *obs[j], oldWorld );
		}
	}
}

void ChunkDynamicObstacles::del( ChunkDynamicObstaclePtr cdo )
{
	BW_GUARD;
	uint i;
	for (i = 0; i < cdos_.size(); i++)
	{
		if (cdos_[i] == cdo) break;
	}
	IF_NOT_MF_ASSERT_DEV( i < cdos_.size() )
	{
		return;
	}
	cdos_[i] = cdos_.back();
	cdos_.pop_back();

	if (pChunk_->focussed())
	{
		ChunkDynamicObstacle::Obstacles & obs = cdo->obstacles();
		for (uint j = 0; j < obs.size(); j++)
		{
			this->delOne( *obs[j] );
		}
	}
}


void ChunkDynamicObstacles::addOne( const ChunkObstacle & cso )
{
	BW_GUARD;
	// put obstacles in appropriate columns

	BoundingBox obstacleBB( cso.bb_ );
	obstacleBB.transformBy( cso.transform_ );

	ChunkSpacePtr pSpace = pChunk_->space();
	const float gridSize = pSpace->gridSize();
	for(	int x = pSpace->pointToGrid( obstacleBB.minBounds().x );
			x <= pSpace->pointToGrid( obstacleBB.maxBounds().x );
			x++ )
	{
		for(int z = pSpace->pointToGrid( obstacleBB.minBounds().z );
			z <= pSpace->pointToGrid( obstacleBB.maxBounds().z );
			z++ )
		{
			// This code is the same as in modOne, if you change one, 
			// change the other too
			Vector3 columnPoint(	( x + 0.5f ) * gridSize,
									0.0f,
									( z + 0.5f ) * gridSize );

			ChunkSpace::Column * pColumn = 
				pSpace->column( columnPoint, false );
			if( pColumn )
			{
				BoundingBox columnBB(	Vector3((x+0) * gridSize,
												0.0f,
												(z+0) * gridSize ),
										Vector3((x+1) * gridSize,
												0.0f,
												(z+1) * gridSize ) );
				columnBB.addYBounds( MIN_CHUNK_HEIGHT );
				columnBB.addYBounds( MAX_CHUNK_HEIGHT );

				if ( columnBB.intersects( obstacleBB ) )
					pColumn->addDynamicObstacle( cso );
			}
		}
	}
}

void ChunkDynamicObstacles::modOne(	const ChunkObstacle & cso,
									const Matrix & oldWorld )
{
	BW_GUARD;
	BoundingBox oldBB( cso.bb_ );
	BoundingBox newBB( cso.bb_ );

	oldBB.transformBy( oldWorld );
	newBB.transformBy( cso.transform_ );

	BoundingBox sumBB( oldBB );
	sumBB.addBounds( newBB );

	ChunkSpacePtr pSpace = pChunk_->space();
	const float gridSize = pSpace->gridSize();
	for(	int x = pSpace->pointToGrid( sumBB.minBounds().x );
			x <= pSpace->pointToGrid( sumBB.maxBounds().x );
			x++ )
	{
		for(int z = pSpace->pointToGrid( sumBB.minBounds().z );
			z <= pSpace->pointToGrid( sumBB.maxBounds().z );
			z++ )
		{
			// This code is the same as in addOne, if you change one, 
			// change the other too
			Vector3 columnPoint(	( x + 0.5f ) * gridSize,
									0.0f,
									( z + 0.5f ) * gridSize );

			ChunkSpace::Column * pColumn =
				pSpace->column( columnPoint, false );
			if( pColumn )
			{
				BoundingBox columnBB(	Vector3((x+0) * gridSize,
												0.0f,
												(z+0) * gridSize ),
										Vector3((x+1) * gridSize,
												0.0f,
												(z+1) * gridSize ) );
				columnBB.addYBounds( MIN_CHUNK_HEIGHT );
				columnBB.addYBounds( MAX_CHUNK_HEIGHT );

				bool inOld = columnBB.intersects( oldBB );
				bool inNew = columnBB.intersects( newBB );

				if (inNew && !inOld)
					pColumn->addDynamicObstacle( cso );

				if (!inNew && inOld)
					pColumn->delDynamicObstacle( cso );
			}
		}
	}
}

void ChunkDynamicObstacles::delOne( const ChunkObstacle & cso )
{
	BW_GUARD;
	// remove obstacles from appropriate columns

	BoundingBox obstacleBB( cso.bb_ );
	obstacleBB.transformBy( cso.transform_ );

	ChunkSpacePtr pSpace = pChunk_->space();
	const float gridSize = pSpace->gridSize();
	for(	int x = pSpace->pointToGrid( obstacleBB.minBounds().x );
			x <= pSpace->pointToGrid( obstacleBB.maxBounds().x );
			x++ )
	{
		for( int z = pSpace->pointToGrid( obstacleBB.minBounds().z );
			z <= pSpace->pointToGrid( obstacleBB.maxBounds().z );
			z++ )
		{
			Vector3 columnPoint(	x * gridSize,
									0.0f,
									z * gridSize );

			ChunkSpace::Column * pColumn =
				pChunk_->space()->column( columnPoint, false );
			if( pColumn )
			{
				BoundingBox columnBB(	columnPoint,
										Vector3( (x+1) * gridSize,
										0.0f,
										(z+1) * gridSize ) );
				columnBB.addYBounds( MIN_CHUNK_HEIGHT );
				columnBB.addYBounds( MAX_CHUNK_HEIGHT );

				if ( columnBB.intersects( obstacleBB ) )
					pColumn->delDynamicObstacle( cso );
			}
		}
	}
}


// -----------------------------------------------------------------------------
// Section: ChunkStaticObstacle
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkStaticObstacle::ChunkStaticObstacle( PyModelObstacle * pObstacle ) :
	ChunkStaticEmbodiment( pObstacle,
	(WantFlags)(WANTS_UPDATE | WANTS_DRAW | WANTS_TICK) ),
	worldTransform_( pObstacle->worldTransform() )
{
	BW_GUARD;
	this->pObstacle()->attach();
#if UMBRA_ENABLE
	// Set up our unit boundingbox for the obstacle, we use a unit bounding box
	// and scale it using transforms.
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init( this, BoundingBox( Vector3(0,0,0), Vector3(1,1,1)), Matrix::identity, NULL );
	pUmbraDrawItem_ = pUmbraChunkItem;
#endif
}


/**
 *	Destructor.
 */
ChunkStaticObstacle::~ChunkStaticObstacle()
{
	BW_GUARD;
	this->pObstacle()->detach();
}


/**
 * Chunk item tick method
 */
void ChunkStaticObstacle::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkStaticObstacle_tick );

	this->pObstacle()->tick( dTime );
#if UMBRA_ENABLE
	// Update umbra object
	if (pChunk_ != NULL)
	{
		Matrix m = Matrix::identity;
		m.preMultiply( worldTransform() );

		BoundingBox bb;
		this->localBoundingBox( bb, true );

		// Create the scale transform from the bounding box
		Vector3 scale = bb.maxBounds() - bb.minBounds();

		if ((scale.x != 0.f && scale.y != 0.f && scale.z != 0.f) &&
			!(bb == BoundingBox::s_insideOut_) &&
			(scale.length() < 1000000.f))
		{
			// Set up our transform, the transform includes the scale of the bounding box
			Matrix m2;
			m2.setTranslate( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
			m.preMultiply( m2 );

			m2.setScale( scale.x, scale.y, scale.z );
			m.preMultiply( m2 );
			pUmbraDrawItem_->updateCell( pChunk_->getUmbraCell() );
			pUmbraDrawItem_->updateTransform( m );
		}
		else
		{
			pUmbraDrawItem_->updateCell( NULL );
		}
	}
	else
	{
		pUmbraDrawItem_->updateCell( NULL );
	}
#endif
}


/**
 *	Re-calculate LOD and apply animations to this obstacle for this frame.
 *	Should be called once per frame.
 */
void ChunkStaticObstacle::updateAnimations()
{
	BW_GUARD;
	this->pObstacle()->updateAnimations( worldTransform_ );
}


/**
 *	Chunk item draw method
 */
void ChunkStaticObstacle::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	this->pObstacle()->draw( drawContext, worldTransform_ );
}

/**
 *	Chunk item toss method
 */
void ChunkStaticObstacle::toss( Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk_ != NULL)
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );

	this->ChunkStaticEmbodiment::toss( pChunk );

	if (pChunk_ != NULL)
	{
		PyModelObstacle * pObstacle = this->pObstacle();
		Obstacles & obs = this->obstacles();
		if (!obs.empty())
		{
			ChunkModelObstacle & chunkModelObstacle =
				ChunkModelObstacle::instance( *pChunk_ );

			Matrix worldInverse; worldInverse.invert( worldTransform_ );
			for (uint i = 0; i < obs.size(); i++)
			{
				obs[i]->transform_ = worldTransform_;
				obs[i]->transformInverse_ = worldInverse;

				chunkModelObstacle.addObstacle( &*obs[i] );
			}
		}
	}
}


/**
 *	overridden lend method
 */
void ChunkStaticObstacle::lend( Chunk * pLender )
{
	BW_GUARD;
	if (pChunk_)
	{
		BoundingBox bb;
		pObstacle()->worldBoundingBox( bb, this->worldTransform_ );
		this->lendByBoundingBox( pLender, bb );
	}
}


void ChunkStaticObstacle::enterSpace( ChunkSpacePtr pSpace, bool transient )
{
	BW_GUARD;
	if (!transient)
	{
		this->pObstacle()->enterWorld();

		const Matrix& worldTransform = this->pObstacle()->worldTransform();
		SuperModel* pSuperModel = this->pObstacle()->superModel();

		collectObstacles( pSuperModel, worldTransform, this, obstacles_ );
	}
	this->ChunkStaticEmbodiment::enterSpace( pSpace, transient );
}

void ChunkStaticObstacle::leaveSpace( bool transient )
{
	BW_GUARD;
	this->ChunkStaticEmbodiment::leaveSpace( transient );
	if (!transient)
	{
		this->pObstacle()->leaveWorld();

		// have to clear these when leaving world or else references to
		// chunk item will never disappear and we will never be detached
		obstacles_.clear();
	}
}


const Matrix & ChunkStaticObstacle::worldTransform() const
{
	return worldTransform_;
}


void ChunkStaticObstacle::localBoundingBox( BoundingBox & bb, bool skinny ) const
{
	BW_GUARD;
	pObstacle()->localBoundingBox( bb, skinny );
}


void ChunkStaticObstacle::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny ) const
{
	BW_GUARD;
	pObstacle()->worldBoundingBox( bb, world, skinny );
}


/**
 *	Static method to convert from a PyObject to a ChunkEmbodiment
 */
int ChunkStaticObstacle::convert( PyObject * pObj, ChunkEmbodimentPtr & pNew,
	const char * varName )
{
	BW_GUARD;
	if (!PyModelObstacle::Check( pObj )) return 0;

	PyModelObstacle * pModelObstacle = (PyModelObstacle*)pObj;
	if (pModelObstacle->dynamic()) return 0;

	if (pModelObstacle->attached())
	{
		PyErr_Format( PyExc_TypeError, "%s must be set to a PyModelObstacle "
			"that is not attached elsewhere", varName );
		return -1;
	}

	pNew = new ChunkStaticObstacle( pModelObstacle );
	return 0;
}

PyModelObstacle * ChunkStaticObstacle::pObstacle() const
{
	return (PyModelObstacle*)&*pPyObject_;
}

/// registerer for our type of ChunkEmbodiment
static ChunkEmbodimentRegisterer<ChunkStaticObstacle> registerer2;

BW_END_NAMESPACE

// chunk_dynamic_obstacle.cpp
