#include "pch.hpp"
#include "chunk_omni_light_embodiment.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "duplo/pymodelnode.hpp"

#include <space/client_space.hpp>
#include <space/deprecated_space_helpers.hpp>

#include <math/boundbox.hpp>
#include <math/convex_hull.hpp>

BW_BEGIN_NAMESPACE

ChunkOmniLightEmbodiment::ChunkOmniLightEmbodiment( const PyOmniLight & parent )
	: ChunkOmniLight( (WantFlags)WANTS_TICK )
	, parent_(parent)
	, visible_( false )
{
	BW_GUARD;
}

ChunkOmniLightEmbodiment::~ChunkOmniLightEmbodiment()
{
	BW_GUARD_PROFILER( ChunkOmniLightEmbodiment_dtor );
	if (this->visible()) this->visible( false );
}

bool ChunkOmniLightEmbodiment::doIntersectAndAddToLightContainer(
	const ConvexHull & hull, Moo::LightContainer & lightContainer ) const
{
	const bool doesIntersect = 
		(visible_ && hull.intersects( position(), outerRadius() ));
	if (doesIntersect)
	{
		lightContainer.addOmni( pLight_ );
	}
	return doesIntersect;
}

bool ChunkOmniLightEmbodiment::doIntersectAndAddToLightContainer(
	const AABB & bbox, Moo::LightContainer & lightContainer ) const
{
	const bool doesIntersect = 
		(visible_ && (bbox.distance( position() ) <= outerRadius()));
	if (doesIntersect)
	{
		lightContainer.addOmni( pLight_ );
	}
	return doesIntersect;
}

bool ChunkOmniLightEmbodiment::visible() const
{
	return visible_;
}

void ChunkOmniLightEmbodiment::visible( bool v )
{
	BW_GUARD;
	if (visible_ == v) return;
	visible_ = v;
	if (visible_)
	{
		this->recalc();
	}
	else if (pChunk_ != NULL)
	{
		pChunk_->delDynamicItem( this, false );
	}
}


const Vector3 & ChunkOmniLightEmbodiment::position() const
{
	BW_GUARD;
	return pLight_->worldPosition();
}

void ChunkOmniLightEmbodiment::position( const Vector3 & npos )
{
	BW_GUARD;

	Chunk* oldChunk = pChunk_;

	Vector3 opos = pLight_->worldPosition();

	// find which chunk light was in, and move it in that chunk
	if (pChunk_ != NULL)
	{
		pChunk_->modDynamicItem( this, opos, npos );

		// Check again as modDynamicItem can put us outside the known chunks
		if (pChunk_ != NULL)
		{
			pLight_->position( pChunk_->transformInverse().applyPoint( npos ) );
			pLight_->worldTransform( pChunk_->transform() );

			// If the light actually moved, and we're still in the same chunk
			// then let the light cache know if it needs to push this light into
			// other chunks.
			if ( oldChunk == pChunk_ && !almostEqual( opos - npos, Vector3::zero() ) )
			{
				Vector3 locOldPos = pChunk_->transformInverse().applyPoint( opos );
				ChunkLightCache::instance( *pChunk_ ).moveOmni( pLight_, locOldPos, pLight_->outerRadius() );
			}
		}
	}
	else if (visible_)
	{
		ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
		if (pSpace.exists())
		{
			Chunk * pDest = pSpace->findChunkFromPoint( npos );
			if (pDest != NULL)
			{
				//since the newly created one's position_==worldPosition_, 
				//so we convert the position_ to be local in chunk space first
				pLight_->position( pDest->transformInverse().applyPoint( npos ) );

				pSpace->delHomelessItem( this );

				pDest->addDynamicItem( this );//ChunkLight::toss will be called here 
				//which will call it's pLight_->worldTransform(..), so we don't need call 
				//pLight_->worldTransform(..) again
			}
		}
	}

	if (pChunk_ == NULL)
	{
		pLight_->position( npos );
		pLight_->worldTransform( Matrix::identity );
	}
}

