#include "pch.hpp"
#include "cached_semi_dynamic_shadow_scene_provider.hpp"

#include "cstdmf/debug.hpp"
#include "math/vector2.hpp"
#include "math/planeeq.hpp"
#include "math/convex_hull.hpp"
#include "math/boundbox.hpp"

#include "scene/scene.hpp"
#include "scene/spatial_query_operation.hpp"
#include "scene/intersect_scene_view.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_intersect_context.hpp"

namespace BW {
namespace CompiledSpace {

struct CachedSemiDynamicShadowSceneProvider::CacheData
{
	bool isCached_;
	AABB bounds_;
};


CachedSemiDynamicShadowSceneProvider::CachedSemiDynamicShadowSceneProvider()
{
	configure( AABB( Vector3::ZERO, Vector3::ZERO ), 1.0f );
}

CachedSemiDynamicShadowSceneProvider::~CachedSemiDynamicShadowSceneProvider()
{

}

void CachedSemiDynamicShadowSceneProvider::configure( 
	const AABB& totalBounds, 
	float divisionSize )
{
	Moo::IntBoundBox2 bounds;
	bounds.max_.x = static_cast<int32>(
		std::floor( totalBounds.maxBounds().x / divisionSize ));
	bounds.max_.y = static_cast<int32>(
		std::floor( totalBounds.maxBounds().z / divisionSize ));
	bounds.min_.x = static_cast<int32>(
		std::floor( totalBounds.minBounds().x / divisionSize ));
	bounds.min_.y = static_cast<int32>(
		std::floor( totalBounds.minBounds().z / divisionSize ));
	configure( bounds, divisionSize );
}

void CachedSemiDynamicShadowSceneProvider::configure( 
	const Moo::IntBoundBox2& bounds, 
	float divisionSize )
{
	divisionBounds_ = bounds;
	divisionSize_ = divisionSize;

	size_t numDivisions = 
		(divisionBounds_.max_.x - divisionBounds_.min_.x + 1) *
		(divisionBounds_.max_.y - divisionBounds_.min_.y + 1);
	
	cache_.resize( numDivisions );

	flushCache();
}

void CachedSemiDynamicShadowSceneProvider::flushCache()
{
	for (size_t index = 0; index < cache_.size(); ++index)
	{
		cache_[index].isCached_ = false;
	}
}

void * CachedSemiDynamicShadowSceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID)
{
	void * result = NULL;

	exposeView<Moo::ISemiDynamicShadowSceneViewProvider>(this, viewTypeID, result);

	return result;
}

void CachedSemiDynamicShadowSceneProvider::updateDivisionConfig( 
	Moo::IntBoundBox2& bounds, 
	float& divisionSize )
{
	divisionSize = divisionSize_;
	bounds.max_.x = std::max( bounds.max_.x, divisionBounds_.max_.x );
	bounds.max_.y = std::max( bounds.max_.y, divisionBounds_.max_.y );
	bounds.min_.x = std::min( bounds.min_.x, divisionBounds_.min_.x );
	bounds.min_.y = std::min( bounds.min_.y, divisionBounds_.min_.y );
}

void CachedSemiDynamicShadowSceneProvider::addDivisionBounds( 
	const Moo::IntVector2& divisionCoords, 
	AABB& divisionBounds )
{
	const AABB &bb = cachedDivisionBounds( divisionCoords );
	divisionBounds.addBounds( bb );
}

const AABB& CachedSemiDynamicShadowSceneProvider::cachedDivisionBounds(
	const Moo::IntVector2& divisionCoords )
{
	size_t internalID = calculateDivisionID( divisionCoords );
	CacheData& data = cache_[internalID];
	AABB& bb = data.bounds_;

	if (!data.isCached_)
	{
		generateDivisionBounds( divisionCoords, bb );
		data.isCached_ = true;
	}

	return bb;
}

size_t CachedSemiDynamicShadowSceneProvider::calculateDivisionID( 
	const Moo::IntVector2& divisionCoords )
{
	MF_ASSERT( isInsideBounds(divisionCoords) );

	// Shift by the min values, to realign to 0 onwards
	Moo::IntVector2 internalCoords( divisionCoords );
	internalCoords.x -= divisionBounds_.min_.x;
	internalCoords.y -= divisionBounds_.min_.y;

	const size_t divisionWidth = 
		divisionBounds_.max_.x - divisionBounds_.min_.x + 1;

	// Now generate id
	const size_t id = internalCoords.x + internalCoords.y * divisionWidth;
	return id;
}

void CachedSemiDynamicShadowSceneProvider::generateDivisionBounds( 
	const Moo::IntVector2& divisionCoords, AABB& bb )
{
	// Work out the 2D bottom left and top right corners of a box
	// on the y=0 plane. (we are going to cull against the column represented)
	Vector3 bottomLeft( 
		static_cast<float>(divisionCoords.x), 
		0.f, 
		static_cast<float>(divisionCoords.y) );
	Vector3 topRight( 
		static_cast<float>(divisionCoords.x + 1), 
		0.f, 
		static_cast<float>(divisionCoords.y + 1) );
	bottomLeft *= divisionSize_;
	topRight *= divisionSize_;

	// Setup a convex hull for the column
	PlaneEq hullPlanes[4];
	hullPlanes[0].init( bottomLeft, -Vector3::I );
	hullPlanes[1].init( bottomLeft, -Vector3::K );
	hullPlanes[2].init( topRight, Vector3::I );
	hullPlanes[3].init( topRight, Vector3::K );
	ConvexHull hull( 4, hullPlanes );

	// Execute a cull on the scene for this division's coordinates.
	IntersectionSet objects;
	SceneIntersectContext cullContext( *scene() );
	const IntersectSceneView* cullView = scene()->getView<IntersectSceneView>();
	MF_ASSERT( cullView );
	cullView->intersect( cullContext, hull, objects );

	// Accumulate the bounding boxes of all these objects returned.
	SpatialQueryOperation *pSpatialQuery = 
		scene()->getObjectOperation<SpatialQueryOperation>();
	MF_ASSERT( pSpatialQuery );

	pSpatialQuery->accumulateWorldVisibilityBoundingBoxes(
		objects, &bb );
}

bool CachedSemiDynamicShadowSceneProvider::isInsideBounds( 
	const Moo::IntVector2& divisionCoords )
{
	return divisionCoords.x <= divisionBounds_.max_.x &&
		divisionCoords.x >= divisionBounds_.min_.x &&
		divisionCoords.y <= divisionBounds_.max_.y &&
		divisionCoords.y >= divisionBounds_.min_.y;
}

} // namespace CompiledSpace
} // namespace BW
