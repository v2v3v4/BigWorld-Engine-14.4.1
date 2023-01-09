#ifndef ORIENTED_BBOX_HPP
#define ORIENTED_BBOX_HPP

#include "polyhedron.hpp"

BW_BEGIN_NAMESPACE

class BoundingBox;
class Matrix;

namespace Math
{

/**
 *  This class implements an oriented bounding box. At the moment, this class
 *  is only useful for doing intersection tests with oriented bounding boxes.
 *  @see Polyhedron
 */
class OrientedBBox : public Polyhedron
{
public:
	OrientedBBox();

	OrientedBBox( const BoundingBox& bb, const Matrix& matrix );

	void create( const BoundingBox& bb, const Matrix& matrix );

	unsigned int numPoints() const;

	unsigned int numFaces() const;

	unsigned int numEdges() const;

	Vector3 point( unsigned int i ) const;

	Face face( unsigned int i ) const;

	Edge edge( unsigned int i ) const;

private:
	BW::vector<Vector3> points_;
	BW::vector<Face> faces_;
	BW::vector<Edge> edges_;
};

} // namespace Math

BW_END_NAMESPACE

#endif //ORIENTED_BBOX_HPP
