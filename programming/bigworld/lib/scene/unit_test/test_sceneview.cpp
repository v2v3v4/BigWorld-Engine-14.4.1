#include "pch.hpp"
#include "test_sceneview.hpp"

namespace BW
{

void TestSceneView::helloWorld() const
{
	ProviderCollection::const_iterator iter = providers_.begin(),
		end = providers_.end();
	for (; iter != end; ++iter)
	{
		ProviderInterface * pProvider = *iter;
		pProvider->helloWorld();
	}
}

bool TestSceneView::success() const
{
	bool allSuccessful = true;
	ProviderCollection::const_iterator iter = providers_.begin(),
		end = providers_.end();
	for (; iter != end; ++iter)
	{
		ProviderInterface * pProvider = *iter;
		allSuccessful = allSuccessful && pProvider->success();
	}
	return allSuccessful;
}

} // namespace BW
