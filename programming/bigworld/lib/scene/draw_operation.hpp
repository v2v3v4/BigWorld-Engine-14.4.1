#ifndef _DRAW_OPERATION_HPP
#define _DRAW_OPERATION_HPP

#include "forward_declarations.hpp"

#include "scene_type_system.hpp"
#include "scene_object.hpp"

namespace BW
{

class SceneDrawContext;

class IDrawOperationTypeHandler
{
public:
	virtual ~IDrawOperationTypeHandler(){}

	virtual void doDraw( const SceneDrawContext & drawContext,
		const SceneObject * pObjects, size_t count ) = 0;
};


class DrawOperation
	: public SceneObjectOperation<IDrawOperationTypeHandler>
{
public:

	void drawAll( const SceneDrawContext & drawContext,
		const IntersectionSet & visibleObjects );

	void drawType( const SceneDrawContext & drawContext,
		SceneTypeSystem::RuntimeTypeID objectType,
		const SceneObject * pObjects, size_t count );

};

} // namespace BW

#endif // _DRAW_OPERATION_HPP
