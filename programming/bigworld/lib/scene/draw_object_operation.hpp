#ifndef DRAW_OBJECT_OPERATION_HPP
#define DRAW_OBJECT_OPERATION_HPP

#include "scene_lib.hpp"

BW_BEGIN_NAMESPACE

class SceneObject;

class IBatchRenderOperationTypeHandler
{
public:
	virtual void doDrawBatch(
		const SceneObject* objects, size_t count ) = 0;
};

BW_END_NAMESPACE

#endif // DRAW_OBJECT_OPERATION_HPP
