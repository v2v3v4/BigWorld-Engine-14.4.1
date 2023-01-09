#ifndef SPRITE_PARTICLE_RENDERER_HPP
#define SPRITE_PARTICLE_RENDERER_HPP

#include "particle_system_draw_item.hpp"
#include "particle_system_renderer.hpp"
#include "resmgr/auto_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class displays the particle system with each particle being a single
 *	spin-to-face polygon.
 */
class SpriteParticleRenderer : public ParticleSystemRenderer
{
public:
	/// An enumeration of the possible material effects.
	enum MaterialFX
	{
		FX_ADDITIVE,
		FX_ADDITIVE_ALPHA,
		FX_BLENDED,
		FX_BLENDED_COLOUR,
		FX_BLENDED_INVERSE_COLOUR,
		FX_SOLID,
		FX_SHIMMER,
        FX_SOURCE_ALPHA,
		FX_MAX	// keep this element at the end
	};

	/// @name Constructor(s) and Destructor.
	//@{
	SpriteParticleRenderer( const BW::StringRef & newTextureName );
	~SpriteParticleRenderer();
	//@}


	///	@name Accessors for Sprite Effects.
	//@{
	MaterialFX materialFX() const;
	void materialFX( MaterialFX newMaterialFX );

	bool isValid() const;

	const BW::string & effectName() const;
	void effectName( const BW::string& value );

	const BW::string & textureName() const;
	void textureName( const BW::string &newString );

	int frameCount() const { return frameCount_; }
	void frameCount( int c ) { frameCount_ = c; }

	float frameRate() const { return frameRate_; }
	void frameRate( float r ) { frameRate_ = r; }

	static bool isSoftSupported();
	void resetSoft();	

	float softDepthRange() const;
	void softDepthRange( float value );

	float softFalloffPower() const;
	void softFalloffPower( float value );

	float softDepthOffset() const;
	void softDepthOffset( float value );

	void resetNearFade();

	float nearFadeCutoff() const;
	void nearFadeCutoff( float value );

	float nearFadeStart() const;
	void nearFadeStart( float value );

	float nearFadeFalloffPower() const;
	void nearFadeFalloffPower( float value );

	//@}


	///	@name Renderer Overrides.
	//@{
	virtual void draw( Moo::DrawContext& drawContext,
		const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb );

	virtual void realDraw( const Matrix& worldTransform,
		Particles::iterator beg,
		Particles::iterator end );

	static void prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output );

	virtual size_t sizeInBytes() const { return sizeof(SpriteParticleRenderer); }

	virtual SpriteParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;

	virtual const Vector3 & explicitOrientation() const { return explicitOrientation_; }
	virtual void explicitOrientation( const Vector3 & newVal ) { explicitOrientation_ = newVal; }

	virtual ParticlesPtr createParticleContainer()
	{
		return new ContiguousParticles;
	}

	void rotated( bool option ) { rotated_ = option; }
    bool rotated() const { return rotated_; }

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Render Methods.
	//@{
	virtual void updateTexture( bool force = false );
	virtual void updateMaterial();
	virtual void setMaterial(
		Moo::EffectMaterialPtr& material );
	void updateMaterial( const Moo::BaseTexturePtr & texture );
	friend struct BGUpdateData<SpriteParticleRenderer>;
	//@}

	virtual void setMaterialProperties( bool force = false );
	virtual void setMaterialFX();

	///	@name Sprite Material/Texture Properties.
	//@{
	MaterialFX materialFX_;			///< Stored material FX setting.
	BW::string textureName_;		///< The name of the texture.

	Moo::Material material_;		///< The Material.
	bool materialSettingsChanged_;	///< Dirty flag for material settings.

	int		frameCount_;			///< Frames of animation in texture.
	float	frameRate_;				///< Frames per second of particle age.

	Vector3 explicitOrientation_;	///< Explicit orientation of the sprite particles.

	bool	rotated_;
	//@}


	///	@name Mesh Properties of the Sprite
	//@{
	Moo::VertexTDSUV2 sprite_[4];	///< The sprite plane.
	//@}

	// -- Particle properties
	// -- Particle material properties
	BW::string effectName_;

	// -- Material effect properties
	Moo::BaseTexturePtr diffuseMap_;

	// -- Particle
	ParticleSystemDrawItem sortedDrawItem_;
	Moo::EffectMaterialPtr effectMaterial_;



	// -----------------------------------------------------------
	// Soft Particle Params 

	// Modifies range of depth difference to which softness is applied.
	float softDepthRange_;

	// Falloff curve power. 1 is linear. Higher values increase fade rate.
	float softFalloffPower_;

	// Shifts the softening by relative depth difference, relative to extent.
	float softDepthOffset_;

	// -----------------------------------------------------------
	// Near Fade Params 

	// Distance at which particles become completely invisible.
	float nearFadeCutoff_;

	// Distance at which fading is full opacity and starts fading towards near cutoff
	float nearFadeStart_;

	// Falloff curve power, 1 is linear. Higher values increase fade rate.
	float nearFadeFalloffPower_;

};


