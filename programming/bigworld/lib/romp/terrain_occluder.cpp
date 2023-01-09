
#include "pch.hpp"

#include "terrain_occluder.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "terrain_occluder.ipp"
#endif

TerrainOccluder::TerrainOccluder()
{
	BW_GUARD;
}

TerrainOccluder::~TerrainOccluder()
{
	BW_GUARD;
}


float TerrainOccluder::collides(
		const Vector3 & photonSourcePosition,
		const Vector3 & cameraPosition,
		const LensEffect& le )
{
	BW_GUARD;
	//Check against terrain
	//Vector3	extent = cameraPosition + photonSourcePosition.applyToUnitAxisVector(2) * 500.f;
/*	if (Moo::Terrain::instance().getContact( cameraPosition, photonSourcePosition ) != photonSourcePosition)
	{
		return 0.f;
	}*/

	return 1.f;
}

std::ostream& operator<<(std::ostream& o, const TerrainOccluder& t)
{
	o << "TerrainOccluder\n";
	return o;
}

BW_END_NAMESPACE

/*terrain_occluder.cpp*/
