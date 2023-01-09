#include "pch.hpp"
#include "py_splodge.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "physics2/worldtri.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/time_of_day.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "math/planeeq.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/material.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SplodgeRenderer
// -----------------------------------------------------------------------------


/**
 *	This class accumulates any triangles passed into it
 */
class TriAcc : public CollisionCallback
{
public:
	VectorNoDestructor<WorldTriangle> tris_;
private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		WorldTriangle wt(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ),
			triangle.flags() );

		// only add it if we don't already have it
		// (not sure why this happens, but it does)
		size_t sz = tris_.size();
		uint i;
		for (i = 0; i < sz; i++)
		{
			if (tris_[i].v0() == wt.v0() &&
				tris_[i].v1() == wt.v1() &&
				tris_[i].v2() == wt.v2()) break;
		}
		if (i >= sz) tris_.push_back( wt );

		return COLLIDE_ALL;
	}
};

/**
 *	This global method specifies the resources required by this file
 */
static AutoConfigString s_pySplodge_mfmName( "environment/splodgeMaterial" );

/**
 *	This class stores and renders splodges
 */
class SplodgeRenderer : public MainLoopTask
{
public:
	SplodgeRenderer() : splodgeMat_( NULL )
	{ MainLoopTasks::root().add( this, "Flora/Splodges", ">App", NULL ); }

	//~SplodgeRenderer()
	//{ MainLoopTasks::root().del( this, "Flora/Splodges" ); }

	/**
	 *	MainLoopTask init method
	 */
	virtual bool init()
	{
		BW_GUARD;
		splodgeMat_ = new Moo::Material();
		splodgeMat_->load( s_pySplodge_mfmName );

		return true;
	}

	/**
	 *	MainLoopTask fini method
	 */
	virtual void fini()
	{
		BW_GUARD;
		vxs_.clear();

		splodges_.clear();

		bw_safe_delete(splodgeMat_);
	}


	virtual void draw();

	void storeSplodge( const Vector3 * quad, const Vector3 & translation,
		uint32 colour = 0x88888888 );

private:

	Moo::Material	* splodgeMat_;

	struct SplodgeRecord
	{
		Vector3	quad[4];
		Vector3	translation;
		uint32	colour;
	};

	VectorNoDestructor<SplodgeRecord>	splodges_;

	CustomMesh<Moo::VertexXYZDUV>	vxs_;
	TriAcc triac_;
};



/**
 *	MainLoopTask draw method
 */
