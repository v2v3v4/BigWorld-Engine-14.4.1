#ifndef PHOTON_OCCLUDER_HPP
#define PHOTON_OCCLUDER_HPP

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

class LensEffect;

/**
 *	This class defines the interface to all photon occluders, meaning
 *	those classes that can return yes or no : 
 *	I am in the way of the light source.
 */
class PhotonOccluder
{
public:
	PhotonOccluder()			{};
	virtual ~PhotonOccluder()	{};

	virtual float collides(
			const Vector3 & photonSourcePosition,
			const Vector3 & cameraPosition,
			const LensEffect& le ) = 0;
	virtual void beginOcclusionTests()	{};
	virtual void endOcclusionTests()	{};

private:
	PhotonOccluder(const PhotonOccluder&);
	PhotonOccluder& operator=(const PhotonOccluder&);
};

#ifdef CODE_INLINE
#include "photon_occluder.ipp"
#endif

BW_END_NAMESPACE


#endif // PHOTON_OCCLUDER_HPP
/*photon_occluder.hpp*/
