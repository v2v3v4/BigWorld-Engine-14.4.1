#include "pch.hpp"

#include "spot_light_embodiment.hpp"

#include "compiled_space.hpp"

#include "math/convex_hull.hpp"
#include "math/boundbox.hpp"
#include "moo/light_container.hpp"
#include "moo/moo_math.hpp"
#include "moo/debug_draw.hpp"
#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/dynamic_light_scene_provider.hpp"

namespace BW {

typedef CompiledSpace::CompiledSpace* CompiledSpacePtr;

SpotLightEmbodiment::SpotLightEmbodiment( const PySpotLight & parent ) :
	parent_( parent ),
	pLight_( new Moo::SpotLight() ),
	visible_( false )
{
}

SpotLightEmbodiment::~SpotLightEmbodiment()
{
	doLeaveSpace();
}

bool SpotLightEmbodiment::doIntersectAndAddToLightContainer(
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

bool SpotLightEmbodiment::doIntersectAndAddToLightContainer(
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

void SpotLightEmbodiment::doEnterSpace( ClientSpacePtr pSpace )
{
	pEnteredSpace_ = static_cast<CompiledSpaceRawPtr>( pSpace.get() );
	pEnteredSpace_->dynamicLightScene().addLightEmbodiment( this );
}

void SpotLightEmbodiment::doLeaveSpace()
{
	if (pEnteredSpace_)
	{
		pEnteredSpace_->dynamicLightScene().removeLightEmbodiment( this );
		pEnteredSpace_ = NULL;
	}
}

bool SpotLightEmbodiment::visible() const
{
	return visible_;
}

void SpotLightEmbodiment::visible( bool v )
{
	// Should we be adding and taking it away from the dynamic light scene
	// provider?
	visible_ = v;
}


const Vector3 & SpotLightEmbodiment::position() const
{
	return pLight_->worldPosition();
}

void SpotLightEmbodiment::position( const Vector3 & npos )
{
	if (!pEnteredSpace_)
	{
		doEnterSpace( DeprecatedSpaceHelpers::cameraSpace() );
	}

	pLight_->position( npos );
	pLight_->worldTransform( Matrix::identity );
}

void SpotLightEmbodiment::doUpdateLocation()
{
	if (parent_.source())
	{
		Matrix m;
		parent_.source()->matrix( m );
		position( m.applyToOrigin() );
		direction( m.applyToUnitAxisVector(2) );
	}
}

const Vector3 & SpotLightEmbodiment::direction() const
{
	return pLight_->worldDirection();
}

void SpotLightEmbodiment::direction( const Vector3 & dir )
{
	pLight_->direction( dir );
	pLight_->worldTransform( Matrix::identity );
}

const Moo::Colour & SpotLightEmbodiment::colour() const
{
	return pLight_->colour();
}

void SpotLightEmbodiment::colour( const Moo::Colour & v )
{
	pLight_->colour( v );
}

int SpotLightEmbodiment::priority() const
{
	return pLight_->priority();
}

void SpotLightEmbodiment::priority( int v )
{
	pLight_->priority( v );
}

float SpotLightEmbodiment::innerRadius() const
{
	return pLight_->innerRadius();
}

void SpotLightEmbodiment::innerRadius( float v )
{
	pLight_->innerRadius( v );
}

float SpotLightEmbodiment::outerRadius() const
{
	return pLight_->outerRadius();
}

void SpotLightEmbodiment::outerRadius( float v )
{
	pLight_->outerRadius( v );
}

float SpotLightEmbodiment::cosConeAngle() const
{
	return pLight_->cosConeAngle();
}

void SpotLightEmbodiment::cosConeAngle( float v )
{
	pLight_->cosConeAngle( v );
}

void SpotLightEmbodiment::doTick( float dTime )
{
	// update inner and outer radii
	if (parent_.bounds())
	{
		Vector4 val;
		parent_.bounds()->output( val );
		if (val.x > val.y && val.x > 0.001 && val.y < 500.0)
		{
			pLight_->innerRadius( val.x );
			pLight_->outerRadius( val.y );
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

void SpotLightEmbodiment::drawBBox() const
{
	if (!visible_)
	{
		return;
	}
	BoundingBox bb;
	bb.addBounds(pLight_->position());
	const float outerRadius = pLight_->outerRadius();
	bb.expandSymmetrically(outerRadius, outerRadius, outerRadius);
	DebugDraw::bboxAdd( bb, Moo::PackedColours::RED );
}


} // namespace BW

