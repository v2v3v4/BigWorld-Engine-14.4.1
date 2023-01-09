#include "pch.hpp"
#include "py_spot_light.hpp"

#include <space/space_manager.hpp>
#include <space/light_embodiment.hpp>

namespace BW {

// -----------------------------------------------------------------------------
// Section: PySpotLight
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PySpotLight )

/*~ function PySpotLight recalc
 *  Recalculates the effects of the light. The initial setup of a PySpotLight
 *  does not require this to be called.
 *  @return None.
 */
PY_BEGIN_METHODS( PySpotLight )
	PY_METHOD( recalc )
PY_END_METHODS()

/*~ attribute PySpotLight visible
 *  Dictates whether this light illuminates things.
 *  @type Read-Write boolean
 */
/*~ attribute PySpotLight position
 *  The centre of the light source. This is overwritten with the output of the
 *  source attribute if it has been specified.
 *  @type Read-Write Vector3
 */
/*~ attribute PySpotLight direction
 *  The direction of the light source. This is overwritten with the output of the
 *  source attribute if it has been specified.
 *  @type Read-Write Vector3
 */
/*~ attribute PySpotLight colour
 *  The colour of the light source in RGBA form. This is overwritten with the 
 *  output of the shader attribute if it has been specified.
 *  @type Read-Write Vector4
 */
/*~ attribute PySpotLight innerRadius
 *  The radius around the centre of the light within which the strength of the
 *  light does not drop off. This is overwritten with the first component of
 *  the output of the bounds attribute if it has been specified.
 *  @type Read-Write Float
 */
/*~ attribute PySpotLight outerRadius
 *  If greater than innerRadius, this is the radius around the centre of the
 *  light at which the strength of the light becomes zero. If this is less than
 *  or equal to innerRadius, then there is no area around the light in which
 *  its effect dissipates. This is overwritten with the second component of the
 *  output of the bounds attribute if it has been specified.
 *  @type Read-Write Float
 */
/*~ attribute PySpotLight cosConeAngle
 *  The cos of the cone angle.
 *	1.0 = zero size cone
 *	0.0 = 180 degree cone (90 degrees on each side)
 *  @type Read-Write Float
 */
/*~ attribute PySpotLight source
 *  When specified, this overwrites the position attribute with it's
 *  translation.
 *  @type Read-Write MatrixProvider
 */
/*~ attribute PySpotLight bounds
 *  When specified, this overwrites the innerRadius and outerRadius attributes
 *  with the first and second components of the Vector4 it provides,
 *  respectively.
 *  @type Read-Write Vector4Provider
 */
/*~ attribute PySpotLight shader
 *  When specified, this overwrites the colour attribute with the Vector4 it
 *  provides.
 *  @type Read-Write Vector4Provider
 */
/*~ attribute PySpotLight priority
 *	A priority bias used when sorting lights to determine the most
 *	relevant set of lights for a particular object. This is useful
 *	to ensure important lights will always be considered before other
 *	lights (e.g. a player's torch).
 *  @type Read-Write Integer
 */
PY_BEGIN_ATTRIBUTES( PySpotLight )
	PY_ATTRIBUTE( visible )
	PY_ATTRIBUTE( position )
	PY_ATTRIBUTE( direction )
	PY_ATTRIBUTE( colour )
	PY_ATTRIBUTE( priority )
	PY_ATTRIBUTE( innerRadius )
	PY_ATTRIBUTE( outerRadius )
	PY_ATTRIBUTE( cosConeAngle )
	PY_ATTRIBUTE( source )
	PY_ATTRIBUTE( bounds )
	PY_ATTRIBUTE( shader )
PY_END_ATTRIBUTES()

/*~ function BigWorld PySpotLight
 *  Creates a new PySpotLight. This is a script controlled spot light which can
 *  be used as a light source for models and shells.
 */
PY_FACTORY( PySpotLight, BigWorld )

// And set up the older python name for backwards compatibility with scripts.
PY_TYPEOBJECT( PyChunkSpotLight )
PY_BEGIN_METHODS( PyChunkSpotLight )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyChunkSpotLight )
PY_END_ATTRIBUTES()
PY_FACTORY( PyChunkSpotLight, BigWorld )

PyChunkSpotLight::PyChunkSpotLight( PyTypeObject * pType )
	: PySpotLight(pType)
{
}

PyChunkSpotLight::~PyChunkSpotLight()
{
}

/**
 *	Constructor.
 */
PySpotLight::PySpotLight( PyTypeObject * pType )
	: PyObjectPlusWithVD( pType )
{
	pLightEmbodiment_ = SpaceManager::instance().createSpotLightEmbodiment(*this);
}


/**
 *	Destructor.
 */
PySpotLight::~PySpotLight()
{
	bw_safe_delete( pLightEmbodiment_ );
}


//---------------------------------------------------------
bool PySpotLight::visible() const
{
	return pLightEmbodiment_->visible();
}

void PySpotLight::visible( bool v )
{
	pLightEmbodiment_->visible( v );
}

//---------------------------------------------------------
const Vector3 & PySpotLight::position() const
{
	return pLightEmbodiment_->position();
}

void PySpotLight::position( const Vector3 & npos )
{
	pLightEmbodiment_->position( npos );
}

//---------------------------------------------------------
const Vector3 & PySpotLight::direction() const
{
	return pLightEmbodiment_->direction();
}

void PySpotLight::direction( const Vector3 & dir )
{
	pLightEmbodiment_->direction( dir );
}

//---------------------------------------------------------
const Moo::Colour & PySpotLight::colour() const
{
	return pLightEmbodiment_->colour();
}

void PySpotLight::colour( const Moo::Colour & v )
{
	pLightEmbodiment_->colour( v );
}

//---------------------------------------------------------
int PySpotLight::priority() const
{
	return pLightEmbodiment_->priority();
}

void PySpotLight::priority( int v )
{
	pLightEmbodiment_->priority( v );
}

//---------------------------------------------------------
float PySpotLight::innerRadius() const
{
	return pLightEmbodiment_->innerRadius();
}

void PySpotLight::innerRadius( float v )
{
	pLightEmbodiment_->innerRadius( v );
}

//---------------------------------------------------------
float PySpotLight::outerRadius() const
{
	return pLightEmbodiment_->outerRadius();
}

void PySpotLight::outerRadius( float v )
{
	pLightEmbodiment_->outerRadius( v );
}

//---------------------------------------------------------
float PySpotLight::cosConeAngle() const
{
	return pLightEmbodiment_->cosConeAngle();
}

void PySpotLight::cosConeAngle( float v )
{
	pLightEmbodiment_->cosConeAngle( v );
}

//---------------------------------------------------------
void PySpotLight::recalc()
{
	pLightEmbodiment_->recalc();
}

} // namespace BW

// py_spot_light.cpp
