#include "pch.hpp"
#include "particle_system_draw_item.hpp"
#include "sprite_particle_renderer.hpp"


BW_BEGIN_NAMESPACE

void ParticleSystemDrawItem::set(  
	ParticleSystemRenderer  *renderer,
    const Matrix&           worldTransform, 
    Particles::iterator     beg, 
    Particles::iterator     end )
{
	renderer_ = renderer;
	worldTransform_ = worldTransform;
	beg_ = beg;
	end_ = end;
}


void ParticleSystemDrawItem::draw()
{
	BW_GUARD;
	renderer_->realDraw( worldTransform_, beg_, end_ );
}

BW_END_NAMESPACE

// particle_system_draw_item.cpp
