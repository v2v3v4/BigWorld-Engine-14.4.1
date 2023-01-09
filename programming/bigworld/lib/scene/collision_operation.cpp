#include "pch.hpp"

#include "collision_operation.hpp"

namespace BW
{

bool CollisionOperation::collide(
	const SceneObject & object,
	const Vector3 & source,
	const Vector3 & extent,
	const SweepParams& sp,
	CollisionState & state ) const
{
	const ICollisionTypeHandler* pTypeHandler =
		this->getHandler( object.type() );
	if (pTypeHandler)
	{
		return pTypeHandler->doCollide(
			object, source, extent, sp, state );
	}
	else
	{
		return false;
	}
}


bool CollisionOperation::collide(
	const SceneObject & object,
	const WorldTriangle & source,
	const Vector3 & extent,
	const SweepParams& sp,
	CollisionState & state ) const
{
	const ICollisionTypeHandler* pTypeHandler =
			this->getHandler( object.type() );
	if (pTypeHandler)
	{
		return pTypeHandler->doCollide(
			object, source, extent, sp, state );
	}
	else
	{
		return false;
	}
}


} // namespace BW
