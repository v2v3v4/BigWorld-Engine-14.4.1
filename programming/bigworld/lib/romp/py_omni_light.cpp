#include "pch.hpp"
#include "py_omni_light.hpp"

#include <space/space_manager.hpp>
#include <space/light_embodiment.hpp>

namespace BW {

// -----------------------------------------------------------------------------
// Section: PyOmniLight
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyOmniLight )

/*~ function PyOmniLight recalc
 *  Recalculates the effects of the light. The initial setup of a PyOmniLight
 *  does not require this to be called.
 *  @return None.
 */
PY_BEGIN_METHODS( PyOmniLight )
	PY_METHOD( recalc )
PY_END_METHODS()

/*~ attribute PyOmniLight visible
 *  Dictates whether this light illuminates things.
 *  @type Read-Write boolean
 */
/*~ attribute PyOmniLight position
 *  The centre of the light source. This is overwritten with the output of the
 *  source attribute if it has been specified.
 *  @type Read-Write Vector3
 */
/*~ attribute PyOmniLight colour
 *  The colour of the light source in RGBA form. This is overwritten with the 
 *  output of the shader attribute if it has been specified.
 *  @type Read-Write Vector4
 */
/*~ attribute PyOmniLight innerRadius
 *  The radius around the centre of the light within which the strength of the
 *  light does not drop off. This is overwritten with the first component of
 *  the output of the bounds attribute if it has been specified.
 *  @type Read-Write Float
 */
/*~ attribute PyOmniLight outerRadius
 *  If greater than innerRadius, this is the radius around the centre of the
 *  light at which the strength of the light becomes zero. If this is less than
 *  or equal to innerRadius, then there is no area around the light in which
 *  its effect dissipates. This is overwritten with the second component of the
 *  output of the bounds attribute if it has been specified.
 *  @type Read-Write Float
 */
/*~ attribute PyOmniLight source
 *  When specified, this overwrites the position attribute with it's
 *  translation.
 *  @type Read-Write MatrixProvider
 */
/*~ attribute PyOmniLight bounds
 *  When specified, this overwrites the innerRadius and outerRadius attributes
 *  with the first and second components of the Vector4 it provides,
 *  respectively.
 *  @type Read-Write Vector4Provider
 */
/*~ attribute PyOmniLight shader
 *  When specified, this overwrites the colour attribute with the Vector4 it
 *  provides.
 *  @type Read-Write Vector4Provider
 */
/*~ attribute PyOmniLight priority
 *	A priority bias used when sorting lights to determine the most
 *	relevant set of lights for a particular object. This is useful
 *	to ensure important lights will always be considered before other
 *	lights (e.g. a player's torch).
 *  @type Read-Write Integer
 */
PY_BEGIN_ATTRIBUTES( PyOmniLight )
	PY_ATTRIBUTE( visible )
	PY_ATTRIBUTE( position )
	PY_ATTRIBUTE( colour )
	PY_ATTRIBUTE( priority )
	PY_ATTRIBUTE( innerRadius )
	PY_ATTRIBUTE( outerRadius )
	PY_ATTRIBUTE( source )
	PY_ATTRIBUTE( bounds )
	PY_ATTRIBUTE( shader )
PY_END_ATTRIBUTES()

/*~ function BigWorld PyOmniLight
 *  Creates a new PyOmniLight. This is a script controlled omni light which can
 *  be used as a light source for models and shells.
 */
PY_FACTORY( PyOmniLight, BigWorld )


// And set up the older python name for backwards compatibility with scripts.
PY_TYPEOBJECT( PyChunkLight )
PY_BEGIN_METHODS( PyChunkLight )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyChunkLight )
PY_END_ATTRIBUTES()
PY_FACTORY( PyChunkLight, BigWorld )


PyChunkLight::PyChunkLight( PyTypeObject * pType )
	: PyOmniLight(pType)
{
}

PyChunkLight::~PyChunkLight()
{
}

/**
 *	Constructor.
 */
PyOmniLight::PyOmniLight( PyTypeObject * pType )
	: PyObjectPlusWithVD( pType )
{
	pLightEmbodiment_ = SpaceManager::instance().createOmniLightEmbodiment(*this);
}


/**
 *	Destructor.
 */
PyOmniLight::~PyOmniLight()
{
	bw_safe_delete( pLightEmbodiment_ );
}


//---------------------------------------------------------
bool PyOmniLight::visible() const
{
	return pLightEmbodiment_->visible();
}

void PyOmniLight::visible( bool v )
{
	pLightEmbodiment_->visible( v );
}

//---------------------------------------------------------
const Vector3 & PyOmniLight::position() const
{
	return pLightEmbodiment_->position();
}

void PyOmniLight::position( const Vector3 & npos )
{
	pLightEmbodiment_->position( npos );
}

//---------------------------------------------------------
const Moo::Colour & PyOmniLight::colour() const
{
	return pLightEmbodiment_->colour();
}

void PyOmniLight::colour( const Moo::Colour & v )
{
	pLightEmbodiment_->colour( v );
}

//---------------------------------------------------------
int PyOmniLight::priority() const
{
	return pLightEmbodiment_->priority();
}

void PyOmniLight::priority( int v )
{
	pLightEmbodiment_->priority( v );
}

//---------------------------------------------------------
float PyOmniLight::innerRadius() const
{
	return pLightEmbodiment_->innerRadius();
}

void PyOmniLight::innerRadius( float v )
{
	pLightEmbodiment_->innerRadius( v );
}

//---------------------------------------------------------
float PyOmniLight::outerRadius() const
{
	return pLightEmbodiment_->outerRadius();
}

void PyOmniLight::outerRadius( float v )
{
	pLightEmbodiment_->outerRadius( v );
}

//---------------------------------------------------------
void PyOmniLight::recalc()
{
	pLightEmbodiment_->recalc();
}

} // namespace BW

// py_omni_light.cpp