typedef SmartPointer<SpriteParticleRenderer> SpriteParticleRendererPtr;


/*~ class Pixie.PySpriteParticleRenderer
 *
 *	SpriteParticleRenderer is a ParticleSystemRenderer which renders each
 *	particle as a single spin to face polygon.
 *
 *	A new PySpriteParticleRenderer is created using Pixie.SpriteRenderer
 *	function.
 */
class PySpriteParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PySpriteParticleRenderer, PyParticleSystemRenderer )

public:

	/// An enumeration of the possible material effects.
	enum MaterialFX
	{
		FX_ADDITIVE,
		FX_ADDITIVE_ALPHA,
		FX_BLENDED,
		FX_BLENDED_COLOUR,
		FX_BLENDED_INVERSE_COLOUR,
		FX_SOLID,
		FX_SHIMMER,
        FX_SOURCE_ALPHA,
		FX_MAX	// keep this element at the end
	};

	/// @name Constructor(s) and Destructor.
	//@{
	PySpriteParticleRenderer(
		SpriteParticleRendererPtr pR,  PyTypeObject *pType = &s_type_ );
	//@}


	///	@name Accessors for Sprite Effects.
	//@{
	MaterialFX materialFX() const				{ return (MaterialFX)pR_->materialFX(); }
	void materialFX( MaterialFX newMaterialFX )	{ pR_->materialFX((SpriteParticleRenderer::MaterialFX)newMaterialFX); }

	const BW::string& effectName() const { return pR_->effectName(); }
	void effectName( const BW::string& v ) { pR_->effectName(v); }

	const BW::string &textureName() const		{ return pR_->textureName(); }
	void textureName( const BW::string &newString )	{ pR_->textureName(newString); }

	int frameCount() const { return pR_->frameCount(); }
	void frameCount( int c ) { pR_->frameCount(c); }

	float frameRate() const { return pR_->frameRate(); }
	void frameRate( float r ) { pR_->frameRate(r); }

	void resetSoft() { pR_->resetSoft(); }

	float softDepthRange() const { return pR_->softDepthRange(); }
	void softDepthRange( float v ) { pR_->softDepthRange(v); }

	float softFalloffPower() const { return pR_->softFalloffPower(); }
	void softFalloffPower( float v ) { pR_->softFalloffPower(v); }

	float softDepthOffset() const { return pR_->softDepthOffset(); }
	void softDepthOffset( float v ) { pR_->softDepthOffset(v); }

	void resetNearFade() { pR_->resetNearFade(); }

	float nearFadeCutoff() const { return pR_->nearFadeCutoff(); }
	void nearFadeCutoff( float v ) { pR_->nearFadeCutoff(v); }

	float nearFadeStart() const { return pR_->nearFadeStart(); }
	void nearFadeStart( float v ) { pR_->nearFadeStart(v); }

	float nearFadeFalloffPower() const { return pR_->nearFadeFalloffPower(); }
	void nearFadeFalloffPower( float v ) { pR_->nearFadeFalloffPower(v); }

	//@}


	/// @name The Python Interface to SpriteParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()

	PY_DEFERRED_ATTRIBUTE_DECLARE( materialFX )

	PY_AUTO_METHOD_DECLARE( RETVOID, resetSoft, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, resetNearFade, END )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, effectName, effectName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )	
	
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, softDepthRange, softDepthRange )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, softFalloffPower, softFalloffPower )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, softDepthOffset, softDepthOffset )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, nearFadeCutoff, nearFadeCutoff )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, nearFadeStart, nearFadeStart )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, nearFadeFalloffPower, nearFadeFalloffPower )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, frameCount, frameCount )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frameRate, frameRate )

	// Python Wrappers for handling Enumerations in Python.
	PY_BEGIN_ENUM_MAP( MaterialFX, FX_ )
		PY_ENUM_VALUE( FX_ADDITIVE )
		PY_ENUM_VALUE( FX_ADDITIVE_ALPHA )
		PY_ENUM_VALUE( FX_BLENDED )
		PY_ENUM_VALUE( FX_BLENDED_COLOUR )
		PY_ENUM_VALUE( FX_BLENDED_INVERSE_COLOUR )
		PY_ENUM_VALUE( FX_SOLID )
		PY_ENUM_VALUE( FX_SHIMMER )
        PY_ENUM_VALUE( FX_SOURCE_ALPHA )
	PY_END_ENUM_MAP()
	//@}

protected:
	SpriteParticleRendererPtr	pR_;
};

PY_ENUM_CONVERTERS_DECLARE( PySpriteParticleRenderer::MaterialFX )


#ifdef CODE_INLINE
#include "sprite_particle_renderer.ipp"
#endif

BW_END_NAMESPACE

#endif // SPRITE_PARTICLE_RENDERER_HPP
