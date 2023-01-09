#ifndef _TEST_STATICSCENEPROVIDER_HPP
#define _TEST_STATICSCENEPROVIDER_HPP

#include "test_sceneview.hpp"
#include "test_polymesh.hpp"

#include <scene/scene_provider.hpp>
#include <scene/scene_object.hpp>

namespace BW
{

class StaticSceneProvider
	: public SceneProvider
	, public ITestSceneViewProvider
{
public:

	void load();

	// SceneProvider implementations...
	void forEachObject( const ConstSceneObjectCallback & function ) const;
	void forEachObject( const SceneObjectCallback & function );
	void * getView( const SceneTypeSystem::RuntimeTypeID& sceneViewTypeID );

	// ITestSceneViewProvider implementations...
	void helloWorld( ) { successfullyCalled = true; }
	bool success( ) const { return successfullyCalled; }
	bool successfullyCalled;

	// A representation of objects contained within the scene
	BW::vector<PolyMesh> meshObjects;
};

} // namespace BW

#endif // _TEST_STATICSCENEPROVIDER_HPP