const Moo::Colour & ChunkOmniLightEmbodiment::colour() const
{
	BW_GUARD;
	return pLight_->colour();
}

void ChunkOmniLightEmbodiment::colour( const Moo::Colour & v )
{
	BW_GUARD;
	pLight_->colour( v );
}

int ChunkOmniLightEmbodiment::priority() const
{
	BW_GUARD;
	return pLight_->priority();
}

void ChunkOmniLightEmbodiment::priority( int v )
{
	BW_GUARD;
	pLight_->priority( v );
}

float ChunkOmniLightEmbodiment::innerRadius() const
{
	BW_GUARD;
	return pLight_->innerRadius();
}

void ChunkOmniLightEmbodiment::innerRadius( float v )
{
	BW_GUARD;
	float orad = pLight_->outerRadius();
	pLight_->innerRadius( v );
	radiusUpdated( orad );
}

float ChunkOmniLightEmbodiment::outerRadius() const
{
	BW_GUARD;
	return pLight_->outerRadius();
}

void ChunkOmniLightEmbodiment::outerRadius( float v )
{
	BW_GUARD;
	float orad = pLight_->outerRadius();
	pLight_->outerRadius( v );
	radiusUpdated( orad );
}

void ChunkOmniLightEmbodiment::radiusUpdated( float orad )
{
	BW_GUARD;
	pLight_->worldTransform( (pChunk_ != NULL) ?
		pChunk_->transform() : Matrix::identity );

	if ( !almostEqual( orad, pLight_->outerRadius() ) && pChunk_ != NULL )
	{
		ChunkLightCache::instance( *pChunk_ ).moveOmni( pLight_, 
			pLight_->position(), 
			orad );
	}
}

void ChunkOmniLightEmbodiment::doLeaveSpace()
{
	// Nothing to be done
}

void ChunkOmniLightEmbodiment::doUpdateLocation()
{
	BW_GUARD;
	if (parent_.source())
	{
		SCOPED_DISABLE_FRAME_BEHIND_WARNING
		Matrix m;
		parent_.source()->matrix( m );
		position( m.applyToOrigin() );
	}
}

void ChunkOmniLightEmbodiment::recalc()
{
	this->position( this->position() );
}

void ChunkOmniLightEmbodiment::nest( ChunkSpace * pSpace )
{
	BW_GUARD;
	Chunk * pDest = pSpace->findChunkFromPoint( this->position() );
	if (pDest != pChunk_)
	{
		if (pChunk_ != NULL)
			pChunk_->delDynamicItem( this, false );
		else
			pSpace->delHomelessItem( this );

		if (pDest != NULL)
			pDest->addDynamicItem( this );
		else
			pSpace->addHomelessItem( this );
	}
}

void ChunkOmniLightEmbodiment::doTick( float dTime )
{
	tick(dTime);
}

void ChunkOmniLightEmbodiment::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkOmniLightEmbodiment_tick );

	// update inner and outer radii
	if (parent_.bounds())
	{
		Vector4 val;
		parent_.bounds()->output( val );
		if (val.x > val.y && val.x > 0.001 && val.y < 500.0)
		{
			float orad = pLight_->outerRadius();

			pLight_->innerRadius( val.x );
			pLight_->outerRadius( val.y );

			pLight_->worldTransform( (pChunk_ != NULL) ?
				pChunk_->transform() : Matrix::identity );

			if ( !almostEqual( orad, val.y ) && pChunk_ != NULL )
			{
				ChunkLightCache::instance( *pChunk_ ).moveOmni( pLight_, 
					pLight_->position(), 
					orad );
			}
		}
	}

	// update colour
	if (parent_.shader())
	{
		Vector4 val;
		parent_.shader()->output( val );
		val *= 1.f/255.f;
		pLight_->colour( reinterpret_cast<Moo::Colour&>( val ) );
	}
}


BW_END_NAMESPACE

