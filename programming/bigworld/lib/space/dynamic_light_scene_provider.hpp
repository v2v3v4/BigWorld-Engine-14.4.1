#ifndef _DYNAMIC_LIGHT_SCENE_PROVIDER_HPP
#define _DYNAMIC_LIGHT_SCENE_PROVIDER_HPP

#include "forward_declarations.hpp"
#include "light_embodiment.hpp"

#include <scene/scene_provider.hpp>
#include <scene/tick_scene_view.hpp>
#include <scene/light_scene_view.hpp>

namespace BW
{

class DynamicLightSceneProvider : 
	public SceneProvider,
	public ITickSceneProvider,
	public ILightSceneViewProvider
{
public:
	
	void addLightEmbodiment( const ILightEmbodimentPtr & pLightEmb );
	void removeLightEmbodiment( const ILightEmbodimentPtr & pLightEmb );
	void clear();

private:
	// Scene view interfaces
	void * getView( 
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID );

	// ITickSceneProvider interfaces...
	void tick( float dTime );
	void updateAnimations( float dTime );
	
	// ILightSceneViewProvider interfaces...
	size_t intersect( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;
	size_t intersect( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;
	void debugDrawLights() const;

private:
	typedef BW::vector<ILightEmbodimentPtr> LightEmbodiments;

	LightEmbodiments lights_;
};

} // namespace BW

#endif // _DYNAMIC_LIGHT_SCENE_PROVIDER_HPP
