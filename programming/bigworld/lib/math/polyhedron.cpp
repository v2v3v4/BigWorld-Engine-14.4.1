
#include "pch.hpp"
#include "polyhedron.hpp"
#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

namespace Math
{

///////////////////////////////////////////////////////////////////////////////
// Section: Polyhedron::Face
///////////////////////////////////////////////////////////////////////////////


/**
 *  Generic Constructor.
 *
 *  @param owner	Polyhedron object that contains the face.
 *  @param idxs		Indices to the points that for the face (co-planar).
 */
Polyhedron::Face::Face( Polyhedron& owner, const BW::vector<unsigned int>& idxs ) :
	owner_(owner),
	idxs_(idxs)
{
	MF_ASSERT( idxs_.size() >= 3 );
	calcNormal();
}


/**
 *  Triangle Constructor.
 *
 *  @param owner	Polyhedron object that contains the face.
 *  @param idx0		Index to the first point of the face.
 *  @param idx1		Index to the second point of the face.
 *  @param idx2		Index to the third point of the face.
 */
Polyhedron::Face::Face( Polyhedron& owner, unsigned int idx0, unsigned int idx1, unsigned int idx2 ) :
	owner_(owner)
{
	idxs_.push_back( idx0 );
	idxs_.push_back( idx1 );
	idxs_.push_back( idx2 );
	calcNormal();
}


/**
 *  Quad Constructor.
 *
 *  @param owner	Polyhedron object that contains the face.
 *  @param idx0		Index to the first point of the face.
 *  @param idx1		Index to the second point of the face.
 *  @param idx2		Index to the third point of the face.
 *  @param idx3		Index to the fourth point of the face.
 */
Polyhedron::Face::Face( Polyhedron& owner, unsigned int idx0, unsigned int idx1, unsigned int idx2, unsigned int idx3 ) :
	owner_(owner)
{
	idxs_.push_back( idx0 );
	idxs_.push_back( idx1 );
	idxs_.push_back( idx2 );
	idxs_.push_back( idx3 );
	calcNormal();
}


/**
 *  This private method calculates the normal of the face. It assumes all
 *  points in the face are co-planar (as should be in a Polyhedron).
 */
void Polyhedron::Face::calcNormal()
{
	// Faces must be co-planar
	Vector3 edge1 = owner_.point(1) - owner_.point(0);
	Vector3 edge2 = owner_.point(2) - owner_.point(0);
	normal_ = edge1.crossProduct( edge2 );
	normal_.normalise();
}


/**
 *  This method returns a point from the face by index.
 *
 *  @param i	Index to the desired point
 *  @return		Vector3 containing the point.
 */
Vector3 Polyhedron::Face::point( int i ) const
{
	return owner_.point( idxs_[i] );
}


/**
 *  This method returns the normal vector of the face.
 *
 *  @return		Vector3 containing the normal of the face.
 */
Vector3 Polyhedron::Face::normal() const
{
	return normal_;
}


///////////////////////////////////////////////////////////////////////////////
// Section: Polyhedron::Edge
///////////////////////////////////////////////////////////////////////////////


/**
 *  Triangle Constructor.
 *
 *  @param owner	Polyhedron object that contains the edge.
 *  @param idx0		Index to the first point of the edge.
 *  @param idx1		Index to the second point of the edge.
 */
Polyhedron::Edge::Edge( Polyhedron& owner, unsigned int idx0, unsigned int idx1 ) :
	owner_(owner),
	idx0_(idx0),
	idx1_(idx1)
{
}


/**
 *  This method returns the first point of the edge.
 *
 *  @return		Vector3 containing the point.
 */
Vector3 Polyhedron::Edge::v0() const
{
	return owner_.point( idx0_ );
}


/**
 *  This method returns the second point of the edge.
 *
 *  @return		Vector3 containing the point.
 */
Vector3 Polyhedron::Edge::v1() const
{
	return owner_.point( idx1_ );
}


///////////////////////////////////////////////////////////////////////////////
// Section: Polyhedron
///////////////////////////////////////////////////////////////////////////////


/**
 *  Default Constructor.
 */
Polyhedron::Polyhedron()
{
}


/**
 *  Destructor.
 */
Polyhedron::~Polyhedron()
{
}


/**
 *  This helper method projects all polyhedron's points onto the separation
 *  line, and returns the minimum and maximum position after projection. Used
 *  when testing intersection in 'intersects', using the separation of axes
 *  theorem.
 *
 *  @param separationLine		Normal of the separation line.
 *  @return						Pair containing the minimum and maximum
 *                              projected position, in that order.
 *  @see intersects
 */
std::pair<float,float> Polyhedron::calcMinMax( const Vector3& separationLine ) const
{
	std::pair<float,float> minMax( std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() );
	for (unsigned int i = 0; i < numPoints(); i++)
	{
		float projection = separationLine.dotProduct( this->point(i) );
		if ( minMax.first > projection ) minMax.first = projection;
		if ( minMax.second < projection ) minMax.second = projection;
	}
	return minMax;
}


/**
 *  This method performs an intersection test against another Polyhedron, using
 *  the separation of axes theorem. This implementation doesn't assume any
 *  particular ordering of the polyhedron's points.
 *
 *  @param other	The other Polyhedron object to test for intersection.
 *  @return			True if the polyhedron intersect, false otherwise.
 */
bool Polyhedron::intersects( const Polyhedron& other ) const
{
	// Test faces of "this" for separation.
	for (unsigned int i = 0; i < numFaces(); i++)
	{
		Vector3 faceNormal = this->face(i).normal(); // outward pointing
		std::pair<float,float> thisMinMax = this->calcMinMax( faceNormal );
		std::pair<float,float> otherMinMax = other.calcMinMax( faceNormal );
		if ( thisMinMax.first > otherMinMax.second || otherMinMax.first > thisMinMax.second )
			return false;
	}

	// Test faces of "other" for separation.
	for (unsigned int i = 0; i < other.numFaces(); i++)
	{
		Vector3 faceNormal = other.face(i).normal(); // outward pointing
		std::pair<float,float> thisMinMax = this->calcMinMax( faceNormal );
		std::pair<float,float> otherMinMax = other.calcMinMax( faceNormal );
		if ( thisMinMax.first > otherMinMax.second || otherMinMax.first > thisMinMax.second )
			return false;
	}

	// Test edges for separation.
	for ( unsigned int i = 0; i < numEdges(); i++ )
	{
		for ( unsigned int j = 0; j < other.numEdges(); j++ )
		{
			Vector3 thisDir = this->edge(i).v1() - this->edge(i).v0();
			Vector3 otherDir = other.edge(j).v1() - other.edge(j).v0();
			Vector3 edgeXProd = thisDir.crossProduct( otherDir );
			std::pair<float,float> thisMinMax = this->calcMinMax( edgeXProd );
			std::pair<float,float> otherMinMax = other.calcMinMax( edgeXProd );
			if ( thisMinMax.first > otherMinMax.second || otherMinMax.first > thisMinMax.second )
				return false;
		}
	}
	return true;
}

BW_END_NAMESPACE

} // namespace Math

