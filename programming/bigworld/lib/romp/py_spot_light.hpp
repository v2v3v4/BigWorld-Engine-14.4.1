#ifndef PY_SPOT_LIGHT_HPP
#define PY_SPOT_LIGHT_HPP

#include "py_light_common.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "space/light_embodiment.hpp"

namespace BW {

class ISpotLight
{
public:
	virtual bool visible() const = 0;
	virtual void visible( bool v ) = 0;

	virtual const Vector3 & position() const = 0;
	virtual void position( const Vector3 & v ) = 0;

	virtual const Vector3 & direction() const = 0;
	virtual void direction( const Vector3 & v ) = 0;

	virtual const Moo::Colour & colour() const = 0;
	virtual void colour( const Moo::Colour & v ) = 0;

	virtual float innerRadius() const = 0;
	virtual void innerRadius( float v ) = 0;

	virtual float outerRadius() const = 0;
	virtual void outerRadius( float v ) = 0;

	virtual float cosConeAngle() const = 0;
	virtual void cosConeAngle( float v ) = 0;

	virtual int priority() const = 0;
	virtual void priority( int v ) = 0;

	virtual void recalc() = 0;;

};

class ISpotLightEmbodiment
	: public ISpotLight
	, public ILightEmbodiment
{
};

/*~ class BigWorld.PySpotLight
 *  This is a script controlled spot light which can be used as a diffuse 
 *  and/or specular light source for models and shells. This is not the class 
 *  used for environmental lights or lights which have been placed in the 
 *  editor.
 *
 *  The colour, radius, cone size, direction and position properties of the
 *  light can be set to static values via the colour, innerRadius and
 *  outerRadius, cosConeAngle, direction and position attributes.
 *  Alternatively, They can be set to dynamic values via the source, bounds,
 *  and shader attributes. These will override the static values when
 *  specified.  This gives you the ability to dynamically alter the light's
 *  look, or attach it to moving models in the scene.
 *
 *  The strength of a PySpotLight at a point is determined by it's
 *  innerRadius, outerRadius, and the distance between the point and
 *  the PySpotLight. If the distance is less than innerRadius, then the light
 *  is applied at full strength (regardless of outerRadius). The strength of
 *  the light falls off linearly from full to 0 between innerRadius and
 *  outerRadius, and is 0 from outerRadius onwards.
 *
 *  Code Example:
 *  @{
 *  # This sets up a PySpotLight object which shows some of the things
 *  # that it can do.
 *
 *
 *  # create a light source which is attached to the player's hand
 *  # NB: the hand node is assumed to be called "HP_right_hand" in this
 *  # example.  Examine the model in ModelViewer to determine the node name
 *  # in your model. In the example models this points out from the palm.
 *  followPlayerHand = BigWorld.PySpotLight()
 *  followPlayerHand.innerRadius = 1
 *  followPlayerHand.outerRadius = 10
 *	followPlayerHand.cosConeAngle = 0.9 # ~25 degrees from center
 *  followPlayerHand.source = BigWorld.player().model.node("HP_right_hand")
 *  followPlayerHand.colour = (255, 255, 255, 0) # white
 *  followPlayerHand.specular = 1
 *  followPlayerHand.diffuse = 1
 *  followPlayerHand.visible = True
 *  @}
 */
/**
 * The intention of the PySpotLight class is to wrap both chunk-based scripted
 * lights as well as any other dynamic/scripted light.  The encapsulation is
 * defined by the ILightEmbodiment, for which the concrete SpaceFactory type is
 * used to create the correct concrete light embodiment type.
 */
class PySpotLight
	: public PyObjectPlusWithVD
	, public ISpotLight
{
	Py_Header( PySpotLight, PyObjectPlusWithVD )

public:
	PySpotLight( PyTypeObject * pType = & s_type_);
	virtual ~PySpotLight();

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visible, visible )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, direction, direction )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Moo::Colour, colour, colour )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, priority, priority )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, innerRadius, innerRadius )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, outerRadius, outerRadius )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, cosConeAngle, cosConeAngle )

	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )
	PY_RW_ATTRIBUTE_DECLARE( pBounds_, bounds )
	PY_RW_ATTRIBUTE_DECLARE( pShader_, shader )

	PY_AUTO_METHOD_DECLARE( RETVOID, recalc, END )

	static PyObject * New() { return new PySpotLight(); }
	PY_AUTO_FACTORY_DECLARE( PySpotLight, END )

	bool visible() const;
	void visible( bool v );

	const Vector3 & position() const;
	void position( const Vector3 & v );

	const Vector3 & direction() const;
	void direction( const Vector3 & v );

	const Moo::Colour & colour() const;
	void colour( const Moo::Colour & v );

	int priority() const;
	void priority( int v );

	float innerRadius() const;
	void innerRadius( float v );

	float outerRadius() const;
	void outerRadius( float v );

	float cosConeAngle() const;
	void cosConeAngle( float v );

	void recalc();

	// calling points for derived classes in case they need to increase/decrease
	// the references.
	void incRef() const	{ this->PyObjectPlus::incRef(); }
	void decRef() const	{ this->PyObjectPlus::decRef(); }
	int refCount() const
	{
		return static_cast< int >( this->PyObjectPlus::refCount() );
	}

	const MatrixProviderPtr  & source() const { return pSource_; }
	const Vector4ProviderPtr & bounds() const { return pBounds_; }
	const Vector4ProviderPtr & shader() const { return pShader_; }

private:
	MatrixProviderPtr pSource_;
	Vector4ProviderPtr pBounds_;
	Vector4ProviderPtr pShader_;

	ISpotLightEmbodiment * pLightEmbodiment_;

	PySpotLight( const PySpotLight& );
	PySpotLight& operator=( const PySpotLight& );

};

/*~ class BigWorld.PyChunkSpotLight
 * For backwards compatibility, so that the old python name will work.
 * See PySpotLight documentation for more details and example usage.
 */
class PyChunkSpotLight : public PySpotLight
{
	Py_Header( PyChunkSpotLight, PySpotLight )
public:
	PyChunkSpotLight( PyTypeObject * pType = & s_type_);
	virtual ~PyChunkSpotLight();
	static PyObject * New() { return new PyChunkSpotLight(); }
	PY_AUTO_FACTORY_DECLARE( PyChunkSpotLight, END )
private:
	PyChunkSpotLight( const PyChunkSpotLight& );
	PyChunkSpotLight& operator=( const PyChunkSpotLight& );
};

} // namespace BW

#endif // PY_SPOT_LIGHT_HPP
