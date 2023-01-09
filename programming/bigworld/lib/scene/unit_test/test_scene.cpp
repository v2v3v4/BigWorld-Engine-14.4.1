#include "pch.hpp"
#include "test_polymesh.hpp"
#include "test_drawoperation.hpp"
#include "test_sceneview.hpp"
#include "test_staticsceneprovider.hpp"

#include "scene/forward_declarations.hpp"
#include "scene/scene.hpp"

#include "cstdmf/guard.hpp"

namespace BW
{

TEST( Scene_Construction )
{
	StaticSceneProvider provider;

	Scene scene;
	scene.addProvider( &provider );

	TestSceneView* pTestScene = 
		scene.getView< TestSceneView >();
	pTestScene->helloWorld();
	CHECK( pTestScene->success() == true );

	DrawOperation* pDrawInterface =
		scene.getObjectOperation<DrawOperation>();
	pDrawInterface->addHandler<PolyMesh, PolyMeshDrawHandler>();
}

} // namespace BW
