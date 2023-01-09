#ifndef VISUAL_PARTICLE_RENDERER_HPP
#define VISUAL_PARTICLE_RENDERER_HPP

#include "base_mesh_particle_renderer.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/device_callback.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class displays the particle system with each particle drawing a visual.
 */
// -----------------------------------------------------------------------------
// Section: VisualParticleRenderer.
// -----------------------------------------------------------------------------
class VisualParticleRenderer : public BaseMeshParticleRenderer
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	VisualParticleRenderer();
	~VisualParticleRenderer();
	//@}

	/// @name visual particle renderer methods.
	//@{
	virtual void visual( const BW::StringRef & v );
	virtual const BW::string& visual() const	{ return visualName_; }
	//@}	


	///	@name Renderer Overrides.
	//@{
	virtual void draw( Moo::DrawContext& drawContext,
		const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb );

	virtual void realDraw( const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end )	{};

	static void prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output );

	virtual VisualParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;

	virtual bool isMeshStyle() const	{ return true; }
	virtual ParticlesPtr createParticleContainer()
	{
		// TODO: Improve performance with Contiguous but need to 
		// still be able to calculate particle index (Flex. Part. Formats)
		return new FixedIndexParticles;
	}

	virtual size_t sizeInBytes() const { return sizeof(VisualParticleRenderer); }

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	BW::string			visualName_;   
};


typedef SmartPointer<VisualParticleRenderer> VisualParticleRendererPtr;


/*~ class Pixie.PyVisualParticleRenderer
 *	VisualParticleRenderer is a ParticleSystemRenderer which renders each particle
 *	as a visual object.  Any visual can be used, although the TintShader will only
 *	work on visuals that use effect files that use MeshParticleTint.
 */
class PyVisualParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PyVisualParticleRenderer, PyParticleSystemRenderer )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyVisualParticleRenderer( VisualParticleRendererPtr pR, PyTypeObject *pType = &s_type_ );
	//@}

	/// @name visual particle renderer methods.
	//@{
	virtual void visual( const BW::string& v )	{ pR_->visual(v); }
	virtual const BW::string& visual() const	{ return pR_->visual(); }
	//@}

	///	@name Python Interface to the VisualParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, visual, visual )
	//@}

private:
	VisualParticleRendererPtr	pR_;
};

BW_END_NAMESPACE

#endif // VISUAL_PARTICLE_RENDERER_HPP
