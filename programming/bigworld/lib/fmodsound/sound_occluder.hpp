#ifndef SOUND_OCCLUDER_HPP
#define SOUND_OCCLUDER_HPP


#include "fmod_config.hpp"
#if FMOD_SUPPORT

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_vector.hpp"

namespace FMOD
{
class Geometry;
}

BW_BEGIN_NAMESPACE

class Model;
class SuperModel;
class Vector3;

namespace Terrain
{
    class TerrainHeightMap;
}

/*
    This class wraps FMOD::Geometry objects.
*/
class SoundOccluder
{
public:
    SoundOccluder();
    SoundOccluder( SuperModel * pSuperModel );
    ~SoundOccluder();


    const bool constructed() const { return (geometries_.size() > 0); }

    bool construct( SuperModel * pSuperModel );
    bool construct( Model *pModel );
    bool construct( const Terrain::TerrainHeightMap& map, float blockSize, float directOcclusion, float reverbOcclusion );
    bool setActive( bool active );
    bool update( const Vector3& position, const Vector3& forward = Vector3::zero(), const Vector3& up = Vector3::zero() );
	bool update( const Matrix& transform );

	void debugDraw();

protected:
    BW::vector<FMOD::Geometry *> geometries_;

private:    
};

BW_END_NAMESPACE

#endif //FMOD_SUPPORT

#endif //SOUND_OCCLUDER_HPP
