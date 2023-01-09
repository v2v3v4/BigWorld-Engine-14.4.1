#ifndef CHUNK_SPOT_LIGHT_EMBODIMENT_HPP
#define CHUNK_SPOT_LIGHT_EMBODIMENT_HPP

#include "chunk/chunk_light.hpp"
#include "moo/spot_light.hpp"

#include "space/light_embodiment.hpp"

#include "romp/py_spot_light.hpp"

BW_BEGIN_NAMESPACE

class ChunkSpotLightEmbodiment
	: public ChunkSpotLight
	, public ISpotLightEmbodiment
{
public:
	ChunkSpotLightEmbodiment( const PySpotLight & parent );
	virtual ~ChunkSpotLightEmbodiment();

	// ILightEmbodiment interfaces...
	bool doIntersectAndAddToLightContainer( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;
	bool doIntersectAndAddToLightContainer( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void doTick( float dTime );
	
	//incRef, decRef, and refCount mentioned below in ChunkItem overrides section.
	// --

	// ISpotLight interfaces...
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
	// --

	// ChunkItem overrides
	void tick( float dTime );
	void nest( ChunkSpace * pSpace );
    // The following methods are also ILightEmbodiment overrides.
	// All reference counting is maintained by the parent python-object.
	void incRef()  const { parent_.incRef(); }
	void decRef()  const { parent_.decRef(); }
	int refCount() const { return parent_.refCount(); }

private:

	virtual void doLeaveSpace();
	virtual void doUpdateLocation();

	void radiusUpdated( float orad );

	const PySpotLight & parent_;
	bool visible_;
};

BW_END_NAMESPACE

#endif // CHUNK_SPOT_LIGHT_EMBODIMENT_HPP

