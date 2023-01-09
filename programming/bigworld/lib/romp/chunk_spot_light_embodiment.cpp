#include "pch.hpp"
#include "chunk_spot_light_embodiment.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

#include <space/client_space.hpp>
#include <space/deprecated_space_helpers.hpp>

#include <math/boundbox.hpp>
#include <math/convex_hull.hpp>

BW_BEGIN_NAMESPACE

ChunkSpotLightEmbodiment::ChunkSpotLightEmbodiment( const PySpotLight & parent )
	: ChunkSpotLight( (WantFlags)WANTS_TICK )
	, parent_(parent)
	, visible_( false )
{
	BW_GUARD;
}

ChunkSpotLightEmbodiment::~ChunkSpotLightEmbodiment()
{
	BW_GUARD_PROFILER( ChunkSpotLightEmbodiment_dtor );
	if (this->visible()) this->visible( false );
}

// We may need to refine the criteria for intersection as this is
// quite conservative for spot lights
bool ChunkSpotLightEmbodiment::doIntersectAndAddToLightContainer(
	const ConvexHull & hull, Moo::LightContainer & lightContainer ) const
{
	const bool doesIntersect = 
		(visible_ && hull.intersects( position(), outerRadius() ));
	if (doesIntersect)
	{
		lightContainer.addSpot( pLight_ );
	}
	return doesIntersect;
}

// We may need to refine the criteria for intersection as this is
// quite conservative for spot lights
bool ChunkSpotLightEmbodiment::doIntersectAndAddToLightContainer(
	const AABB & bbox, Moo::LightContainer & lightContainer ) const
{
	const bool doesIntersect = 
		(visible_ && (bbox.distance( position() ) <= outerRadius()));
	if (doesIntersect)
	{
		lightContainer.addSpot( pLight_ );
	}
	return doesIntersect;
}

bool ChunkSpotLightEmbodiment::visible() const
{
	return visible_;
}

void ChunkSpotLightEmbodiment::visible( bool v )
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


const Vector3 & ChunkSpotLightEmbodiment::position() const
{
	BW_GUARD;
	return pLight_->worldPosition();
}

void ChunkSpotLightEmbodiment::position( const Vector3 & npos )
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
				ChunkLightCache::instance( *pChunk_ ).moveSpot( pLight_, locOldPos, pLight_->outerRadius() );
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

const Vector3 & ChunkSpotLightEmbodiment::direction() const
{
	BW_GUARD;
	return pLight_->worldDirection();
}

void ChunkSpotLightEmbodiment::direction( const Vector3 & ndir )
{
	BW_GUARD;
	if (pChunk_ != NULL)
	{
		pLight_->direction( pChunk_->transformInverse().applyVector( ndir ) );
	}
	else
	{
		pLight_->direction( ndir );
	}
}

const Moo::Colour & ChunkSpotLightEmbodiment::colour() const
{
	BW_GUARD;
	return pLight_->colour();
}

void ChunkSpotLightEmbodiment::colour( const Moo::Colour & v )
{
	BW_GUARD;
	pLight_->colour( v );
}

int ChunkSpotLightEmbodiment::priority() const
{
	BW_GUARD;
	return pLight_->priority();
}

void ChunkSpotLightEmbodiment::priority( int v )
{
	BW_GUARD;
	pLight_->priority( v );
}

float ChunkSpotLightEmbodiment::innerRadius() const
{
	BW_GUARD;
	return pLight_->innerRadius();
}

void ChunkSpotLightEmbodiment::innerRadius( float v )
{
	BW_GUARD;
	float orad = pLight_->outerRadius();
	pLight_->innerRadius( v );
	radiusUpdated( orad );
}

float ChunkSpotLightEmbodiment::outerRadius() const
{
	BW_GUARD;
	return pLight_->outerRadius();
}

void ChunkSpotLightEmbodiment::outerRadius( float v )
{
	BW_GUARD;
	float orad = pLight_->outerRadius();
	pLight_->outerRadius( v );
	radiusUpdated( orad );
}

float ChunkSpotLightEmbodiment::cosConeAngle() const
{
	BW_GUARD;
	return pLight_->cosConeAngle();
}

void ChunkSpotLightEmbodiment::cosConeAngle( float v )
{
	BW_GUARD;
	pLight_->cosConeAngle( v );
}

void ChunkSpotLightEmbodiment::radiusUpdated( float orad )
{
	BW_GUARD;
	pLight_->worldTransform( (pChunk_ != NULL) ?
		pChunk_->transform() : Matrix::identity );

	if ( !almostEqual( orad, pLight_->outerRadius() ) && pChunk_ != NULL )
	{
		ChunkLightCache::instance( *pChunk_ ).moveSpot( pLight_, 
			pLight_->position(), 
			orad );
	}
}

void ChunkSpotLightEmbodiment::doLeaveSpace()
{
	// Nothing to be done
}

void ChunkSpotLightEmbodiment::doUpdateLocation()
{
	BW_GUARD;
	if (parent_.source())
	{
		Matrix m;
		parent_.source()->matrix( m );
		position( m.applyToOrigin() );
		direction( m.applyToUnitAxisVector(2) );
	}
}

void ChunkSpotLightEmbodiment::recalc()
{
	this->position( this->position() );
}

void ChunkSpotLightEmbodiment::nest( ChunkSpace * pSpace )
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

void ChunkSpotLightEmbodiment::doTick( float dTime )
{
	tick(dTime);
}

void ChunkSpotLightEmbodiment::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkSpotLightEmbodiment_tick );

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
				ChunkLightCache::instance( *pChunk_ ).moveSpot( pLight_, 
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

