#ifndef PARTICLE_SYSTEM_DRAW_ITEM_HPP
#define PARTICLE_SYSTEM_DRAW_ITEM_HPP


#include "particle/particle.hpp"
#include "particle/particles.hpp"
#include "math/matrix.hpp"
#include "moo/draw_context.hpp"


BW_BEGIN_NAMESPACE

class ParticleSystemRenderer;

class ParticleSystemDrawItem : public Moo::DrawContext::UserDrawItem
{
public:
	ParticleSystemDrawItem() :
		renderer_(),
		worldTransform_( Matrix::identity )
	{
	}

	void set( ParticleSystemRenderer* renderer,
		const Matrix& worldTransform, 
        Particles::iterator beg, 
        Particles::iterator end );
	void draw();

private:
	ParticleSystemRenderer          * renderer_;
	Matrix	                        worldTransform_;
	Particles::iterator             beg_;
	Particles::iterator             end_;
};

BW_END_NAMESPACE

#endif // PARTICLE_SYSTEM_DRAW_ITEM_HPP
