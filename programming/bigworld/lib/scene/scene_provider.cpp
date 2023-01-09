#include "pch.hpp"
#include "scene_provider.hpp"
#include "scene_object.hpp"
#include "scene_listener.hpp"

#include <functional>


namespace BW
{

ISceneView::ISceneView()
	: pOwner_( NULL )
{

}

void ISceneView::scene( Scene * pScene ) 
{ 
	Scene* pOldScene = pOwner_;
	pOwner_ = pScene;

	onSetScene( pOldScene, pScene );
}


Scene* ISceneView::scene() const
{ 
	return pOwner_; 
}


void ISceneView::onSetScene( Scene * pOldScene, Scene * pNewScene )
{
	// Nothing to be done by default
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
SceneProvider::SceneProvider() : pScene_( NULL )
{

}


SceneProvider::~SceneProvider()
{

}


void SceneProvider::scene( Scene* pScene )
{
	Scene* pOldScene = pScene_;
	pScene_ = pScene;

	onSetScene( pOldScene, pScene );
}


void SceneProvider::onSetScene( Scene* pOldScene, Scene* pNewScene )
{
	// Nothing to be done by default
}


Scene* SceneProvider::scene()
{
	return pScene_;
}


const Scene* SceneProvider::scene() const
{
	return pScene_;
}


void * SceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID)
{
	return NULL;
}


} // namespace BW
