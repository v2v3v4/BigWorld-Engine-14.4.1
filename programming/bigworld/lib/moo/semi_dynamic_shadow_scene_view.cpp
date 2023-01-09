#include "pch.hpp"
#include "semi_dynamic_shadow_scene_view.hpp"

namespace BW
{
namespace Moo
{

bool SemiDynamicShadowSceneView::getDivisionConfig( 
	IntBoundBox2& o_bounds, 
	float& o_divisionSize )
{
	float divisionSize = 0.0f;
	IntBoundBox2 bounds;
	bounds.max_.x = std::numeric_limits<int32>::min();
	bounds.max_.y = std::numeric_limits<int32>::min();
	bounds.min_.x = std::numeric_limits<int32>::max();
	bounds.min_.y = std::numeric_limits<int32>::max();

	for ( ProviderCollection::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ProviderInterface * pProvider = *iter;
		float curDivisionSize = 0.0f;
		pProvider->updateDivisionConfig( bounds, curDivisionSize );

		// NOTE: This assertion makes sure all scene providers are providing 
		// the same information regarding the scene structure.
		// Using equals operator on floats in this case is INTENDED.
		MF_ASSERT( divisionSize == 0.0f || curDivisionSize == divisionSize );
		divisionSize = curDivisionSize;
	}

	o_bounds = bounds;
	o_divisionSize = divisionSize;
	return o_divisionSize != 0.0f &&
		bounds.max_.x >= bounds.min_.x &&
		bounds.max_.y >= bounds.min_.y;
}

void SemiDynamicShadowSceneView::getDivisionBounds( 
	const IntVector2& divisionCoords, 
	AABB& divisionBounds )
{
	AABB bb;
	for ( ProviderCollection::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ProviderInterface * pProvider = *iter;
		pProvider->addDivisionBounds( divisionCoords, bb );
	}
	divisionBounds = bb;
}

} // End namespace Moo 
} // End namespace BW

// semi_dynamic_shadow_view.cpp