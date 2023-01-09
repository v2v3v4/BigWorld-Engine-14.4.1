
#include "pch.hpp"
#include "boundbox.hpp"
#include "oriented_bbox.hpp"

BW_BEGIN_NAMESPACE

namespace Math
{

///////////////////////////////////////////////////////////////////////////////
// Section: OrientedBBox
///////////////////////////////////////////////////////////////////////////////


/**
 *  Default Constructor.
 */
OrientedBBox::OrientedBBox()
{
	create( BoundingBox( Vector3( 0,0,0 ), Vector3( 0,0,0 ) ), Matrix::identity );
}


/**
 *  Constructor.
 *
 *  @param bb		Axis-oriented bounding box, before any transformation.
 *  @param matrix	Transform matrix of the final oriented bounding box.
 */
OrientedBBox::OrientedBBox( const BoundingBox& bb, const Matrix& matrix )
{
	create( bb, matrix );
}


/**
 *  This method initialises the object from an axis-aligned bounding box and
 *  a transformation matrix.
 *
 *  @param bb		Axis-oriented bounding box, before any transformation.
 *  @param matrix	Transform matrix of the final oriented bounding box.
 */
void OrientedBBox::create( const BoundingBox& bb, const Matrix& matrix )
{
	points_.clear();
	faces_.clear();
	edges_.clear();

	points_.push_back( matrix.applyPoint( Vector3( bb.minBounds()[0], bb.minBounds()[1], bb.minBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.minBounds()[0], bb.minBounds()[1], bb.maxBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.maxBounds()[0], bb.minBounds()[1], bb.maxBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.maxBounds()[0], bb.minBounds()[1], bb.minBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.minBounds()[0], bb.maxBounds()[1], bb.minBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.minBounds()[0], bb.maxBounds()[1], bb.maxBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.maxBounds()[0], bb.maxBounds()[1], bb.maxBounds()[2] ) ) );
	points_.push_back( matrix.applyPoint( Vector3( bb.maxBounds()[0], bb.maxBounds()[1], bb.minBounds()[2] ) ) );

	faces_.push_back( Face( *this, 0, 1, 2, 3 ) );
	faces_.push_back( Face( *this, 4, 7, 6, 5 ) );
	faces_.push_back( Face( *this, 0, 3, 7, 4 ) );
	faces_.push_back( Face( *this, 3, 2, 6, 7 ) );
	faces_.push_back( Face( *this, 2, 1, 5, 6 ) );
	faces_.push_back( Face( *this, 1, 0, 4, 5 ) );

	edges_.push_back( Edge( *this, 0, 1 ) );
	edges_.push_back( Edge( *this, 1, 2 ) );
	edges_.push_back( Edge( *this, 2, 3 ) );
	edges_.push_back( Edge( *this, 3, 0 ) );
	edges_.push_back( Edge( *this, 4, 5 ) );
	edges_.push_back( Edge( *this, 5, 6 ) );
	edges_.push_back( Edge( *this, 6, 7 ) );
	edges_.push_back( Edge( *this, 7, 4 ) );
	edges_.push_back( Edge( *this, 0, 4 ) );
	edges_.push_back( Edge( *this, 1, 5 ) );
	edges_.push_back( Edge( *this, 2, 6 ) );
	edges_.push_back( Edge( *this, 3, 7 ) );
}


/**
 *  This method returns the number of points.
 *
 *  @return		Number of points.
 */
unsigned int OrientedBBox::numPoints() const
{
	size_t numPoints = points_.size();
	MF_ASSERT( numPoints <= UINT_MAX );
	return ( unsigned int )numPoints;
}


/**
 *  This method returns the number of faces.
 *
 *  @return		Number of faces.
 */
unsigned int OrientedBBox::numFaces() const
{
	size_t numFaces = faces_.size();
	MF_ASSERT( numFaces <= UINT_MAX );
	return ( unsigned int )numFaces;
}

/**
 *  This method returns the number of edges.
 *
 *  @return		Number of edges.
 */
unsigned int OrientedBBox::numEdges() const
{
	size_t numEdges = edges_.size();
	MF_ASSERT( numEdges <= UINT_MAX );
	return ( unsigned int )numEdges;
}


/**
 *  This method returns a point from the Polyhedron by index.
 *
 *  @param i	Index to the desired point
 *  @return		Vector3 containing the point.
 */
Vector3 OrientedBBox::point( unsigned int i ) const
{
	return points_[i];
}


/**
 *  This method returns a face from the Polyhedron by index.
 *
 *  @param i	Index to the desired face
 *  @return		The desired face.
 */
Polyhedron::Face OrientedBBox::face( unsigned int i ) const
{
	return faces_[i];
}


/**
 *  This method returns an edge from the Polyhedron by index.
 *
 *  @param i	Index to the desired edge
 *  @return		The desired edge.
 */
Polyhedron::Edge OrientedBBox::edge( unsigned int i ) const
{
	return edges_[i];
}

BW_END_NAMESPACE

} // namespace Math