void SplodgeRenderer::draw()
{
	BW_GUARD;
	if (!ChunkManager::instance().cameraSpace().exists())
		return;

	vxs_.clear();

	size_t nsp = splodges_.size();
	for (size_t i=0; i < nsp; i++)
	{
		SplodgeRecord & rec = splodges_[i];

		// find the plane of our triangle
		PlaneEq	plane( rec.quad[0], rec.quad[1], rec.quad[2] );

		// collide the volume formed by moving the quadrilateral
		// 'rec.quad' through 'rec.translation' through the scene
		float	avgdim = (rec.quad[2] - rec.quad[0]).length();
		Vector3	source = (rec.quad[0] + rec.quad[2]) / 2.f;
		Vector3	extent = source + rec.translation;

		triac_.tris_.clear();

		Vector3 torg = rec.quad[0];
		Vector3 twid = rec.quad[1] - torg;
		Vector3 thgt = rec.quad[3] - torg;
		WorldTriangle	wt( torg, torg + twid*2.f, torg + thgt*2.f );
		ChunkManager::instance().cameraSpace()->collide(
			wt, wt.v0() + rec.translation, triac_ );

		if (!triac_.tris_.size()) continue;

		Vector3		tdirA = rec.quad[1] - rec.quad[0];
		Vector3		tdirB = rec.quad[3] - rec.quad[0];
		tdirA /= tdirA.lengthSquared();
		tdirB /= tdirB.lengthSquared();

		Moo::VertexXYZDUV	vert;

		float normalDotDir = plane.normal().dotProduct( rec.translation );

		float	sumpdst = 0.f;

		// draw all these triangles again
		for (uint j = 0; j < triac_.tris_.size(); j++)
		{
			WorldTriangle	* wt = &triac_.tris_[j];

			if (wt->isTransparent())
			{
				continue;
			}

			// project the triangle back onto rec.quad
			float pdist[3] = {
				plane.intersectRayHalf( wt->v0(), normalDotDir ),
				plane.intersectRayHalf( wt->v1(), normalDotDir ),
				plane.intersectRayHalf( wt->v2(), normalDotDir ) };
			Vector3	onp[3] = {
				wt->v0() + rec.translation * pdist[0] - rec.quad[0],
				wt->v1() + rec.translation * pdist[1] - rec.quad[0],
				wt->v2() + rec.translation * pdist[2] - rec.quad[0] };

			// figure out its texture coords there
			Vector2 tex[3] = {
				Vector2( tdirA.dotProduct( onp[0] ), tdirB.dotProduct( onp[0] ) ),
				Vector2( tdirA.dotProduct( onp[1] ), tdirB.dotProduct( onp[1] ) ),
				Vector2( tdirA.dotProduct( onp[2] ), tdirB.dotProduct( onp[2] ) ) };

			// find the average colour (the polygons are too big to blend
			// the colour over them - the colour often wants to exceed its limits)
			PlaneEq	tpeq( wt->v0(), wt->v1(), wt->v2() );
			float avgdist = (tpeq.intersectRay( source, rec.translation ) -
				source).length();
			uint32 ocol = uint32( uint8(rec.colour) *
				(1-Math::clamp(0.f,avgdist / rec.translation.length(),1.f)) );

			// now make some vertices, adding a small offset to lift splodge off
			// surface.
			Vector3 offset = wt->normal();
			offset.normalise();
			offset *= 0.01f;

			for (int k = 0; k < 3; k++)
			{
				vert.pos_		= wt->v(k) + offset;
				vert.colour_	= (ocol << 24) | (ocol << 16) | (ocol << 8) | (ocol << 0);
				vert.uv_.set( tex[k][0], tex[k][1] );

				vxs_.push_back( vert );
			}

		}
	} 
	splodges_.clear();

	if (!vxs_.size()) return;

	// draw the triangles!
	splodgeMat_->set();

	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	for (uint i=0; i<vxs_.size(); i++) vxs_[i].pos_.y += 0.001f;
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	vxs_.draw();
}


/**
 *	This method saves up a splodge to draw at the appropriate time
 *
 *	If translation points directly up then it is turned into a vector from
 *	 the direction of the sun with the same magnitude, and the input
 *	 quadrilateral is turned to face that direction, assuming it started
 *	 out facing along the positive Z axis.
 */
void SplodgeRenderer::storeSplodge( const Vector3 * quad,
	const Vector3 & translation, uint32 colour )
{
	BW_GUARD;
	SplodgeRecord	rec;

	if (translation.x == 0 && translation.z == 0 && translation.y > 0)
	{
		Vector3 sunDir = ChunkManager::instance().cameraSpace()->enviro().
			timeOfDay()->lighting().mainLightDir();

		rec.translation = translation.y * sunDir;

		Vector3 centre = (quad[0] + quad[2]) / 2.f;

		Matrix	sunRotor;
		sunRotor.setRotateY( atan2f(sunDir[0],sunDir[2]) );
		for (int i=0; i < 4; i++)
		{
			rec.quad[i] = centre + sunRotor.applyPoint( quad[i] - centre );
		}
	}
	else
	{
		rec.quad[0] = quad[0];
		rec.quad[1] = quad[1];
		rec.quad[2] = quad[2];
		rec.quad[3] = quad[3];
		rec.translation = translation;
	}

	rec.colour = colour;

	splodges_.push_back( rec );
}



static SplodgeRenderer s_splodges;



// -----------------------------------------------------------------------------
// Section: PySplodge
// -----------------------------------------------------------------------------

