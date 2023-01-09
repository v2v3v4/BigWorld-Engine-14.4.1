#include "pch.hpp"
#include "mesh_particle_sorter.hpp"
#include "moo/render_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

//class static storage.  no need to have multiple copies of these.
MeshParticleSorter::InfoVector MeshParticleSorter::info_[PARTICLE_MAX_MESHES];
MeshParticleSorter::InfoVector MeshParticleSorter::info2_;


/**
 *	This method sorts optimised mesh particles, keeping each particle
 *	in the same part of the vector (treating the vector like MAX_MESHES
 *	interleaved vectors).
 *	This keeps each particle looking the same, and means the renderer
 *	can draw the particles quickly (15 at a time) but will introduce some
 *	sorting error.
 */
void MeshParticleSorter::sortOptimised( Particles::iterator beg, Particles::iterator end )
{
	BW_GUARD;
	//first calculate the distances, and sort our info structure.
	particleDistConvert_ = 65535.f / Moo::rc().camera().farPlane();

	size_t numParticles = end - beg;
	MF_ASSERT( numParticles <= USHRT_MAX );
	nParticles_ = static_cast<uint16>(numParticles);

	if (!nParticles_)
		return;
	
	for (uint32 i=0; i< PARTICLE_MAX_MESHES; i++)
	{
		//resize on a VectorNoDestructor will only
		//ever grow the vector, not shrink it.
		info_[i].resize( (nParticles_/PARTICLE_MAX_MESHES) + 1 );
		info_[i].clear();
	}

	const Matrix& view = Moo::rc().view();
	float distance = 0.f;

	//take a pointer to the array of particles pointed to by beg,
	//hereafter we will use an index into this array.
	Particle* p = &*beg;	
	
	const Vector3& pos = p->position();
	maxDist_ = pos.x * view[0][2] +
		pos.y * view[1][2] +
		pos.z * view[2][2] +
		view[3][2];

	minDist_ = maxDist_;

	ParticleInfo curr;
	
	for (uint16 i = 0; i < nParticles_; i++, p++)	
	{
		curr.index_ = i;

		if (p->isAlive())
		{
			const Vector3& pos = p->position();
			float dist = pos.x * view[0][2] +
				pos.y * view[1][2] +
				pos.z * view[2][2] +
				view[3][2];		

			if (dist <= 0)
				curr.distance_ = 0;
			else if (dist >= Moo::rc().camera().farPlane())
				curr.distance_ = 65535;
			else
				curr.distance_ = uint16(particleDistConvert_ * dist);

			maxDist_ = max( maxDist_, dist );
			minDist_ = min( minDist_, dist );
		}
		else
		{
			//note - do not include dead particles in the min/max range,
			//as doing so could upset sorting the particles w.r.t other objects
			curr.distance_ = 65535;
		}

		info_[i%PARTICLE_MAX_MESHES].push_back(curr);		
	}	

	for (uint32 i=0; i< PARTICLE_MAX_MESHES; i++)
	{		
		std::sort( info_[i].begin(), info_[i].end(), ParticleInfo::sortReverse );				
	}		
}



/**
 *	This method sorts mesh particles, not worrying about keeping each particle
 *	in the same part of the vector (not treating the vector like MAX_MESHES
 *	interleaved vectors).
 *
 *	This produces the best sorting order, but means the renderer either
 *	has to draw the mesh pieces one at a time, or it assumes the mesh visual
 *	used has each piece looking the same.
 */
void MeshParticleSorter::sort( Particles::iterator beg, Particles::iterator end )
{
	BW_GUARD;
	//first calculate the distances, and sort our info structure.
	particleDistConvert_ = 65535.f / Moo::rc().camera().farPlane();

	size_t numParticles = end - beg;
	MF_ASSERT( numParticles <= USHRT_MAX );
	nParticles_ = ( uint16 ) numParticles;

	if (!nParticles_)
		return;
	
	//resize on a VectorNoDestructor will only
	//ever grow the vector, not shrink it.
	info2_.resize( nParticles_ );
	info2_.clear();

	const Matrix& view = Moo::rc().view();
	float distance = 0.f;

	//take a pointer to the array of particles pointed to by beg,
	//hereafter we will use an index into this array.
	Particle* p = &*beg;	
	uint16 i = 0;
	
	const Vector3& pos = p->position();
	maxDist_ = pos.x * view[0][2] +
		pos.y * view[1][2] +
		pos.z * view[2][2] +
		view[3][2];
	minDist_ = maxDist_;

	ParticleInfo curr;
	
	for (i = 0; i < nParticles_; i++, p++)	
	{
		curr.index_ = i;

		if (p->isAlive())
		{
			const Vector3& pos = p->position();
			float dist = pos.x * view[0][2] +
				pos.y * view[1][2] +
				pos.z * view[2][2] +
				view[3][2];		

			if (dist <= 0)
				curr.distance_ = 0;
			else if (dist >= Moo::rc().camera().farPlane())
				curr.distance_ = 65535;
			else
				curr.distance_ = uint16(particleDistConvert_ * dist);

			maxDist_ = max( maxDist_, dist );
			minDist_ = min( minDist_, dist );
		}
		else
		{
			//note - do not include dead particles in the min/max range,
			//as doing so could upset sorting the particles w.r.t other objects
			curr.distance_ = 65535;
		}

		info2_.push_back(curr);		
	}	

	std::sort( info2_.begin(), info2_.end(), ParticleInfo::sortReverse );
}

BW_END_NAMESPACE

// mesh_particle_sorter.cpp
