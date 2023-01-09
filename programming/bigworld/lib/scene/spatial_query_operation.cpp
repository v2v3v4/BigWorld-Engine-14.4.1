#include "pch.hpp"
#include "spatial_query_operation.hpp"

#include "math/boundbox.hpp"

#include "scene/intersection_set.hpp"
#include "scene/scene_object.hpp"
#include "scene/scene_type_system.hpp"

namespace BW
{

void SpatialQueryOperation::getWorldTransform( const SceneObject& obj, 
	Matrix* xform )
{
	ISpatialQueryOperationTypeHandler* pTypeHandler =
		this->getHandler( obj.type() );
	if (pTypeHandler)
	{
		pTypeHandler->getWorldTransform( obj, xform );
	}
}

void SpatialQueryOperation::getWorldVisibilityBoundingBox( 
	const SceneObject& obj, AABB* bb )
{
	ISpatialQueryOperationTypeHandler* pTypeHandler =
		this->getHandler( obj.type() );
	if (pTypeHandler)
	{
		pTypeHandler->getWorldVisibilityBoundingBox( obj, bb );
	}
}

void SpatialQueryOperation::accumulateWorldVisibilityBoundingBoxes(
	const IntersectionSet& objs, AABB* bb )
{
	for (SceneTypeSystem::RuntimeTypeID objectType = 0;
		objectType < objs.maxType(); ++objectType)
	{
		const IntersectionSet::SceneObjectList& objects =
			objs.objectsOfType( objectType );

		if (!objects.empty())
		{
			this->accumulateWorldVisibilityBoundingBoxes( objectType,
				&objects[0], objects.size(), bb );
		}
	}
}

void SpatialQueryOperation::accumulateWorldVisibilityBoundingBoxes( 
	SceneTypeSystem::RuntimeTypeID objectType,
	const SceneObject * pObjects, size_t count, AABB* bb )
{
	ISpatialQueryOperationTypeHandler* pTypeHandler =
		this->getHandler( objectType );
	if (!pTypeHandler)
	{
		return;
	}
	
	AABB curBB;
	for (size_t index = 0; index < count; ++index)
	{
		const SceneObject& obj = pObjects[index];
		pTypeHandler->getWorldVisibilityBoundingBox( obj, &curBB );
		bb->addBounds( curBB );
	}
}

} // namespace BW

