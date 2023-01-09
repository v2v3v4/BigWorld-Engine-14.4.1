#ifndef _ENVIRO_MINDER_SCENE_PROVIDER_HPP
#define _ENVIRO_MINDER_SCENE_PROVIDER_HPP

#include <scene/scene_provider.hpp>
#include <scene/tick_scene_view.hpp>
#include <scene/light_scene_view.hpp>

#include <moo/directional_light.hpp>

#include <romp/enviro_minder.hpp>

#include <space/forward_declarations.hpp>

BW_BEGIN_NAMESPACE
namespace Moo {
	class LightContainer;
	typedef SmartPointer< LightContainer > LightContainerPtr;
}

class DataSection;
typedef SmartPointer< DataSection > DataSectionPtr;

class OutsideLighting;

BW_END_NAMESPACE

namespace BW {

class EnviroMinderSceneProvider :
	public SceneProvider,
	public ITickSceneProvider,
	public ILightSceneViewProvider
{
public:
	EnviroMinderSceneProvider( SpaceID id );
	virtual ~EnviroMinderSceneProvider();

	// ITickSceneProvider interfaces ...
	void tick( float dTime );
	void updateAnimations( float dTime );

	// ILightSceneViewProvider interfaces ...
	size_t intersect( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;

	size_t intersect( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void debugDrawLights() const { }

	const Moo::DirectionalLightPtr & sunLight() const;
	const Moo::Colour & ambientLight() const;
	const Moo::LightContainerPtr & lights() const;

	EnviroMinder & enviroMinder();

	void loadEnvironment( const DataSectionPtr& pSpaceSettings );
private:
	// Scene view implementations
	virtual void * getView(
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

	void updateHeavenlyLighting();

	size_t addEnviroLights( Moo::LightContainer & lightContainer ) const;

	EnviroMinder environment_;

	const OutsideLighting * pOutLight_;
	Moo::DirectionalLightPtr sunLight_;
	Moo::Colour	ambientLight_;
	Moo::LightContainerPtr lights_;
};

} // namespace BW

#endif // _ENVIRO_MINDER_SCENE_PROVIDER_HPP
