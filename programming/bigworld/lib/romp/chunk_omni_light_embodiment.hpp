#ifndef CHUNK_OMNI_LIGHT_EMBODIMENT_HPP
#define CHUNK_OMNI_LIGHT_EMBODIMENT_HPP

#include "chunk/chunk_light.hpp"
#include "moo/omni_light.hpp"

#include "space/light_embodiment.hpp"

#include "romp/py_omni_light.hpp"

BW_BEGIN_NAMESPACE

class ChunkOmniLightEmbodiment
	: public ChunkOmniLight
	, public IOmniLightEmbodiment
{
public:
	ChunkOmniLightEmbodiment( const PyOmniLight & parent );
	virtual ~ChunkOmniLightEmbodiment();

	// ILightEmbodiment interfaces...
	bool doIntersectAndAddToLightContainer( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;
	bool doIntersectAndAddToLightContainer( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void doTick( float dTime );

	//incRef, decRef, and refCount mentioned below in ChunkItem overrides section.
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

	void recalc();
	// --


	// ChunkItem overrides
	void nest( ChunkSpace * pSpace );
	void tick( float dTime );
    // The following methods are also ILightEmbodiment overrides.
	// All reference counting is maintained by the parent python-object.
	void incRef()  const { parent_.incRef(); }
	void decRef()  const { parent_.decRef(); }
	int refCount() const { return parent_.refCount(); }

private:

	virtual void doLeaveSpace();
	virtual void doUpdateLocation();

	void radiusUpdated( float orad );

	const PyOmniLight & parent_;
	bool visible_;
};

BW_END_NAMESPACE

#endif // CHUNK_OMNI_LIGHT_EMBODIMENT_HPP

