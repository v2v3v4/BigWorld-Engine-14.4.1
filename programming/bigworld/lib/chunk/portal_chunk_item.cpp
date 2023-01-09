#include "pch.hpp"

#include "portal_chunk_item.hpp"

#include "chunk.hpp"
#include "chunk_model_obstacle.hpp"
#include "chunk_obstacle.hpp"

#include "scene/scene_object.hpp"

#ifndef MF_SERVER
#include "moo/draw_context.hpp"
#include "moo/render_context.hpp"
#endif

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PortalObstacle
// -----------------------------------------------------------------------------

/**
 *	This class is used to add a collision scene item for a portal. It checks
 *	the permissive state of the associated portal to decide whether to proceed
 *	with the collision test.
 */
class PortalObstacle : public ChunkObstacle
{
public:
	PortalObstacle( PortalChunkItem * pChunkItem );

	virtual bool collide( const Vector3 & source, const Vector3 & extent,
		CollisionState & state ) const;
	virtual bool collide( const WorldTriangle & source, const Vector3 & extent,
		CollisionState & state ) const;

private:
	void buildTriangles();

	PortalChunkItem * pChunkItem() const
	{
		return static_cast< PortalChunkItem * >( this->pItem() );
	}

	ChunkBoundary::Portal * pPortal() const
	{
		return this->pChunkItem()->pPortal();
	}

	WorldTriangle::Flags collisionFlags() const
	{
		return this->pPortal()->collisionFlags();
	}

	BoundingBox		bb_;
	mutable BW::vector<WorldTriangle>	ltris_;
};


/**
 *	Constructor
 */
PortalObstacle::PortalObstacle( PortalChunkItem * pChunkItem ) :
	ChunkObstacle( pChunkItem->chunk()->transform(), &bb_, pChunkItem )
{
	BW_GUARD;
	// now calculate our bb. fortunately the ChunkObstacle constructor
	// doesn't do anything with it except store it.

	ChunkBoundary::Portal * pPortal = this->pPortal();

	// extend 10cm into the chunk (the normal is always normalised)
	Vector3 ptExtra = pPortal->plane.normal() * 0.10f;

	// build up the bb from the portal points
	for (uint i = 0; i < pPortal->points.size(); i++)
	{
		Vector3 pt =
			pPortal->uAxis * pPortal->points[i][0] +
			pPortal->vAxis * pPortal->points[i][1] +
			pPortal->origin;

		if (!i)
			bb_ = BoundingBox( pt, pt );
		else
			bb_.addBounds( pt );
		bb_.addBounds( pt + ptExtra );
	}

	// and figure out the triangles (a similar process)
	this->buildTriangles();
}


/**
 *	Build the 'world' triangles to collide with
 */
void PortalObstacle::buildTriangles()
{
	BW_GUARD;
	ltris_.clear();

	ChunkBoundary::Portal * pPortal = this->pPortal();

	if (pPortal->points.size() < 3)
	{
		return;
	}

	// extend 5cm into the chunk
	Vector3 ptExOri = pPortal->origin + pPortal->plane.normal() * 0.05f;

	// build the wt's from the points
	Vector3 pto =	pPortal->uAxis * pPortal->points[0][0] +
			pPortal->vAxis * pPortal->points[0][1] +
			ptExOri;
	Vector3 pta;
	Vector3 ptb =	pPortal->uAxis * pPortal->points[1][0] +
			pPortal->vAxis * pPortal->points[1][1] +
			ptExOri;

	for (ChunkBoundary::V2Vector::size_type i = 2;
		i < pPortal->points.size(); i++)
	{
		// shuffle and find the next pt
		pta = ptb;
		ptb =	pPortal->uAxis * pPortal->points[i][0] +
			pPortal->vAxis * pPortal->points[i][1] +
			ptExOri;

		// make a triangle then
		ltris_.push_back( WorldTriangle( pto, pta, ptb ) );
	}
}


/**
 *	Collision test with an extruded point
 */
bool PortalObstacle::collide( const Vector3 & source, const Vector3 & extent,
	CollisionState & state ) const
{
	BW_GUARD;
	// see if we let anyone through
	if (this->pPortal()->permissive)
		return false;

	// ok, see if they collide then
	// (chances are very high if they're in the bb!)
	Vector3 tranl = extent - source;
	for (uint i = 0; i < ltris_.size(); i++)
	{
		// see if it intersects
		float rd = 1.0f;
		if (!ltris_[i].intersects( source, tranl, rd ) ) continue;

		// see how far we really travelled (handles scaling, etc.)
		float ndist = state.sTravel_ + (state.eTravel_-state.sTravel_) * rd;

		if (state.onlyLess_ && ndist > state.dist_) continue;
		if (state.onlyMore_ && ndist < state.dist_) continue;
		state.dist_ = ndist;

		// call the callback function
		ltris_[i].flags( uint8(this->collisionFlags()) );
		
		CollisionObstacle ob( &transform_, &transformInverse_,
			this->pItem()->sceneObject() );
		int say = state.cc_.visit( ob, ltris_[i], state.dist_ );

		// see if any other collisions are wanted
		if (!say) return true;

		// some are wanted ... see if it's only one side
		state.onlyLess_ = !(say & 2);
		state.onlyMore_ = !(say & 1);
	}

	return false;
}


