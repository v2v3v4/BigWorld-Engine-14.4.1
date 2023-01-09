#ifndef AMP_PARTICLE_RENDERER_HPP
#define AMP_PARTICLE_RENDERER_HPP

// -----------------------------------------------------------------------------
// Section: AmpParticleRenderer.
// -----------------------------------------------------------------------------
#include "particle_system_renderer.hpp"
#include "particle_system_draw_item.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class AmpParticleRenderer : public ParticleSystemRenderer
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	AmpParticleRenderer();
	~AmpParticleRenderer();
	//@}

	static void prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output );

	/// @name Amp particle renderer methods.
	//@{
	void textureName( const BW::StringRef & v );
	const BW::string& textureName() const	{ return textureName_; }
	//@}

	///	@name Accessors for effects.
	//@{
	float width() const { return width_; }
	void width(float value) { width_ = value; }

	float height() const { return height_; }
	void height(float value) { height_ = value; }

	int steps() const { return steps_; }
	void steps(int value) { steps_ = value; }

	float variation() const { return variation_; }
	void variation(float value) { variation_ = value; }

	bool circular() const { return circular_; }
	void circular(bool state) { circular_ = state; }

	bool useFog() const { return useFog_; }
	void useFog( bool b) { useFog_ = b; material_.fogged(b); }
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
	virtual AmpParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;
	virtual ParticlesPtr createParticleContainer()
	{
		return new ContiguousParticles;
	}
	virtual size_t sizeInBytes() const { return sizeof(AmpParticleRenderer); }

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
	float				height_;
	int					steps_;
	float				variation_;
	bool				circular_;
};


typedef SmartPointer<AmpParticleRenderer>	AmpParticleRendererPtr;


/*~ class Pixie.PyAmpParticleRenderer
 *	AmpPartileRenderer is a ParticleRenderer that renders segmented lines
 *	from the particle position back to the origin of the particle system. It
 *	can be used for effects such as electricty.
 *
 *	Eg. (bad ascii art warning) 3 line segments from start point (s) to end
 *	point (e)
 *	@{
 *    /\
 *	 /  \
 *	s    \  e
 *        \/
 *	@}
 *	
 *	The origin referred to below is either the source position of the
 *	particle system (in the case of a non-circular AmpParticleRenderer)
 *	or the position of the previous particle (ordered by the age of the
 *	particle - in the case of a circular AmpParticleRenderer).
 *
 *	A new PyAmpParticleRenderer is created using Pixie.AmpParticleRenderer
 *	function.
 */
class PyAmpParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PyAmpParticleRenderer, PyParticleSystemRenderer )
public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyAmpParticleRenderer( AmpParticleRendererPtr pR,  PyTypeObject *pType = &s_type_ );
	//@}

	///	@name Accessors for effects.
	//@{
	void textureName( const BW::string& v )	{ pR_->textureName(v); }
	const BW::string& textureName() const		{ return pR_->textureName(); }

	float width() { return pR_->width(); }
	void width(float value) { pR_->width(value); }

	float height() { return pR_->height(); }
	void height(float value) { pR_->height(value); }

	int steps() { return pR_->steps(); }
	void steps(int value) { pR_->steps(value); }

	float variation() { return pR_->variation(); }
	void variation(float value) { pR_->variation(value); }

	bool circular() { return pR_->circular(); }
	void circular(bool state) { pR_->circular(state); }

	bool useFog() const { return pR_->useFog(); }
	void useFog( bool b ){ pR_->useFog(b); }
	//@}

	///	@name Python Interface to the AmpParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, width, width )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, height, height )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, steps, steps )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, variation, variation )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, circular, circular )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, useFog, useFog )
	//@}
private:
	AmpParticleRendererPtr pR_;
};

BW_END_NAMESPACE

#endif // AMP_PARTICLE_RENDERER_HPP
