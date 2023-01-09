#include "pch.hpp"

#include "photon_occluder.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "photon_occluder.ipp"
#endif

std::ostream& operator<<(std::ostream& o, const PhotonOccluder& t)
{
	o << "PhotonOccluder\n";
	return o;
}

BW_END_NAMESPACE

/*photon_occluder.cpp*/
