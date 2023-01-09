#ifndef _TEST_DRAWOPERATION_HPP
#define _TEST_DRAWOPERATION_HPP

#include <scene/scene_object.hpp>
#include <scene/scene_type_system.hpp>

namespace BW
{

class IDrawOperationTypeHandler
{
public:
	virtual ~IDrawOperationTypeHandler(){}

	virtual void doDraw( const SceneObject & object ) const = 0;
	virtual bool success( const SceneObject & object ) const = 0;
};

class DrawOperation
	: public SceneObjectOperation<IDrawOperationTypeHandler>
{
public:
	void doDraw( const SceneObject & object );
	bool success( const SceneObject & object );
};


class DrawSuccessChecker
{
public:
	DrawSuccessChecker(DrawOperation * drawOp);
	void checkSuccess(const SceneObject& object);
	bool result() const;

private:
	DrawOperation * drawOp_;
	bool success_;
};

} // namespace BW

#endif // _TEST_DRAWOPERATION_HPP
