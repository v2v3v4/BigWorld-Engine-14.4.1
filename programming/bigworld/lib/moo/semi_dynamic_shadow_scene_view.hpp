#ifndef SEMI_DYNAMIC_SHADOW_VIEW_HPP
#define SEMI_DYNAMIC_SHADOW_VIEW_HPP

#include "scene/scene_provider.hpp"

namespace BW
{
namespace Moo
{

class IntVector2
{
public:
	IntVector2()
	{
	}

	IntVector2(int x, int y):
		x(x),
		y(y)
	{
	}

	int32 x;
	int32 y;
};

class IntBoundBox2
{
public:
	IntVector2 min_;
	IntVector2 max_;
};

class ISemiDynamicShadowSceneViewProvider
{
public:
	virtual ~ISemiDynamicShadowSceneViewProvider() {}

	virtual void updateDivisionConfig( IntBoundBox2& bounds, 
		float& divisionSize ) = 0;
	virtual void addDivisionBounds( const IntVector2& divisionCoords, 
		AABB& divisionBounds ) = 0;
};

class SemiDynamicShadowSceneView : 
	public SceneView<ISemiDynamicShadowSceneViewProvider>
{
public:

	bool getDivisionConfig( IntBoundBox2& bounds, 
		float& divisionSize );
	void getDivisionBounds( const IntVector2& divisionCoords, 
		AABB& divisionBounds );
};

}
}

#endif // SEMI_DYNAMIC_SHADOW_VIEW_HPP