#ifndef COMPILED_SPACE_LIGHT_EMBODIMENT_HPP
#define COMPILED_SPACE_LIGHT_EMBODIMENT_HPP

#include "forward_declarations.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/singleton.hpp"

BW_BEGIN_NAMESPACE

namespace Moo {
	class LightContainer;
}
BW_END_NAMESPACE

namespace BW
{

class ConvexHull;

class ILightEmbodiment;
typedef SmartPointer< ILightEmbodiment > ILightEmbodimentPtr;

class LightEmbodimentManager : public Singleton<LightEmbodimentManager>
{
public:

	void addLightEmbodiment( ILightEmbodiment* pEmb );
	void removeLightEmbodiment( ILightEmbodiment* pEmb );
	void tick();

private:
	typedef BW::vector<ILightEmbodiment*> LightEmbodiments;
	LightEmbodiments currentLights_;
};

class ILightEmbodiment : private SafeReferenceCount
{
public:
	ILightEmbodiment();
	virtual ~ILightEmbodiment();

	bool intersectAndAddToLightContainer( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;
	bool intersectAndAddToLightContainer( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void tick( float dTime );
	void leaveSpace();
	void updateLocation();

	virtual void drawBBox() const;

	// So that anyone holding smartpointers to this object can have their
	// inc/decRef requests forwarded on to the right handler.
	// In the case of ILightEmbodiments used in Py*Lights, all inc/decRefs need
	// to be forwarded on to the python side due to the rigid ownership of its
	// ref count. Ideally all ref counts would be centered at the LightEmbodiment
	// but for now this cannot be helped.
	virtual void incRef() const { this->SafeReferenceCount::incRef(); }
	virtual void decRef() const { this->SafeReferenceCount::decRef(); }
	virtual int refCount() const { return this->SafeReferenceCount::refCount(); }

protected:
	virtual bool doIntersectAndAddToLightContainer( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const = 0;
	virtual bool doIntersectAndAddToLightContainer( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const = 0;

	virtual void doTick( float dTime ) = 0;
	virtual void doLeaveSpace() = 0;
	virtual void doUpdateLocation() = 0;
};

} // namespace BW

#endif // COMPILED_SPACE_LIGHT_EMBODIMENT_HPP
