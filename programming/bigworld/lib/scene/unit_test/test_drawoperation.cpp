#include "pch.hpp"
#include "test_drawoperation.hpp"

#include <scene/scene_object.hpp>

namespace BW
{

void DrawOperation::doDraw( const SceneObject & object )
{
	IDrawOperationTypeHandler * pTypeHandler = getHandler( object.type() );
	if (pTypeHandler)
	{
		pTypeHandler->doDraw( object );
	}
}

bool DrawOperation::success( const SceneObject & object )
{
	IDrawOperationTypeHandler * pTypeHandler = getHandler( object.type() );
	return (pTypeHandler && pTypeHandler->success( object ) );
}



DrawSuccessChecker::DrawSuccessChecker( DrawOperation * drawOp ) : 
	drawOp_(drawOp), 
	success_(true)
{

}

void DrawSuccessChecker::checkSuccess( const SceneObject& object )
{
	success_ = success_ && drawOp_->success(object);
}

bool DrawSuccessChecker::result() const
{
	return success_;
}

} // namespace BW
