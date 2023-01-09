#ifndef _SPATIAL_QUERY_OPERATION_HPP
#define _SPATIAL_QUERY_OPERATION_HPP

#include "forward_declarations.hpp"

#include "scene_type_system.hpp"
#include "scene_object.hpp"

namespace BW
{

class ISpatialQueryOperationTypeHandler
{
public:
	virtual ~ISpatialQueryOperationTypeHandler(){}

	virtual void getWorldTransform( const SceneObject& obj, 
		Matrix* xform ) = 0;
	virtual void getWorldVisibilityBoundingBox( 
		const SceneObject& obj, 
		AABB* bb ) = 0;
};


class SpatialQueryOperation
	: public SceneObjectOperation<ISpatialQueryOperationTypeHandler>
{
public:

	void getWorldTransform( const SceneObject& obj, 
		Matrix* xform );
	void getWorldVisibilityBoundingBox(
		const SceneObject& obj,
		AABB* bb );
	void accumulateWorldVisibilityBoundingBoxes(
		const IntersectionSet& objs, 
		AABB* bb );
	void accumulateWorldVisibilityBoundingBoxes( 
		SceneTypeSystem::RuntimeTypeID objectType,
		const SceneObject * pObjects, size_t count, AABB* bb );
};

} // namespace BW

#endif // _SPATIAL_QUERY_OPERATION_HPP
