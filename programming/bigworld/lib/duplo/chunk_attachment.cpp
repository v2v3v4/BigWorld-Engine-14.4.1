#include "pch.hpp"
#include "chunk_attachment.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"

#include "moo/render_context.hpp"

#include "chunk/umbra_config.hpp"

#include "moo/geometrics.hpp"
#include "moo/lod_settings.hpp"

#if UMBRA_ENABLE
#include <ob/umbraObject.hpp>
#include "chunk/umbra_chunk_item.hpp"
#endif


DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkAttachment
// -----------------------------------------------------------------------------


/**
 *	Constructor
 */
ChunkAttachment::ChunkAttachment() :
	ChunkDynamicEmbodiment(
		(WantFlags)(WANTS_DRAW | WANTS_TICK | WANTS_UPDATE) ),
	needsSync_( false ),
	worldTransform_( Matrix::identity ),
	inited_( false )
{
}


/**
 *	Constructor
 */
ChunkAttachment::ChunkAttachment( PyAttachmentPtr pAttachment ) :
	ChunkDynamicEmbodiment( pAttachment.get(),
		(WantFlags)(WANTS_DRAW | WANTS_TICK | WANTS_UPDATE) ),
	needsSync_( false ),
	worldTransform_( Matrix::identity ),
	inited_( true )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pPyObject_ )
	{
		MF_EXIT( "attach to NULL object" );
	}

	if (pAttachment != NULL && !pAttachment->castsShadow())
	{
		sceneObject_.flags().isCastingDynamicShadow( false );
	}

	this->pAttachment()->attach( this );
}

/**
 *	Destructor
 */
ChunkAttachment::~ChunkAttachment()
{
	BW_GUARD;
	if ( inited_ )
	{
		this->pAttachment()->detach();
	}
}


/**
 *	Chunk item tick method
 */
void ChunkAttachment::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkAttachment_tick );

	if ( !inited_ )
	{
		inited_ = true;
		this->pAttachment()->attach( this );
	}

	this->pAttachment()->tick( dTime );

	// Update the visibility bounds of our parent chunk
	if (pChunk_)
	{
		pChunk_->space()->updateOutsideChunkInQuadTree( pChunk_ );
	}

#if UMBRA_ENABLE
	// Update umbra object

	if (!pUmbraDrawItem_)
	{
		// Set up our unit boundingbox for the attachment, we use a unit bounding box
		// and scale it using transforms since this is a dynamic object.
		UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
		pUmbraChunkItem->init( this, BoundingBox( Vector3(0,0,0), Vector3(1,1,1)), Matrix::identity, NULL );
		pUmbraDrawItem_ = pUmbraChunkItem;
	}

	if ( !pUmbraShadowCasterTestItem_ )
	{
		UmbraChunkItemShadowCaster* pUmbraShadowCasterTestItem = new UmbraChunkItemShadowCaster();
		pUmbraShadowCasterTestItem->init( this, BoundingBox( Vector3(0,0,0), Vector3(1,1,1)), Matrix::identity, NULL, true );
		pUmbraShadowCasterTestItem_ = pUmbraShadowCasterTestItem;
		pUmbraShadowCasterTestItem->pUmbraObject()->object()->set( Umbra::OB::Object::UNBOUNDED, true );
		// pUmbraShadowCasterTestItem->pUmbraObject()->object()->setRenderCost( Umbra::OB::Object::EXPENSIVE );
	}
	
	if (pChunk_ != NULL)
	{
		// Get the object to cell transform
		Matrix m = Matrix::identity;
		m.preMultiply( worldTransform_ );

		BoundingBox bb;
		this->localVisibilityBox( bb, false );

		// Create the scale transform from the bounding box
		Vector3 scale(0,0,0);
		if (!bb.insideOut())
			scale = bb.maxBounds() - bb.minBounds();

		// Const number to see if the bounding box is too big for umbra.
		const float TOO_BIG_FOR_UMBRA = 100000.f;

		if (scale.x != 0.f && scale.y != 0.f && scale.z != 0.f &&
			scale.x < TOO_BIG_FOR_UMBRA && scale.y < TOO_BIG_FOR_UMBRA && scale.z < TOO_BIG_FOR_UMBRA)
		{
			// Set up our transform, the transform includes the scale of the bounding box
			Matrix m2;
			m2.setTranslate( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
			m.preMultiply( m2 );

			m2.setScale( scale.x, scale.y, scale.z );
			m.preMultiply( m2 );
			pUmbraDrawItem_->updateCell( pChunk_->getUmbraCell() );
			pUmbraDrawItem_->updateTransform( m );
			
			pUmbraShadowCasterTestItem_->updateCell( pChunk_->getUmbraCell() );
			// pUmbraShadowCasterTestItem_->updateTransform( m );
		}
		else
		{			
			pUmbraDrawItem_->updateCell( NULL );
			pUmbraShadowCasterTestItem_->updateCell( NULL );
		}
	}
	else
	{
		pUmbraDrawItem_->updateCell( NULL );
		pUmbraShadowCasterTestItem_->updateCell( NULL );
	}
#endif
}


/**
 *	Re-calculate LOD and apply animations to this ChunkAttachment for this
 *	frame. Should be called once per frame.
 */
void ChunkAttachment::updateAnimations()
{
	BW_GUARD;

	// Calculate LOD
	const float lod = LodSettings::instance().calculateLod( worldTransform_ );
	this->pAttachment()->updateAnimations( worldTransform_, lod );
}


/**
 *	Chunk item draw method
 */
