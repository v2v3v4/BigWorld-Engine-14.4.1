#include "pch.hpp"
#include "test_polymesh.hpp"

#include <scene/scene_object.hpp>

namespace BW
{

PolyMesh::PolyMesh() : value(0)
{

}

void PolyMesh::doDraw()
{
	value = 1;
}

bool PolyMesh::success() const
{
	return (value == 1);
}

void PolyMeshDrawHandler::doDraw( const SceneObject & object ) const
{
	PolyMesh* pPolyMesh = object.getAs<PolyMesh>();
	pPolyMesh->doDraw();
}

bool PolyMeshDrawHandler::success( const SceneObject & object ) const
{
	PolyMesh* pPolyMesh = object.getAs<PolyMesh>();
	return pPolyMesh->success();
}

} // namespace BW
