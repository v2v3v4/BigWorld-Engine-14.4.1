#include "pch.hpp"

#include "omni_light_embodiment.hpp"

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


OmniLightEmbodiment::OmniLightEmbodiment( const PyOmniLight & parent ) :
	parent_( parent ),
	pLight_( new Moo::OmniLight() ),
	visible_( false )
{
}

OmniLightEmbodiment::~OmniLightEmbodiment()
{
	doLeaveSpace();
}

bool OmniLightEmbodiment::doIntersectAndAddToLightContainer(
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

bool OmniLightEmbodiment::doIntersectAndAddToLightContainer(
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

void OmniLightEmbodiment::doEnterSpace( ClientSpacePtr pSpace )
{
	pEnteredSpace_ = static_cast<CompiledSpaceRawPtr>( pSpace.get() );
	pEnteredSpace_->dynamicLightScene().addLightEmbodiment( this );
}

void OmniLightEmbodiment::doLeaveSpace()
{
	if (pEnteredSpace_)
	{
		pEnteredSpace_->dynamicLightScene().removeLightEmbodiment( this );
		pEnteredSpace_ = NULL;
	}
}

bool OmniLightEmbodiment::visible() const
{
	return visible_;
}

void OmniLightEmbodiment::visible( bool v )
{
	// Should we be adding and taking it away from the dynamic light scene
	// provider?
	visible_ = v;
}


const Vector3 & OmniLightEmbodiment::position() const
{
	return pLight_->worldPosition();
}

void OmniLightEmbodiment::position( const Vector3 & npos )
{
	if (!pEnteredSpace_)
	{
		doEnterSpace( DeprecatedSpaceHelpers::cameraSpace() );
	}

	pLight_->position( npos );
	pLight_->worldTransform( Matrix::identity );
}

void OmniLightEmbodiment::doUpdateLocation()
{
	if (parent_.source())
	{
		Matrix m;
		parent_.source()->matrix( m );
		position( m.applyToOrigin() );
	}
}

const Moo::Colour & OmniLightEmbodiment::colour() const
{
	return pLight_->colour();
}

void OmniLightEmbodiment::colour( const Moo::Colour & v )
{
	pLight_->colour( v );
}

int OmniLightEmbodiment::priority() const
{
	return pLight_->priority();
}

void OmniLightEmbodiment::priority( int v )
{
	pLight_->priority( v );
}

float OmniLightEmbodiment::innerRadius() const
{
	return pLight_->innerRadius();
}

void OmniLightEmbodiment::innerRadius( float v )
{
	pLight_->innerRadius( v );
}

float OmniLightEmbodiment::outerRadius() const
{
	return pLight_->outerRadius();
}

void OmniLightEmbodiment::outerRadius( float v )
{
	pLight_->outerRadius( v );
}

void OmniLightEmbodiment::doTick( float dTime )
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

void OmniLightEmbodiment::drawBBox() const
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

