#ifndef TRAIL_PARTICLE_RENDERER_HPP
#define TRAIL_PARTICLE_RENDERER_HPP

#include "particle_system_renderer.hpp"
#include "particle_system_draw_item.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TrailParticleRenderer.
// -----------------------------------------------------------------------------
/**
 * TODO: to be documented.
 */
class TrailParticleRenderer : public ParticleSystemRenderer
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	TrailParticleRenderer();
	~TrailParticleRenderer();
	//@}

	/// @name Trail particle renderer methods.
	//@{
	void textureName( const BW::StringRef & v );
	const BW::string & textureName() const { return textureName_; }
	//@}

	///	@name Accessors for effects.
	//@{
	float width() const { return width_; }
	void width(float value) { width_ = value; }

	int steps() const { return steps_; }
	void steps( int value );

	int skip() const { return skip_; }
	void skip( int value )	{ skip_ = value; }

	bool useFog() const { return useFog_; }
	void useFog( bool b){ useFog_ = b; material_.fogged(b); }
	//@}	


	///	@name Renderer Overrides.
	//@{
	void update( Particles::iterator beg,
		Particles::iterator end, float dTime );
	virtual void draw( Moo::DrawContext& drawContext,
		const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb );
	virtual void realDraw( const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end );
	static void prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output );
	virtual TrailParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;
	virtual ParticlesPtr createParticleContainer()
	{
		particles_ = new TrailParticles;
		this->steps( steps_ );
		return particles_;
	}
	virtual size_t sizeInBytes() const;

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
	int					steps_;

	class TrailParticles : public FixedIndexParticles
	{
	public:
		TrailParticles();
		virtual ~TrailParticles();

		Particle* addParticle( const Particle &particle, bool isMesh );
		void reserve( size_t amount );

		Vector3 getPosition( uint index, uint age ) const;
		void copyPositions();
		void historySteps( uint32 steps );
		void resetHistory( const Particle &particle );

		uint steps_;
		uint historyIndex_;
		BW::vector<Vector3> * positionHistories_;
	};

	TrailParticles*		particles_;
	int					skip_;		//for each cache::copy, skip n updates
	int					frame_;
};


typedef SmartPointer<TrailParticleRenderer> TrailParticleRendererPtr;


/*~ class Pixie.PyTrailParticleRenderer
 *
 *	TrailParticleRenderer is a ParticleSystemRenderer that lofts out a trail
 *	based on the cached history of particles.  The trail is a textured 3D
 *	line that looks good from most angles, except when going straight into/out
 *	of the screen, in which case you will see a thin cross.
 *
 *	Because the history of a particles position is cached, the trail of each
 *	particle may bend ( as opposed to the BlurParticleRenderer, which will
 *	always have straight-line trails )
 *
 *	The TrailParticleRenderer should be used judiciously, because of the memory
 *	overhead of the particle history cache.  However, it can create great effects
 *	using just a small amount of particles.
 *
 *	This renderer will use an extra n * s memory, where n is the capacity of the
 *	particle system, and s is the number of steps in each trail.
 *
 *	A new PyTrailParticleRenderer is created using Pixie.TrailParticleRenderer
 *	function.
 */
class PyTrailParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PyTrailParticleRenderer, PyParticleSystemRenderer )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyTrailParticleRenderer( TrailParticleRendererPtr pRenderer, PyTypeObject *pType = &s_type_ );	
	//@}

	///	@name Accessors for effects.
	//@{
	float width() { return pR_->width(); }
	void width(float value) { pR_->width(value); }

	int steps() { return pR_->steps(); }
	void steps( int value )	{ pR_->steps(value); }

	const BW::string& textureName() const	{ return pR_->textureName(); }
	void textureName( const BW::string& v )	{ pR_->textureName(v); }

	int skip() { return pR_->skip(); }
	void skip( int value )	{ pR_->skip(value); }

	bool useFog() const { return pR_->useFog(); }
	void useFog( bool b ){ pR_->useFog(b); }
	//@}

	///	@name Python Interface to the TrailParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, width, width )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, steps, steps )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, skip, skip )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, useFog, useFog )
	//@}
private:
	TrailParticleRendererPtr pR_;
};

BW_END_NAMESPACE

#endif // TRAIL_PARTICLE_RENDERER_HPP
