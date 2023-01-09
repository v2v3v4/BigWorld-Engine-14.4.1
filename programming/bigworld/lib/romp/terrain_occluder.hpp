#ifndef TERRAIN_OCCLUDER_HPP
#define TERRAIN_OCCLUDER_HPP

#include <iostream>

#include "photon_occluder.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class TerrainOccluder : public PhotonOccluder
{
public:
	TerrainOccluder();
	~TerrainOccluder();

	virtual float collides(
			const Vector3 & photonSourcePosition,
			const Vector3 & cameraPosition,
			const LensEffect& le );

private:
	TerrainOccluder(const TerrainOccluder&);
	TerrainOccluder& operator=(const TerrainOccluder&);

	friend std::ostream& operator<<(std::ostream&, const TerrainOccluder&);
};

#ifdef CODE_INLINE
#include "terrain_occluder.ipp"
#endif

BW_END_NAMESPACE

#endif
/*terrain_occluder.hpp*/