void ChunkAttachment::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	// check that it fits in the BB first
	BoundingBox bb = BoundingBox::s_insideOut_;
	this->pAttachment()->localBoundingBox( bb, false );
	BoundingBox maxbb = BoundingBox::s_insideOut_;
	this->pAttachment()->localVisibilityBox( maxbb, false );

	if (bb != BoundingBox::s_insideOut_ && maxbb != BoundingBox::s_insideOut_)
	{

		Matrix clipTrans;
		clipTrans.multiply( worldTransform_, Moo::rc().viewProjection() );
		maxbb.calculateOutcode( clipTrans );

		if (!maxbb.combinedOutcode())
		{
			this->pAttachment()->draw( drawContext, worldTransform_ );
#ifdef EDITOR_ENABLED
			drawBoundingBoxes( bb, maxbb, worldTransform_ );
#endif
		}
	}
}


/**
 *	Chunk item toss method
 */
void ChunkAttachment::toss( Chunk * pChunk )
{
	BW_GUARD;
	this->ChunkDynamicEmbodiment::toss( pChunk );

	this->pAttachment()->tossed(
		pChunk_ == NULL || pChunk_->isOutsideChunk() );
}


void ChunkAttachment::generateTextureUsage( 
	Moo::ModelTextureUsageGroup& usageGroup )
{
	PyAttachment* pAttached = pAttachment();
	if (pAttached)
	{
		pAttached->generateTextureUsage( usageGroup );
	}
}


/**
 *	Enter the given space. Transient is set to true if switching spaces.
 */
void ChunkAttachment::enterSpace( ChunkSpacePtr pSpace, bool transient )
{
	BW_GUARD;
	this->ChunkDynamicEmbodiment::enterSpace( pSpace, transient );
	needsSync_ = false;

	if (!transient)
		this->pAttachment()->enterWorld();
}

/**
 *	Leave the current space.
 *	Transient is set to true if switching spaces.
 */
void ChunkAttachment::leaveSpace( bool transient )
{
	BW_GUARD;
	if (!transient)
		this->pAttachment()->leaveWorld();

	this->ChunkDynamicEmbodiment::leaveSpace( transient );
}


/**
 *	This method is called before tick to allow us a chance to move
 *	(which is not permitted during tick since we might change to
 *	a different chunk)
 */
void ChunkAttachment::move( float dTime )
{
	BW_GUARD;
	// promote our transforms if it has such aspirations but is hesitant
	if (needsSync_)
	{
		needsSync_ = false;
		this->sync();
	}

	// let the attachment move about
	this->pAttachment()->move( dTime );

	// and let our base class do its stuff
	this->ChunkDynamicEmbodiment::move( dTime );
}


/**
 *	This method retrieves a representative bounding box (in local coords)
 *	for this embodiment. It is not a strict bounding box, but it's a good
 *	one for displaying or intersecting with or other similar operations.
 */
void ChunkAttachment::localBoundingBox( BoundingBox & bb, bool skinny ) const
{
	BW_GUARD;	
	this->pAttachment()->localBoundingBox( bb, skinny );
}


/**
 *	This method retrieves a representative bounding box (in world coords)
 *	for this embodiment. It is not a strict bounding box, but it's a good
 *	one for displaying or intersecting with or other similar operations.
 */
void ChunkAttachment::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny ) const
{
	BW_GUARD;
	this->pAttachment()->worldBoundingBox( bb, world, skinny );
}


/**
 *	This method retrieves a representative visibility box (in local coords)
 *	for this embodiment. It is used to determine the visibility of the
 *  attachment.
 */
void ChunkAttachment::localVisibilityBox( BoundingBox & vbb, bool skinny ) const
{
	BW_GUARD;
	this->pAttachment()->localVisibilityBox( vbb, skinny );	
}


/**
 *	This method retrieves a representative visibility box (in world coords)
 *	for this embodiment. It is used to determine the visibility of the
 *  attachment.
 */
void ChunkAttachment::worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny ) const
{
	BW_GUARD;
	this->pAttachment()->worldVisibilityBox( vbb, world, skinny );
}


/**
 *	This method sets the matrix
 */
bool ChunkAttachment::setMatrix( const Matrix & worldTransform )
{
	BW_GUARD;
	worldTransform_ = worldTransform;

	if (!pSpace_) return true;

	if (!pSpace_->ticking())
		this->sync();
	else
		needsSync_ = true;

	return true;
}

/**
 * This method is called when a chunk item has moved
 * so we can update the texture usage group with that
 * information
 */
void ChunkAttachment::sync()
{
	ChunkDynamicEmbodiment::sync();
}

/**
 *	Static method to convert from a PyObject to a ChunkEmbodiment
 */
int ChunkAttachment::convert( PyObject * pObj, ChunkEmbodimentPtr & pNew,
	const char * varName )
{
	BW_GUARD;
	ScriptObject scriptObj( pObj );
	PyAttachmentPtr pAttachment = PyAttachmentPtr::create( scriptObj );
	if (!pAttachment)
	{
		return 0;
	}

	if (pAttachment->isAttached())
	{
		PyErr_Format( PyExc_TypeError, "%s must be set to an Attachment "
			"that is not attached elsewhere", varName );
		return -1;
	}

	pNew = new ChunkAttachment( pAttachment );
	return 0;
}


bool ChunkAttachment::addYBounds( BoundingBox& bb ) const
{
	BW_GUARD;

	BoundingBox vbb = BoundingBox::s_insideOut_;
	this->pAttachment()->localVisibilityBox(vbb, false);
	if (!vbb.insideOut())
	{
		vbb.transformBy( worldTransform_ );
		bb.addYBounds(vbb.minBounds().y);
		bb.addYBounds(vbb.maxBounds().y);
	}

	return true;
}


/// registerer for our type of ChunkEmbodiment
static ChunkEmbodimentRegisterer<ChunkAttachment> s_chunkAttachmentRegisterer;
int ChunkAttachment_token;

BW_END_NAMESPACE

// chunk_attachment.cpp
