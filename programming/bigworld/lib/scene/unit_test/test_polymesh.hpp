#ifndef _TEST_POLYMESH_HPP
#define _TEST_POLYMESH_HPP

#include "test_drawoperation.hpp"

#include <scene/scene_object.hpp>

namespace BW
{

class PolyMesh
{
public:
	PolyMesh();
	void doDraw();
	bool success() const;
	int value; // Dummy value used for diagnostics.
};

class PolyMeshDrawHandler
	: public IDrawOperationTypeHandler
{
public:
	void doDraw( const SceneObject & object ) const;
	bool success( const SceneObject & object ) const;
};

} // namespace BW

#endif // _TEST_POLYMESH_HPP
