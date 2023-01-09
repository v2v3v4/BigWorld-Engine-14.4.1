#ifndef BASE_MESH_PARTICLE_RENDERER_HPP
#define BASE_MESH_PARTICLE_RENDERER_HPP


#include "particle_system_renderer.hpp"
#include "moo/visual.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This is the base class for mesh particle renderers.
 */
class BaseMeshParticleRenderer : public ParticleSystemRenderer
{
public:
	/// @name Constructor.
	//@{
    BaseMeshParticleRenderer();    
	//@}

	///	@name Renderer Interface Methods.
	//@{
    virtual bool knowParticleRadius() const;
    virtual float particleRadius() const;
    //@}

	/// @name visual particle renderer methods.
	//@{
	virtual void visual( const BW::StringRef & v ) = 0;
	virtual const BW::string & visual() const = 0;
    //@/

protected:
    Moo::VisualPtr		pVisual_;
};

BW_END_NAMESPACE

#endif // BASE_MESH_PARTICLE_RENDERER_HPP
