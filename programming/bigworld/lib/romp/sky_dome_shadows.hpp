/**
 *	This class draws the 'cloud shadows' for sky boxes.
 */

#ifndef SKY_DOME_SHADOWS_HPP
#define SKY_DOME_SHADOWS_HPP

#include "sky_light_map.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class implements the SkyLightMap::IContributor interface.
 *
 * It works by setting the "EnvironmentShadowDraw" flag as an effect
 * constant so that the existing sky domes, whatever they may be, can
 * be drawn again using special shadow generation shaders.
 *
 * In order to capture the shadow information, the sky boxes are
 * transformed using an orthogonal projection into the sky light map. 
 */
class SkyDomeShadows : public SkyLightMap::IContributor
{
public:
	SkyDomeShadows( class EnviroMinder& enviroMinder );
	~SkyDomeShadows();

	static bool isAvailable();

	bool needsUpdate()	{ return true; }
	void render( SkyLightMap* lightMap,
				Moo::EffectMaterialPtr material, float sunAngle );	
private:	
	class EnviroMinder& enviroMinder_;
};

BW_END_NAMESPACE

#endif
