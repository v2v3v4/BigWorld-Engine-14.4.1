#ifndef POINT_SPRITE_PARTICLE_RENDERER_HPP
#define POINT_SPRITE_PARTICLE_RENDERER_HPP

#include "sprite_particle_renderer.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class PointSpriteParticleRenderer : public SpriteParticleRenderer
{
public:

	/// @name Constructor(s) and Destructor.
	//@{
	PointSpriteParticleRenderer( const BW::StringRef & newTextureName );
	~PointSpriteParticleRenderer();
	//@}

	///	@name Renderer Overrides.
	//@{
	virtual void draw( Moo::DrawContext& drawContext,
		const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb );

	void realDraw( const Matrix& worldTransform,
		Particles::iterator beg,
		Particles::iterator end );

	virtual size_t sizeInBytes() const { return sizeof(PointSpriteParticleRenderer); }

	virtual PointSpriteParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;
	virtual ParticlesPtr createParticleContainer()
	{
		return new ContiguousParticles;
	}

protected:
	///	@name Auxiliary Methods.
	//@{
	void updateMaterial();
	void updateMaterial( const Moo::BaseTexturePtr & texture );
	friend struct BGUpdateData<PointSpriteParticleRenderer>;
	//@}
};


typedef SmartPointer<PointSpriteParticleRenderer> PointSpriteParticleRendererPtr;


/*~ class Pixie.PyPointSpriteParticleRenderer
 *
 *	PointSpriteParticleRenderer is a ParticleSystemRenderer which renders each
 *	particle as a point sprite.
 *
 *	Texture animation (rolling uv) is not supported, nor are particle sizes
 *	on the screen > 64 pixels.
 *
 *	This particle renderer is the quickest to draw, and uses the least amount
 *	of graphics memory ( push buffer ).
 *
 *	A new PyPointSpriteParticleRenderer is created using Pixie.PointSpriteParticleRenderer
 *	function.
 */
class PyPointSpriteParticleRenderer : public PySpriteParticleRenderer
{
	Py_Header( PyPointSpriteParticleRenderer, PySpriteParticleRenderer )

public:

	/// @name Constructor(s) and Destructor.
	//@{
	PyPointSpriteParticleRenderer( PointSpriteParticleRendererPtr pR, PyTypeObject *pType = &s_type_ );
	//@}

	/// @name The Python Interface to PointSpriteParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()
	//@}
};

BW_END_NAMESPACE

#endif // POINT_SPRITE_PARTICLE_RENDERER_HPP
