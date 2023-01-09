#ifndef SKYGRADIENTDOME2_HPP
#define SKYGRADIENTDOME2_HPP

#include <iostream>
#include "resmgr/datasection.hpp"
#include "moo/effect_material.hpp"
#include "moo/device_callback.hpp"
#include "moo/visual.hpp"
#include "math/linear_animation.hpp"
#include "fog_controller.hpp"
#include "effect_parameter_cache.hpp"


BW_BEGIN_NAMESPACE

class TimeOfDay;


/**
 * TODO: to be documented.
 */
class SkyGradientDome : public Moo::DeviceCallback
{
public:
	SkyGradientDome();
	~SkyGradientDome();

	void activate( const class EnviroMinder&, DataSectionPtr pSpaceSettings );
	void deactivate( const class EnviroMinder& );

	void		load( DataSectionPtr root );
    bool        loadTexture( const BW::string & textureName );
#ifdef EDITOR_ENABLED
	void		save( DataSectionPtr root );

    float       mieEffect         () const { return mieEffect_         ; }
    float		turbidityOffset   () const { return turbidityOffset_   ; }
	float		turbidityFactor   () const { return turbidityFactor_   ; }
	float		vertexHeightEffect() const { return vertexHeightEffect_; }
	float		sunHeightEffect   () const { return sunHeightEffect_   ; }
	float		power             () const { return power_             ; }
	float		effectiveTurbidity() const { return effectiveTurbidity_; }

    void        mieEffect         (float value) { mieEffect_          = value; }
    void        turbidityOffset   (float value) { turbidityOffset_    = value; }
    void        turbidityFactor   (float value) { turbidityFactor_    = value; }
    void        vertexHeightEffect(float value) { vertexHeightEffect_ = value; }
    void        sunHeightEffect   (float value) { sunHeightEffect_    = value; }
    void        power             (float value) { power_              = value; }
    void        effectiveTurbidity(float value) { effectiveTurbidity_ = value; } 

    const BW::string & textureName() const { return texName_; }

#endif
	bool		fullOpacity() const;

	void		fogModulation( const Vector3 & modulateColour, float fogMultiplier );

	void		draw( TimeOfDay* timeOfDay );
	void		update( float time );

	void		nearMultiplier( float n );
	float		nearMultiplier() const;

	void		farMultiplier( float f );
	float		farMultiplier() const;

	void		createUnmanagedObjects();
	void		deleteUnmanagedObjects();

	//called by enviro minder
	void		addFogEmitter();
	void		remFogEmitter();

private:
	bool		createAnimationsFromTexture( Moo::BaseTexturePtr pTex );

	SkyGradientDome(const SkyGradientDome&);
	SkyGradientDome& operator=(const SkyGradientDome&);

	//Common variables
	FogController::Emitter	    fogEmitter_;
	int				            emitterID_;	
	DWORD			            col_;
	bool			            fullOpacity_;
	Vector3			            modulateColour_;
	float			            fogMultiplier_;
	Moo::EffectMaterialPtr		material_;
	Moo::VisualPtr	            skyDome_;	

	//These variables define the mie scattering.
	float			            mieEffect_;
	float			            turbidityOffset_;
	float			            turbidityFactor_;
	float			            vertexHeightEffect_;
	float			            sunHeightEffect_;
	float			            power_;	
	float			            effectiveTurbidity_;

#ifdef EDITOR_ENABLED
    BW::string                 texName_;
#endif

	LinearAnimation< float >	textureAlphas_;
	LinearAnimation< Vector3 >	fogAnimation_;
	Vector4						fogFactors_;
	bool			            inited_;

	Moo::BaseTexturePtr	        pRayleighMap_;	
	EffectParameterCache        parameters_;	

	friend std::ostream& operator<<(std::ostream&, const SkyGradientDome&);
};

#ifdef CODE_INLINE
#include "sky_gradient_dome.ipp"
#endif

BW_END_NAMESPACE


#endif
/*skygradientdome2.hpp*/
