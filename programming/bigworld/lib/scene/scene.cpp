#include "pch.hpp"
#include "scene.hpp"

#include "scene_provider.hpp"
#include "scene_object.hpp"

#include <functional>

namespace BW
{

Scene::Scene()
{
	// Construct maps with smart pointers here.
	// (saves classes that construct this class from 
	// including all our dependencies)
}


Scene::~Scene()
{
	// Destruct smart pointers in maps here.
	// (saves classes that construct this class from 
	// including all our dependencies)
}


void Scene::addProvider( SceneProvider * pProvider )
{
	// Make sure that we don't add providers more than once
	MF_ASSERT( std::find( sceneProviders_.begin(), sceneProviders_.end(), 
		pProvider ) == sceneProviders_.end() );

	MF_ASSERT( pProvider->scene() == NULL );
	pProvider->scene( this );

	sceneProviders_.push_back( pProvider );

	// Now iterate over all scene interfaces, and add this provider
	for (ViewMap::iterator iter = views_.begin();
		iter != views_.end(); ++iter)
	{
		ISceneViewPtr pInterface = iter->second;

		pInterface->addProvider( pProvider );
	}
}


void Scene::removeProvider( SceneProvider * pProvider )
{
	// Iterate over all scene interfaces, and remove this provider
	for (ViewMap::iterator iter = views_.begin();
		iter != views_.end(); ++iter)
	{
		ISceneViewPtr pInterface = iter->second;

		pInterface->removeProvider( pProvider );
	}

	// Remove from our list of scene providers
	sceneProviders_.erase( 
		std::remove(
			sceneProviders_.begin(), 
			sceneProviders_.end(), 
			pProvider ), 
		sceneProviders_.end() );

	MF_ASSERT( pProvider->scene() == this || pProvider->scene() == NULL );
	pProvider->scene( NULL );
}


} // namespace BW
