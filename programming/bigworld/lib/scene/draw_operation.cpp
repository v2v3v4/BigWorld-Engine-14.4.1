#include "pch.hpp"
#include "draw_operation.hpp"

#include "cstdmf/profiler.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_object.hpp"
#include "scene/scene_type_system.hpp"

namespace BW
{

void DrawOperation::drawAll( const SceneDrawContext & context, 
	const IntersectionSet & visibleObjects )
{
	PROFILER_SCOPED( DrawOperation_drawAll );

	for (SceneTypeSystem::RuntimeTypeID objectType = 0;
			objectType < visibleObjects.maxType(); ++objectType)
	{
		const IntersectionSet::SceneObjectList& objects =
			visibleObjects.objectsOfType( objectType );

		if (!objects.empty())
		{
			this->drawType( context, objectType,
					&objects[0], objects.size() );
		}
	}
}

void DrawOperation::drawType( const SceneDrawContext & context, 
	SceneTypeSystem::RuntimeTypeID objectType,
	const SceneObject* pObjects, size_t count )
{
	IDrawOperationTypeHandler* pTypeHandler =
		this->getHandler( objectType );
	if (pTypeHandler)
	{
		pTypeHandler->doDraw( context, pObjects, count );
	}
}

} // namespace BW

