#include "pch.hpp"
#include "test_staticsceneprovider.hpp"

#include "scene/forward_declarations.hpp"
#include "scene/scene_object.hpp"

namespace BW
{

void StaticSceneProvider::load()
{
	// We're mimicking the loading of a static scene from some data
	// source. We'll just populate our object vectors with default
	// construction values.
	const size_t numPolyMeshes = 10;
	meshObjects.resize(numPolyMeshes);
}

void StaticSceneProvider::forEachObject( const ConstSceneObjectCallback & function ) const
{
	const size_t numPolyMeshes = meshObjects.size();
	for (size_t i = 0; i < numPolyMeshes; ++i)
	{
		SceneObject obj( &meshObjects[i], SceneObjectFlags() );
		function(obj);
	}
}

void StaticSceneProvider::forEachObject( const SceneObjectCallback & function )
{
	const size_t numPolyMeshes = meshObjects.size();
	for (size_t i = 0; i < numPolyMeshes; ++i)
	{
		SceneObject obj( &meshObjects[i], SceneObjectFlags() );
		function(obj);
	}
}

void * StaticSceneProvider::getView( const SceneTypeSystem::RuntimeTypeID& viewTypeID )
{
	void * result = NULL;

	exposeView<ITestSceneViewProvider>(this, viewTypeID, result);

	return result;
}

} // namespace BW
