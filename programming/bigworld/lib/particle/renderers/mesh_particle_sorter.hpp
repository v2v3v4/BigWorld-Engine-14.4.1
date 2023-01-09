#ifndef MESH_PARTICLE_SORTER_HPP
#define MESH_PARTICLE_SORTER_HPP

#include "particle/particle.hpp"
#include "particle/particles.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class sorts mesh particles, which do not have distance members.
 *	It does it by utilising its own storage and providing its own
 *	iterator access.
 */
class MeshParticleSorter
{
public:
	MeshParticleSorter() :
		nParticles_(),
		particleDistConvert_(),
		minDist_(),
		maxDist_(),
		itMeshType_(),
		itIndex_()
	{
	}

	///sort particle list
	void	sortOptimised( Particles::iterator beg, Particles::iterator end );
	void	sort( Particles::iterator beg, Particles::iterator end );

	///return the average distance of all partices
	float	distance() const	{ return (maxDist_ + minDist_) / 2.f; }

	///return the distance to the closest or farthest particle
	float	minDist() const		{ return minDist_; }
	float	maxDist() const		{ return maxDist_; }

	Particle* beginOptimised( Particle* arrayBase )
	{
		itMeshType_ = 0;
		itIndex_ = 0;
		int index = itIndex_ * PARTICLE_MAX_MESHES + itMeshType_;
		if (index < nParticles_)
			return arrayBase + info_[itMeshType_][itIndex_].index_;
		return NULL;
	}

	Particle* nextOptimised( Particle* arrayBase )
	{
		itMeshType_++;
		if (itMeshType_ == PARTICLE_MAX_MESHES)
		{
			itMeshType_ = 0;
			itIndex_++;
		}
		int index = itIndex_ * PARTICLE_MAX_MESHES + itMeshType_;
		if (index < nParticles_)		
			return arrayBase + info_[itMeshType_][itIndex_].index_;
		return NULL;
	}

	Particle* begin( Particle* arrayBase, int& originalType )
	{		
		itIndex_ = 0;	
		if (itIndex_ < nParticles_)
		{
			originalType = info2_[itIndex_].index_ % PARTICLE_MAX_MESHES;
			return arrayBase + info2_[itIndex_].index_;
		}
		return NULL;
	}

	Particle* next( Particle* arrayBase, int& originalType )
	{
		itIndex_++;		
		if (itIndex_ < nParticles_)
		{
			originalType = info2_[itIndex_].index_ % PARTICLE_MAX_MESHES;
			return arrayBase + info2_[itIndex_].index_;
		}
		return NULL;
	}

private:	

	class ParticleInfo
	{
	public:
		ParticleInfo() :
			index_(),
			distance_()
		{
		}

		uint16 index_;
		uint16 distance_;		
		static bool sortReverse( const ParticleInfo& p1, const ParticleInfo& p2 )
		{
			return p1.distance_ > p2.distance_;
		}	
	};	

	typedef VectorNoDestructor< ParticleInfo > InfoVector;
	static InfoVector info_[PARTICLE_MAX_MESHES];
	static InfoVector info2_;	//single larger vector for non optimised meshes.

	uint16		nParticles_;
	float		particleDistConvert_;
	float		minDist_;
	float		maxDist_;
	int			itMeshType_;
	int			itIndex_;	
};

BW_END_NAMESPACE

#endif	//MESH_PARTICLE_SORTER_HPP