#ifndef ENTITY_FLARE_COLLIDER_HPP
#define ENTITY_FLARE_COLLIDER_HPP


#include <iostream>
#include "romp/photon_occluder.hpp"
#include "entity.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class determines when photons are occluded by an entity.
 *
 *	@see PhotonOccluder
 */
class EntityPhotonOccluder : public PhotonOccluder
{
public:
	EntityPhotonOccluder( Entity & ent );
	~EntityPhotonOccluder();

	float collides(
			const Vector3 & lightSourcePosition,
			const Vector3 & cameraPosition,
			const LensEffect& le );

private:
	float checkTorso(
		const Matrix & objectToClip,
		const Matrix & sunToClip );

	float checkHead(
		const Matrix & objectToClip,
		const Matrix & sunToClip );

	EntityPhotonOccluder(const EntityPhotonOccluder&);
	EntityPhotonOccluder& operator=(const EntityPhotonOccluder&);

	friend std::ostream& operator<<(std::ostream&, const EntityPhotonOccluder&);

	Entity * entity_;
};

#ifdef CODE_INLINE
#include "entity_flare_collider.ipp"
#endif

BW_END_NAMESPACE


#endif
/*entity_flare_collider.hpp*/
