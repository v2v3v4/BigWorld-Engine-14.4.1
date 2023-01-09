#ifndef PARTICLE_SYSTEM_RENDERER_HPP
#define PARTICLE_SYSTEM_RENDERER_HPP

// Standard MF Library Headers.
#include "moo/moo_math.hpp"
#include "moo/device_callback.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "moo/material.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/effect_material.hpp"
#include "particle/particle.hpp"
#include "particle/particles.hpp"

#include "moo/vertex_formats.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;

namespace Moo
{
	class DrawContext;
}
/**
 *	This is the base class for displaying a particle system on screen.
 */
class ParticleSystemRenderer : public ReferenceCount
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	ParticleSystemRenderer();
	virtual ~ParticleSystemRenderer();
	//@}


	///	@name Renderer Interface Methods.
	//@{
	virtual void update( Particles::iterator beg,
		Particles::iterator end, float dTime )	{}
	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb ) = 0;

	//Callback fn for ParticleSystemDrawItem sorted draw
	virtual void realDraw( const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end ) = 0;

	bool viewDependent() const;
	void viewDependent( bool flag );

	bool local() const;
	void local( bool flag );

	virtual bool isMeshStyle() const	{ return false; }
	virtual ParticlesPtr createParticleContainer() = 0;

    virtual bool knowParticleRadius() const { return false; }
    virtual float particleRadius() const { return 0.0f; }

	virtual size_t sizeInBytes() const = 0;
	//@}	

	void load( DataSectionPtr pSect );
	void save( DataSectionPtr pSect ) const;
	virtual ParticleSystemRenderer * clone() const = 0;

	virtual const BW::string & nameID() const = 0;

	static ParticleSystemRenderer * createRendererOfType(
		const BW::StringRef & type, DataSectionPtr ds = NULL );	

	// Return the resources required for a ParticleSystemRenderer
	static void prerequisitesOfType( DataSectionPtr pSection, BW::set<BW::string>& output );
	static void prerequisites( DataSectionPtr pSection, BW::set<BW::string>& output )	{};

protected:
	virtual void loadInternal( DataSectionPtr pSect ) = 0;
	virtual void saveInternal( DataSectionPtr pSect ) const = 0;
	void clone( ParticleSystemRenderer * dest ) const;

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	virtual void setupRenderState() const;

private:
	bool viewDependent_;
	bool local_;
};


typedef SmartPointer<ParticleSystemRenderer> ParticleSystemRendererPtr;


/**
 *	Helper struct used to load material in the background.
 */
template <typename RenderT>
struct BGUpdateData
{
	BGUpdateData(RenderT * spr) :
		spr_(spr),
		texture_(NULL)
	{}

	static void loadTexture(void * data)
	{
		BGUpdateData * updateData = static_cast<BGUpdateData *>(data);
		updateData->texture_ = Moo::TextureManager::instance()->get(
			updateData->spr_->textureName(), true, true, true, "texture/particle");
	}

	static void updateMaterial(void * data)
	{
		BGUpdateData * updateData = static_cast<BGUpdateData *>(data);
		updateData->spr_->updateMaterial(updateData->texture_);
		bw_safe_delete(updateData);
	}

	RenderT *                           spr_;
	Moo::BaseTexturePtr                 texture_;
	// std::auto_ptr<class BackgroundTask> task_;
};


class PyParticleSystemRenderer;
typedef SmartPointer<PyParticleSystemRenderer> PyParticleSystemRendererPtr;


/*~ class Pixie.PyParticleSystemRenderer
 *	ParticleSystemRenderer is an abstract base class for PyParticleSystem
 *	renderers.
 */
class PyParticleSystemRenderer : public PyObjectPlus
{
	Py_Header( PyParticleSystemRenderer, PyObjectPlus )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyParticleSystemRenderer( ParticleSystemRendererPtr pRenderer, PyTypeObject *pType = &s_type_ );
	virtual ~PyParticleSystemRenderer()	{};
	//@}

	ParticleSystemRendererPtr pRenderer()	{ return pRenderer_; }

	bool viewDependent() const	{ return pRenderer_->viewDependent(); }
	void viewDependent( bool flag )	{ pRenderer_->viewDependent(flag); }

	bool local() const			{ return pRenderer_->local(); }
	void local( bool flag )		{ pRenderer_->local(flag); }

	///	@name Python Interface to the ParticleSystemRenderer.
	//@{
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, viewDependent, viewDependent )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, local, local )
	//@}	

	static PyParticleSystemRendererPtr createPyRenderer(ParticleSystemRendererPtr pRenderer);

private:
	ParticleSystemRendererPtr	pRenderer_;
};


PY_SCRIPT_CONVERTERS_DECLARE( PyParticleSystemRenderer )


#ifdef CODE_INLINE
#include "particle_system_renderer.ipp"
#endif

BW_END_NAMESPACE

#endif // PARTICLE_SYSTEM_RENDERER_HPP
