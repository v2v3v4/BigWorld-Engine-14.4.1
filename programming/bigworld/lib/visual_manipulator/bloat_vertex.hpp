#ifndef VISUAL_MANIPULATOR_BLOAT_VERTEX_HPP
#define VISUAL_MANIPULATOR_BLOAT_VERTEX_HPP

#include "math/vector4.hpp"
#include "math/vector3.hpp"
#include "math/vector2.hpp"

#include "moo/vertex_formats.hpp"


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{

struct BloatVertex
{
	BloatVertex();
	Vector3	pos;
	Vector3	normal;
	Vector3	binormal;
	Vector3	tangent;
	Vector4 colour;
	Vector2	uv;
	Vector2	uv2;
	int		meshIndex;
	int		vertexIndex;
	int		smoothingGroup;
	Vector4 weights;
	uint32  indices[4];

	void output( Moo::VertexXYZNUV& v ) const;
	void output( Moo::VertexXYZNUVIIIWW& v ) const;
	void output( Moo::VertexXYZNUVTB& v ) const;
	void output( Moo::VertexXYZNUVIIIWWTB& v ) const;

	// for creating MeshParticles
	bool operator == ( const BloatVertex& bv ) const;
	bool operator < ( const BloatVertex& bv ) const;
};

}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_BLOAT_VERTEX_HPP
