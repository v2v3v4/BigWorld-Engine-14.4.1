#ifndef BLUR_PARTICLE_RENDERER_HPP
#define BLUR_PARTICLE_RENDERER_HPP

#include "particle_system_renderer.hpp"
#include "particle_system_draw_item.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BlurParticleRenderer.
// -----------------------------------------------------------------------------
/**
 * TODO: to be documented.
 */
class BlurParticleRenderer : public ParticleSystemRenderer
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	BlurParticleRenderer();
	~BlurParticleRenderer();
	//@}

	/// @name Blur particle renderer methods.
	//@{
	void textureName( const BW::StringRef & v );
	const BW::string& textureName() const { return textureName_; }
	//@}

	///	@name Accessors for effects.
	//@{
	float width() const { return width_; }
	void width(float value) { width_ = value; }

	float time() const { return time_; }
	void time( float value ) { time_ = value; }

	bool useFog() const { return useFog_; }
	void useFog( bool b){ useFog_ = b; material_.fogged(b); }
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
		Particles::iterator end );
	virtual BlurParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;
	virtual ParticlesPtr createParticleContainer()
	{
		return new ContiguousParticles;
	}
	virtual size_t sizeInBytes() const { return sizeof(BlurParticleRenderer); }

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	ParticleSystemDrawItem sortedDrawItem_;
	Moo::Material		material_;
	BW::string			textureName_;
	bool				useFog_;	///< Use scene fogging or not.
	float				width_;
	float				time_;
};


typedef SmartPointer<BlurParticleRenderer> BlurParticleRendererPtr;


/*~ class Pixie.PyBlurParticleRenderer
 *
 *	BlurParticleRenderer is a ParticleSystemRenderer that draws a trail
 *	for each particle, but does so in a more efficient manner than the
 *	TrailParticleRenderer.
 *
 *	The BlurParticleRenderer draws a single straight trail for each
 *	particle, by drawing a textured line using the particle's current
 *	velocity negated.
 *
 *	Due to this, the trails drawn by the BlurParticleRenderer will not
 *	exhibit bends - however, there is no memory overhead for using this
 *	renderer.
 *
 *	A new PyBlurParticleRenderer is created using Pixie.BlurParticleRenderer
 *	function.
 */
class PyBlurParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PyBlurParticleRenderer, PyParticleSystemRenderer )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyBlurParticleRenderer( BlurParticleRendererPtr pR, PyTypeObject *pType = &s_type_ );	
	//@}

	/// @name Blur particle renderer methods.
	//@{
	void textureName( const BW::string& v )	{ pR_->textureName(v); }
	const BW::string& textureName() const	{ return pR_->textureName(); }
	//@}

	///	@name Accessors for effects.
	//@{
	float width() { return pR_->width(); }
	void width(float value) { pR_->width(value); }

	float time() { return pR_->time(); }
	void time( float value ) { pR_->time(value); }

	bool useFog() const { return pR_->useFog(); }
	void useFog( bool b ){ pR_->useFog(b); }
	//@}

	///	@name Python Interface to the BlurParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, width, width )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, time, time )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, useFog, useFog )
	//@}
private:
	BlurParticleRendererPtr pR_;
};

BW_END_NAMESPACE

#endif // BLUR_PARTICLE_RENDERER_HPP