/**
 *	Collision test with an extruded triangle
 */
bool PortalObstacle::collide( const WorldTriangle & source, const Vector3 & extent,
	CollisionState & state ) const
{
	BW_GUARD;
	// see if we let anyone through
	if (this->pPortal()->permissive)
		return false;

	// ok, see if they collide then
	// (chances are very high if they're in the bb!)
	Vector3 tranl = extent - source.v0();
	for (uint i = 0; i < ltris_.size(); i++)
	{
		// see if it intersects
		if (!ltris_[i].intersects( source, tranl ) ) continue;

		// see how far we really travelled
		float ndist = state.sTravel_;

		if (state.onlyLess_ && ndist > state.dist_) continue;
		if (state.onlyMore_ && ndist < state.dist_) continue;
		state.dist_ = ndist;

		// call the callback function
		ltris_[i].flags( uint8(this->collisionFlags()) );

		CollisionObstacle ob( &transform_, &transformInverse_,
			this->pItem()->sceneObject() );
		int say = state.cc_.visit( ob, ltris_[i], state.dist_ );

		// see if any other collisions are wanted
		if (!say) return true;

		// some are wanted ... see if it's only one side
		state.onlyLess_ = !(say & 2);
		state.onlyMore_ = !(say & 1);
	}

	return false;
}


// -----------------------------------------------------------------------------
// Section: PortalChunkItem
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PortalChunkItem::PortalChunkItem( ChunkBoundary::Portal * pPortal ) :
	pPortal_( pPortal )
{
}


/**
 *	This methods creates the collision obstacles of a portal.
 */
void PortalChunkItem::toss( Chunk * pChunk )
{
	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).addObstacle(
				new PortalObstacle( this ) );
	}
}

#ifndef MF_SERVER

/*
 *  This class is a helper class for rendering PortalChunkItem
 */
class PortalDrawItem : public Moo::DrawContext::UserDrawItem
{
public:
	PortalDrawItem( const Vector3* pRect, uint32 colour )
	: colour_( colour )
	{
		BW_GUARD;
		memcpy( rect_, pRect, sizeof(Vector3) * 4 );
	}
	void draw()
	{
		BW_GUARD;
		Moo::Material::setVertexColour();
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );

		Moo::rc().setFVF( Moo::VertexXYZNDS::fvf() );
		Moo::rc().setVertexShader( NULL );
		Moo::rc().device()->SetPixelShader( NULL );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
		Moo::rc().device()->SetTransform( D3DTS_VIEW, &Matrix::identity );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );

		Moo::VertexXYZNDS pVerts[4];

		for (int i = 0; i < 4; i++)
		{
			pVerts[i].colour_ = colour_;
			pVerts[i].specular_ = 0xffffffff;
		}
		pVerts[0].pos_ = rect_[0];
		pVerts[1].pos_ = rect_[1];
		pVerts[2].pos_ = rect_[3];
		pVerts[3].pos_ = rect_[2];

		Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, pVerts, sizeof( pVerts[0] ) );
	}
	void fini()
	{
		delete this;
	}
private:
	Vector3 rect_[4];
	uint32 colour_;
};

namespace
{
	/*
	 *	Helper class for watcher value for draw portals
	 */
	class ShouldDrawChunkPortals
	{
	public:
		ShouldDrawChunkPortals() :
		  state_( false )
		{
			MF_WATCH( "Render/Draw PortalChunkItems", state_ );
		}
		bool state_;

	};
}

/**
 *	Draw method to debug portal states
 */
void PortalChunkItem::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	static ShouldDrawChunkPortals shouldDrawChunkPortals;

	// get out if we don't want to draw them
	if (!shouldDrawChunkPortals.state_) return;

	// get the transformation matrix
	Matrix tran;
	tran.multiply( Moo::rc().world(), Moo::rc().view() );

	// transform all the points
	Vector3 prect[4];
	for (int i = 0; i < 4; i++)
	{
		// project the point straight into clip space
		tran.applyPoint( prect[i], Vector3(
			pPortal_->uAxis * pPortal_->points[i][0] +
			pPortal_->vAxis * pPortal_->points[i][1] +
			pPortal_->origin ) );

	}
	float distance = (prect[0].z + prect[1].z + prect[2].z + prect[3].z) / 4.f;
	PortalDrawItem* drawItem = new PortalDrawItem( prect, 
		pPortal_->permissive ? 0xff003300 : 0xff550000 );

	drawContext.drawUserItem( drawItem,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );

}
#else
void PortalChunkItem::draw( Moo::DrawContext& drawContext )
{
}
#endif

BW_END_NAMESPACE

// portal_chunk_item.cpp
