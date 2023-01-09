#ifndef POLYHEDRON_HPP
#define POLYHEDRON_HPP

#include "vector3.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace Math
{

/**
 *  This class is an interface for a generic polyhedron, and implements
 *  intersection tests against other polyhedra.
 *  @see OrientedBBox
 */
class Polyhedron
{
public:
	class Face
	{
	public:
		Face( Polyhedron& owner, const BW::vector<unsigned int>& idxs );
		Face( Polyhedron& owner, unsigned int idx0, unsigned int idx1, unsigned int idx2 );
		Face( Polyhedron& owner, unsigned int idx0, unsigned int idx1, unsigned int idx2, unsigned int idx3 );

		Vector3 point( int i ) const;
		Vector3 normal() const;

		Face& operator=( const Face& other )
		{
			owner_ = other.owner_;
			idxs_ = other.idxs_;
			normal_ = other.normal_;
			return *this;
		}

	private:
		Polyhedron& owner_;
		BW::vector<unsigned int> idxs_;
		Vector3 normal_;

		void calcNormal();
	};

	class Edge
	{
	public:
		Edge( Polyhedron& owner, unsigned int idx0, unsigned int idx1 );

		Vector3 v0() const;
		Vector3 v1() const;

		Edge& operator=( const Edge& other )
		{
			owner_ = other.owner_;
			idx0_ = other.idx0_;
			idx1_ = other.idx1_;
			return *this;
		}

	private:
		Polyhedron& owner_;
		unsigned int idx0_;
		unsigned int idx1_;
	};

	Polyhedron();
	virtual ~Polyhedron();

	// This method returns the number of points.
	virtual unsigned int numPoints() const = 0;

	// This method returns the number of points.
	virtual unsigned int numFaces() const = 0;

	// This method returns the number of points.
	virtual unsigned int numEdges() const = 0;

	// This method returns a point from the Polyhedron by index.
	virtual Vector3 point( unsigned int i ) const = 0;

	// This method returns a face from the Polyhedron by index.
	virtual Face face( unsigned int i ) const = 0;

	// This method returns an edge from the Polyhedron by index.
	virtual Edge edge( unsigned int i ) const = 0;

	virtual bool intersects( const Polyhedron& other ) const;

protected:
	std::pair<float,float> calcMinMax( const Vector3& separationLine ) const;
};


} // namespace Math

BW_END_NAMESPACE

#endif //POLYHEDRON_HPP