/*~ attribute PySplodge size
 *  The x and y components of this describe the height and width of the object
 *  casting the splodge's shadow. This is used to calculate the size and shape
 *  of the splodge as it is drawn on the ground. The z component has no effect.
 *  @type Read-Write Vector3.
 */
/*~ attribute PySplodge maxLod
 *  The maximum distance from a camera at which this will be drawn.
 *  @type Read-Write float.
 */
PY_TYPEOBJECT( PySplodge )

PY_BEGIN_METHODS( PySplodge )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySplodge )
	PY_ATTRIBUTE( size )
	PY_ATTRIBUTE( maxLod )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Splodge
 *  Creates a new PySplodge object which is ready to be attached to a node on a
 *  model. This draws a "splodge" shadow on the ground below the point where 
 *  it is attached.
 *
 *  @return A new PySplodge.
 */
PY_FACTORY_NAMED( PySplodge, "Splodge", BigWorld )


/**
 *	Constructor
 */
PySplodge::PySplodge( Vector3 bbSize, PyTypeObject * pType ) :
	PyAttachment( pType ),
	bbSize_( bbSize ),
	lod_( 0.0f ),
	maxLod_( 50.f ),
	outsideNow_( true )
{
}


/**
 *	Check if this if visible or lodded out.
 *	@return true if this object is visible, false if lodded out.
 */
bool PySplodge::isLodVisible() const
{
	BW_GUARD;

	// Infinite lod
	if (maxLod_ <= 0.0f)
	{
		return true;
	}

	if (lod_ <= maxLod_)
	{
		return true;
	}
	
	return false;
}


/**
 *	Set the LOD for this frame.
 *	@param worldTransform the world transform for this object.
 *	@param lod the lod for drawing.
 */
void PySplodge::updateAnimations( const Matrix & worldTransform,
	float lod )
{
	BW_GUARD;
	lod_ = lod;
}


/**
 *	The draw method
 */
void PySplodge::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD;

	// See if we want to draw
	if (!this->isLodVisible())
	{
		return;
	}
	if (s_splodgeIntensity_ <= 0) return;
	if (!outsideNow_) return;
	if (s_ignoreSplodge_) return;

	// And draw it then
	uint32 splodgeColour = 0xff000000UL | (s_splodgeIntensity_ << 16) |
		(s_splodgeIntensity_ << 8) | (s_splodgeIntensity_);

	if (bbSize_.x + bbSize_.y + bbSize_.z > 0.f)
	{
		Vector3 base = worldTransform.applyToOrigin();
		Vector3 pts[4] = {
			base + Vector3( -bbSize_.x/2, 0, 0 ),
			base + Vector3( bbSize_.x/2, 0, 0 ),
			base + Vector3( bbSize_.x/2, bbSize_.y, 0 ),
			base + Vector3( -bbSize_.x/2, bbSize_.y, 0 ) };

		s_splodges.storeSplodge( pts, Vector3( 0, 10, 0 ), splodgeColour );
	}
	else	// leglike shadow
	{
		Vector3 footPos = worldTransform.applyToOrigin();
		Vector3 pts[4] = {
			footPos + Vector3( -.15f, -.1f, 0 ),
			footPos + Vector3( .15f, -.1f, 0 ),
			footPos + Vector3( .15f, 0.6f, 0 ),
			footPos + Vector3( -.15f, 0.6f, 0 ) };

		s_splodges.storeSplodge( pts, Vector3( 0, 2, 0 ), splodgeColour );
	}
}


/**
 *	Tossed method
 */
void PySplodge::tossed( bool outside )
{
	outsideNow_ = outside;
}


/**
 *	Factory method
 */
PySplodge * PySplodge::New( Vector3 bbSize )
{
	BW_GUARD;
	return new PySplodge( bbSize );
}


/// Static initialisers
bool PySplodge::s_ignoreSplodge_ = false;
uint32 PySplodge::s_splodgeIntensity_ = 160;

BW_END_NAMESPACE

// py_splodge.cpp
