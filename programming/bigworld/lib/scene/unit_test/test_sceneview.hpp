#ifndef _TEST_SCENEVIEW_HPP
#define _TEST_SCENEVIEW_HPP

#include "scene/forward_declarations.hpp"
#include "scene/scene_provider.hpp"

namespace BW
{

class ITestSceneViewProvider
{
public:

	virtual void helloWorld( ) = 0;
	virtual bool success() const = 0;
};


class TestSceneView
	: public SceneView<ITestSceneViewProvider>
{
public:
	void helloWorld( ) const;
	bool success( ) const;
};

} // namespace BW

#endif // _TEST_SCENEVIEW_HPP
