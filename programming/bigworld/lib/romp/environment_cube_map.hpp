#ifndef ENVIRONMENT_CUBE_MAP_HPP
#define ENVIRONMENT_CUBE_MAP_HPP

#include "moo/cube_render_target.hpp"
#include "moo/effect_constant_value.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}
/**
 *	This class exposes the CubeMap render of the environment
 */
class EnvironmentCubeMap
{
public:
	EnvironmentCubeMap( uint16 textureSize = 0, uint8 numFacesPerFrame = 0 );
	~EnvironmentCubeMap();

	void update( float dTime,
		bool defaultNumFaces = true, uint8 numberOfFaces = 1,
		uint32 drawSelection = 0x80000000  );

	void numberOfFacesPerFrame( uint8 numberOfFaces );

	Moo::CubeRenderTargetPtr cubeRenderTarget() { return pCubeRenderTarget_; }
private:
	Moo::EffectConstantValuePtr pCubeRenderTargetEffectConstant_;
	Moo::CubeRenderTargetPtr pCubeRenderTarget_;

	uint8 environmentCubeMapFace_;
	uint8 numberOfFacesPerFrame_;
	Moo::DrawContext*	cubeMapDrawContext_;
};

BW_END_NAMESPACE

#endif // ENVIRONMENT_CUBE_MAP_HPP
