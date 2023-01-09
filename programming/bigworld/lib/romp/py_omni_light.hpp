#ifndef PY_OMNI_LIGHT_HPP
#define PY_OMNI_LIGHT_HPP

#include "py_light_common.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "space/light_embodiment.hpp"

namespace BW {

class IOmniLight
{
public:
	virtual bool visible() const = 0;
	virtual void visible( bool v ) = 0;

	virtual const Vector3 & position() const = 0;
	virtual void position( const Vector3 & v ) = 0;

	virtual const Moo::Colour & colour() const = 0;
	virtual void colour( const Moo::Colour & v ) = 0;

	virtual float innerRadius() const = 0;
	virtual void innerRadius( float v ) = 0;

	virtual float outerRadius() const = 0;
	virtual void outerRadius( float v ) = 0;

	virtual int priority() const = 0;
	virtual void priority( int v ) = 0;

	virtual void recalc() = 0;;

};

class IOmniLightEmbodiment
	: public IOmniLight
	, public ILightEmbodiment
{
};

/*~ class BigWorld.PyOmniLight
 *  This is a script controlled omni light which can be used as a diffuse 
 *  and/or specular light source for models and shells. This is not the class 
 *  used for environmental lights or lights which have been placed in the 
 *  editor.
 *
 *  The colour, radius, and position properties of the light can be set to
 *  static values via the colour, innerRadius and outerRadius, and position
 *  attributes. Alternatively, They can be set to dynamic values via the
 *  source, bounds, and shader attributes. These will override the static
 *  values when specified.  This gives you the ability to dynamically
 *  alter the light's look, or attach it to moving models in the scene.
 *
 *  The strength of a PyOmniLight at a point is determined by it's
 *  innerRadius, outerRadius, and the distance between the point and
 *  the PyOmniLight. If the distance is less than innerRadius, then the light
 *  is applied at full strength (regardless of outerRadius). The strength of
 *  the light falls off linearly from full to 0 between innerRadius and
 *  outerRadius, and is 0 from outerRadius onwards.
 *
 *  Code Example:
 *  @{
 *  # This sets up a few PyOmniLight objects which show some of the things
 *  # that they can do.
 *
 *  # import Math module
 *  import Math
 *
 *  # create a simple diffuse light source at position (0, 0, 0)
 *  simpleDiffuse = BigWorld.PyOmniLight()
 *  simpleDiffuse.innerRadius = 1
 *  simpleDiffuse.outerRadius = 3
 *  simpleDiffuse.position = (0, 0, 0)
 *  simpleDiffuse.colour = (255, 255, 255, 0) # white
 *  simpleDiffuse.specular = 0
 *  simpleDiffuse.diffuse = 1
 *  simpleDiffuse.visible = True
 *
 *  # create a simple specular light source
 *  simpleSpecular = BigWorld.PyOmniLight()
 *  simpleSpecular.innerRadius = 1
 *  simpleSpecular.outerRadius = 3
 *  simpleSpecular.position = (5, 0, 0)
 *  simpleSpecular.colour = (255, 255, 255, 0) # white
 *  simpleSpecular.specular = 1
 *  simpleSpecular.diffuse = 0
 *  simpleSpecular.visible = True
 *
 *  # create a light source which does not disperse
 *  noDissipate = BigWorld.PyOmniLight()
 *  noDissipate.innerRadius = 2
 *  noDissipate.outerRadius = 1.5
 *  noDissipate.position = (10, 0, 0)
 *  noDissipate.colour = (255, 255, 255, 0) # white
 *  noDissipate.specular = 1
 *  noDissipate.diffuse = 1
 *  noDissipate.visible = True
 *
 *  # create a light source which follows the player entity
 *  followPlayer = BigWorld.PyOmniLight()
 *  followPlayer.innerRadius = 1
 *  followPlayer.outerRadius = 3
 *  followPlayer.source = BigWorld.player().matrix
 *  followPlayer.colour = (255, 255, 255, 0) # white
 *  followPlayer.specular = 1
 *  followPlayer.diffuse = 1
 *  followPlayer.visible = True
 *
 *  # create a light source which is attached to the player's hand
 *  # NB: the hand node is assumed to be called "HP_right_hand" in this
 *  # example.  Examine the model in ModelViewer to determine the node name
 *  # in your model.
 *  followPlayerHand = BigWorld.PyOmniLight()
 *  followPlayerHand.innerRadius = 1
 *  followPlayerHand.outerRadius = 3
 *  followPlayerHand.source = BigWorld.player().model.node("HP_right_hand")
 *  followPlayerHand.colour = (255, 255, 255, 0) # white
 *  followPlayerHand.specular = 1
 *  followPlayerHand.diffuse = 1
 *  followPlayerHand.visible = True
 *
 *  # create a light source which changes colour over time at position 
 *  # (15, 0, 0)
 *  pulseShader = Math.Vector4Animation()
 *  pulseShader.duration = 1.0
 *  pulseShader.keyframes = [ ( 0.0, (200, 0,   0, 0) ),    # red at 0.0s
 *                            ( 0.5, (0,   0, 200, 0) ),    # blue at 0.5s
 *                            ( 1.0, (200, 0,   0, 0) ) ]   # red at 1.0s
 *  pulse = BigWorld.PyOmniLight()
 *  pulse.innerRadius = 1
 *  pulse.outerRadius = 3
 *  pulse.position = (15, 0, 0)
 *  pulse.shader = pulseShader
 *  pulse.specular = 1
 *  pulse.diffuse = 1
 *  pulse.visible = True
 *
 *  # create a light source which changes radius over time
 *  growBounds = Math.Vector4Animation()
 *  growBounds.duration = 1.0
 *  growBounds.keyframes = [ ( 0.0, ( 0, 0, 0, 0 ) ),    # 0 at 0.0s
 *                           ( 0.5, ( 2, 1, 0, 0 ) ),    # 2 at 0.5s
 *                           ( 1.0, ( 0, 0, 0, 0 ) ) ]   # 0 at 1.0s
 *  grow = BigWorld.PyOmniLight()
 *  grow.position = (20, 0, 0)
 *  grow.colour = (255, 255, 255, 0) # white
 *  grow.bounds = growBounds
 *  grow.specular = 1
 *  grow.diffuse = 1
 *  grow.visible = True
 *  @}
 */
/**
 * The intention of the PyOmniLight class is to wrap both chunk-based scripted
 * lights as well as any other dynamic/scripted light.  The encapsulation is
 * defined by the ILightEmbodiment, for which the concrete SpaceFactory type is
 * used to create the correct concrete light embodiment type.
 */
class PyOmniLight
	: public PyObjectPlusWithVD
	, public IOmniLight
{
	Py_Header( PyOmniLight, PyObjectPlusWithVD )

public:
	PyOmniLight( PyTypeObject * pType = & s_type_);
	virtual ~PyOmniLight();

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visible, visible )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Moo::Colour, colour, colour )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, priority, priority )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, innerRadius, innerRadius )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, outerRadius, outerRadius )

	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )
	PY_RW_ATTRIBUTE_DECLARE( pBounds_, bounds )
	PY_RW_ATTRIBUTE_DECLARE( pShader_, shader )

	PY_AUTO_METHOD_DECLARE( RETVOID, recalc, END )

	static PyObject * New() { return new PyOmniLight(); }
	PY_AUTO_FACTORY_DECLARE( PyOmniLight, END )

	bool visible() const;
	void visible( bool v );

	const Vector3 & position() const;
	void position( const Vector3 & v );

	const Moo::Colour & colour() const;
	void colour( const Moo::Colour & v );

	int priority() const;
	void priority( int v );

	float innerRadius() const;
	void innerRadius( float v );

	float outerRadius() const;
	void outerRadius( float v );

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

	IOmniLightEmbodiment * pLightEmbodiment_;

	PyOmniLight( const PyOmniLight& );
	PyOmniLight& operator=( const PyOmniLight& );

};

/*~ class BigWorld.PyChunkLight
 * For backwards compatibility, so that the old python name will work.
 * See PyOmniLight documentation for more details and example usage.
 */
class PyChunkLight : public PyOmniLight
{
	Py_Header( PyChunkLight, PyOmniLight )
public:
	PyChunkLight( PyTypeObject * pType = & s_type_);
	virtual ~PyChunkLight();
	static PyObject * New() { return new PyChunkLight(); }
	PY_AUTO_FACTORY_DECLARE( PyChunkLight, END )
private:
	PyChunkLight( const PyChunkLight& );
	PyChunkLight& operator=( const PyChunkLight& );
};

} // namespace BW

#endif // PY_OMNI_LIGHT_HPP
