#include "pch.hpp"
#include "chunk_water.hpp"

#include "cstdmf/guard.hpp"

#include "moo/render_context.hpp"

#include "moo/geometrics.hpp"

#include "chunk.hpp"
#include "space/space_romp_collider.hpp"
#include "chunk_space.hpp"
#include "chunk_manager.hpp"
#include "geometry_mapping.hpp"

#ifndef CODE_INLINE
#include "chunk_water.ipp"
#endif

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkWater
// -----------------------------------------------------------------------------


int ChunkWater_token;

/**
 *	Constructor.
 */
ChunkWater::ChunkWater( BW::string uid ) :
	VeryLargeObject( uid, "water" ),
	pWater_( NULL ),
	pChunk_( NULL )
{
}


/**
 *	Constructor.
 */
ChunkWater::ChunkWater() :
	pWater_( NULL ),
	pChunk_( NULL )
{
	uid_ = "";
	type_ = "water";
}


/**
 *	Destructor.
 */
ChunkWater::~ChunkWater()
{
	if (pWater_ != NULL)
	{
		Water::deleteWater(pWater_);
		pWater_ = NULL;
	}
}


/**
 *	Load method
 */
bool ChunkWater::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk==NULL)
		return false;

	// clear existing water if present
	if (pWater_ != NULL)
		shouldRebuild(true);

	BW::string odata = pChunk->mapping()->path() + '_' + uid_ + ".odata";
	BW::string legacyOdata = pChunk->mapping()->path() + uid_ + ".odata";
	if (!BWResource::fileExists( odata ) && BWResource::fileExists( legacyOdata ))
	{
		odata = legacyOdata;
	}

	if (!state_.load( pSection, pChunk->boundingBox().centre(), pChunk->mapping()->mapper() ) ||
		!resources_.load( pSection, odata ))
	{
		return false;
	}

	pChunk_ = pChunk;
	return true;
}


void ChunkWater::syncInit(ChunkVLO* pVLO)
{
	BW_GUARD;
}

BoundingBox ChunkWater::chunkBB( Chunk* pChunk )
{
	BW_GUARD;
	BoundingBox bb = BoundingBox::s_insideOut_;
	BoundingBox cbb = pChunk->boundingBox();

	Vector3 size( state_.size_.x * 0.5f, 0, state_.size_.y * 0.5f );
	BoundingBox wbb( -size, size );

	Matrix m;
	m.setRotateY( state_.orientation_ );
	m.postTranslateBy( state_.position_ );

	wbb.transformBy( m );

	if (wbb.intersects( cbb ))
	{
		bb.setBounds( 
			Vector3(	std::max( wbb.minBounds().x, cbb.minBounds().x ),
						std::max( wbb.minBounds().y, cbb.minBounds().y ),
						std::max( wbb.minBounds().z, cbb.minBounds().z ) ),
			Vector3(	std::min( wbb.maxBounds().x, cbb.maxBounds().x ),
						std::min( wbb.maxBounds().y, cbb.maxBounds().y ),
						std::min( wbb.maxBounds().z, cbb.maxBounds().z ) ) );
		bb.transformBy( pChunk->transformInverse() );
	}

	return bb;
}


bool ChunkWater::addYBounds( BoundingBox& bb ) const
{
	BW_GUARD;
	bb.addYBounds( state_.position_.y );

	return true;
}


/**
 *	Draw (and update) this body of water
 */
void ChunkWater::drawInChunk( Moo::DrawContext& drawContext, Chunk* pChunk )
{
	BW_GUARD;
	static DogWatch drawWatch( "ChunkWater" );
	ScopedDogWatch watcher( drawWatch );

	if (Moo::rc().reflectionScene())
		return;

	// create the water if this is the first time
	if (pWater_ == NULL)
	{
		pWater_ = new Water( state_, resources_, 
			RompCollider::createDefault( RompCollider::TERRAIN_ONLY/*, pSpace*/ ) );	// TODO: Uncomment pSpace!

		this->objectCreated();
	}
	else if ( shouldRebuild() )
	{
		pWater_->rebuild( state_, resources_ );
		shouldRebuild(false);
		this->objectCreated();
	}

	// and remember to draw it after the rest of the solid scene
	if ( !s_simpleDraw )
	{
		Waters::addToDrawList( pWater_ );
		if (!pChunk->isOutsideChunk())
		{
			pWater_->addVisibleChunk( pChunk );
		}
		else
		{
			pWater_->outsideVisible( true );
		}
	}
	//RA: turned off simple draw for now....TODO: FIX
}


/**
 *	Apply a disturbance to this body of water
 */
void ChunkWater::sway( const Vector3 & src, const Vector3 & dst, const float diameter )
{
	BW_GUARD;
	if (pWater_ != NULL)
	{
		pWater_->addMovement( src, dst, diameter );
	}
}


#ifdef EDITOR_ENABLED
/**
 *	This method regenerates the water ... later
 */
void ChunkWater::dirty()
{
	BW_GUARD;
	if (pWater_ != NULL)
		shouldRebuild(true);
}
#endif //EDITOR_ENABLED

/**
 *	This static method creates a body of water from the input section and adds
 *	it to the given chunk.
 */
bool ChunkWater::create(
	Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid )
{
	BW_GUARD;
	//TODO: check it isnt already created?
	ChunkWater * pItem = new ChunkWater( uid );	
	if (pItem->load( pSection, pChunk ))
		return true;
	bw_safe_delete(pItem);
	return false;
}


void ChunkWater::simpleDraw( bool state )
{
	ChunkWater::s_simpleDraw = state;
}


/// Static factory initialiser
VLOFactory ChunkWater::factory_( "water", 0, ChunkWater::create );
/// This variable is set to true if we would like to draw cheaply
/// ( e.g. during picture-in-picture )
bool ChunkWater::s_simpleDraw = false;
//
//
//bool ChunkWater::oldCreate( Chunk * pChunk, DataSectionPtr pSection )
//{
//	bool converted = pSection->readBool("deprecated",false);
//	if (converted)
//		return false;
//
//	//TODO: generalise the want flags...
//	ChunkVLO * pVLO = new ChunkVLO( 5 );	
//	if (!pVLO->legacyLoad( pSection, pChunk, BW::string("water") ))
//	{
//		bw_safe_delete(pVLO);
//		return false;
//	}
//	else
//	{
//		pChunk->addStaticItem( pVLO );
//		return true;
//	}
//	return false;
//}
//
///// Static factory initialiser
//ChunkItemFactory ChunkWater::oldWaterFactory_( "water", 0, ChunkWater::oldCreate );

BW_END_NAMESPACE

// chunk_water.cpp
