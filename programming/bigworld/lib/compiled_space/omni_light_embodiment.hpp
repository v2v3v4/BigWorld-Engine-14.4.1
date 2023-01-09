#ifndef OMNI_LIGHT_EMBODIMENT_HPP
#define OMNI_LIGHT_EMBODIMENT_HPP

#include "moo/omni_light.hpp"
#include "moo/moo_math.hpp"

#include "romp/py_omni_light.hpp"
#include "space/light_embodiment.hpp"

BW_BEGIN_NAMESPACE

class AABB;

namespace Moo {
	class LightContainer;
}

BW_END_NAMESPACE

namespace BW {

class ConvexHull;
typedef SmartPointer< class ClientSpace > ClientSpacePtr;

namespace CompiledSpace
{
	class CompiledSpace;
}

class OmniLightEmbodiment : public IOmniLightEmbodiment
{
	typedef CompiledSpace::CompiledSpace* CompiledSpaceRawPtr;
	typedef SmartPointer<CompiledSpace::CompiledSpace> CompiledSpacePtr;

public:
	OmniLightEmbodiment( const PyOmniLight & parent );
	virtual ~OmniLightEmbodiment();

	// ILightEmbodiment interfaces...
	bool doIntersectAndAddToLightContainer( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;
	bool doIntersectAndAddToLightContainer( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void doTick( float dTime );

	void drawBBox() const;

	// All reference counting is maintained by the parent python-object.
	void incRef() const { parent_.incRef(); };
	void decRef() const { parent_.decRef(); };
	int refCount() const { return parent_.refCount(); }
	// --

	// IOmniLight interfaces...
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

	// Unused. Required for support of the older PyChunkLight interface.
	void recalc() { }
	// --

private:
	void doEnterSpace( ClientSpacePtr pSpace );
	virtual void doLeaveSpace();
	virtual void doUpdateLocation();

	const PyOmniLight & parent_;
	Moo::OmniLightPtr pLight_;
	CompiledSpacePtr pEnteredSpace_;
	bool visible_;
};

} // namespace BW

#endif // OMNI_LIGHT_EMBODIMENT_HPP

