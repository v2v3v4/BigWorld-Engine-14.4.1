#include "pch.hpp"
#include "bsp.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/static_array.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/vectornodest.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"


DECLARE_DEBUG_COMPONENT2( "Physics", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
	#include "bsp.ipp"
#endif

// enable runtime bsp valiation
#define BSP_RUNTIME_VALIDATION

typedef int32 OFFSET_32BIT;
const int32 OFFSET_32BIT_NULL = 0xFFFFFFFF;
const int32 MAX_OFFSET_32BIT = 0x7FFFFFFF;

/**
 *	This class is used to provide access to BSP node.
 *  It holds pointers to its own triangles and triangles which are shared with another nodes.
 *  Note that data for the class itself and its data is allocated from the BSPTree class.
 */
class BSPNode
{
public:
	bool intersects( const WorldTriangle &	triangle,
					const WorldTriangle **	ppHitTriangle = NULL ) const;

	bool intersects( const WorldTriangle &	triangle,
					const Vector3 &			translation,
					CollisionVisitor *		pVisitor = NULL ) const;

	bool intersectsThisNode( const WorldTriangle & triangle,
							const WorldTriangle ** ppHitTriangle) const;

	bool intersectsThisNode( const Vector3&		start,
							const Vector3&		end,
							float&				dist,
							const WorldTriangle** ppHitTriangle,
							CollisionVisitor*	pVisitor ) const;

	bool intersectsThisNode( const WorldTriangle& triangle,
							const Vector3&		translation,
							CollisionVisitor*	pVisitor ) const;

	// pointers to front and back child, NULL if there is no child
	BSPNode*		pFront_;
	BSPNode*		pBack_;
	// splitting plane 
	PlaneEq			planeEq_;
	// pointers to BSPTree data
	// triangles which belong to this node
	WorldTriangle*	pTriangles_;
	uint32			numTriangles_;
	// shared triangles which also belong to this node
	WorldTriangle**	ppSharedTriangles_;
	uint32			numSharedTriangles_;
	// WARNING: If you want to add a new member which needs to be saved
	// on disk you have to modify the following PACKED_SIZE constant,
	// bump bsp version and write new save/load function.
	// loadBSP function should also support the old format

	// amount of memory for 1 BSPNode when it's saved on disk
	static const uint32 PACKED_SIZE = 
				uint32(sizeof (OFFSET_32BIT ) * 2 + sizeof( PlaneEq ) +
				sizeof( OFFSET_32BIT ) * 2 + sizeof( uint32 ) * 2);

};

/// This constant is the tolerance from the plane equation that a triangle is
/// added to the "on" set.
namespace
{
const float BSPNODE_TOLERANCE = 0.01f;

enum BSPSide
{
	BSP_ON		= 0x0,	///< Lies on the partitioning plane.
	BSP_FRONT	= 0x1, 	///< Lies in front of the partitioning plane.
	BSP_BACK	= 0x2,	///< Lies behind the partitioning plane.
	BSP_BOTH	= 0x3	///< Crosses over the partitioning plane.
};

/**
 *	This method returns which side of the plane the triangle is on.
 */
BSPSide		whichSideOfPlane( const PlaneEq& plane, const WorldTriangle& triangle )
{
	BSPSide side = BSP_ON;

	float dist0 = plane.distanceTo( triangle.v0() );
	float dist1 = plane.distanceTo( triangle.v1() );
	float dist2 = plane.distanceTo( triangle.v2() );

	// Is the triangle in front of the plane?

	if (dist0 > BSPNODE_TOLERANCE ||
		dist1 > BSPNODE_TOLERANCE ||
		dist2 > BSPNODE_TOLERANCE)
	{
		side = (BSPSide)(side | 0x1);
	}

	// Is the triangle behind the plane?

	if (dist0 < -BSPNODE_TOLERANCE ||
		dist1 < -BSPNODE_TOLERANCE ||
		dist2 < -BSPNODE_TOLERANCE)
	{
		side = (BSPSide)(side | 0x2);
	}

	return side;
}

/**
 *	This method returns which side of the plane BSP node the polygon is on.
 */
BSPSide		whichSideOfPlane( const PlaneEq& plane, const WorldPolygon & poly )
{
	BSPSide side = BSP_ON;

	WorldPolygon::const_iterator iter = poly.begin();

	while (iter != poly.end())
	{
		float dist = plane.distanceTo( *iter );

		if (dist > BSPNODE_TOLERANCE)
		{
			// Is this point on the front side?
			side = (BSPSide)(side | 0x1);
		}
		else if (dist < -BSPNODE_TOLERANCE)
		{
			// Is this point on the back side?
			side = (BSPSide)(side | 0x2);
		}

		iter++;
	}

	return side;
}

}

/**
 *	This method returns whether the volume formed by moving the input
 *	triangle by a given translation intersects any triangle assigned
 *	to this node.
 */
bool BSPNode::intersectsThisNode( const WorldTriangle & triangle,
								const Vector3 & translation,
								CollisionVisitor * pVisitor ) const
{
	for (uint i = 0; i < numTriangles_ + numSharedTriangles_ ; ++i)
	{
		WorldTriangle& candidate = i < numTriangles_ ? 
			pTriangles_[i] : *ppSharedTriangles_[i - numTriangles_];

		if (candidate.collisionFlags() != TRIANGLE_NOT_IN_BSP)
		{
			if (candidate.intersects( triangle, translation ))
			{
				if (pVisitor == NULL || pVisitor->visit( candidate, 0.0f ))
				{
					return true;
				}
			}
		}
	}
	return false;
}

/**
 *	This method returns whether the input triangle intersects any triangle
 *	assigned to this node.
 */
bool BSPNode::intersectsThisNode(const WorldTriangle & triangle,
								const WorldTriangle ** ppHitTriangle) const
{
	for (uint i = 0; i < numTriangles_ + numSharedTriangles_ ; ++i)
	{
		WorldTriangle& candidate = i < numTriangles_ ? 
			pTriangles_[i] : *ppSharedTriangles_[i - numTriangles_];

		if (candidate.collisionFlags() != TRIANGLE_NOT_IN_BSP)
		{
			if (candidate.intersects(triangle))
			{
				if (ppHitTriangle != NULL)
				{
					*ppHitTriangle = &triangle;
				}
				return true;
			}
		}
	}
	return false;
}

/**
 *	This method returns whether the input interval intersects a triangle
 *	assigned to this node. If it does, dist is set to the fraction of the
 *	distance along the vector that the intersection point lies.
 *
 *	@see BSPNode::intersects
 */
bool BSPNode::intersectsThisNode(const Vector3 &	start,
						 const Vector3 &		end,
						 float &				dist,
						 const WorldTriangle ** ppHitTriangle,
						 CollisionVisitor *		pVisitor ) const
{
	bool bIntersects = false;

	const Vector3 directionVec(end - start);

	for (uint i = 0; i < numTriangles_ + numSharedTriangles_ ; ++i)
	{
		WorldTriangle& candidate = i < numTriangles_ ? 
			pTriangles_[i] : *ppSharedTriangles_[i - numTriangles_];

		if (candidate.collisionFlags() != TRIANGLE_NOT_IN_BSP)
		{
			float originalDist = dist;

			if (candidate.intersects(start, directionVec, dist) &&
				(!pVisitor || pVisitor->visit( candidate, dist )))
			{
				bIntersects = true;

				if (ppHitTriangle != NULL)
				{
					*ppHitTriangle = &candidate;
				}
			}
			else
			{
				dist = originalDist;
			}
		}
	}

	return bIntersects;
}
/**
 *	This is a helper class for the intersects method
 */
class NullCollisionVisitor : public CollisionVisitor
{
public:
	virtual bool visit( const WorldTriangle &, float )
	{
		return true;
	}
};

static NullCollisionVisitor	s_nullCV;

namespace
{
//
// Helper methods for intersects.
//

/**
 *	This function returns the minimum of the 3 input values.
 */
template <class T> inline float min3(T a, T b, T c)
{
	return a < b ?
		(a < c ? a : c) :
		(b < c ? b : c);
}


/**
 *	This function returns the maximum of the 3 input values.
 */
template <class T> inline float max3(T a, T b, T c)
{
	return a > b ?
		(a > c ? a : c) :
		(b > c ? b : c);
}

}

/**
 *	This method returns whether the input triangle interesects any triangle
 *	assigned to this node or any of its children.
 *
 *	@see BSPNode::intersectThisNode
 */
bool BSPNode::intersects( const WorldTriangle & triangle,
	const WorldTriangle ** ppHitTriangle ) const
{
	bool bIntersects = false;

	if (pBack_ == NULL && pFront_ == NULL)
	{
		bIntersects = this->intersectsThisNode(triangle, ppHitTriangle);
	}
	else
	{
		float d0 = planeEq_.distanceTo(triangle.v0());
		float d1 = planeEq_.distanceTo(triangle.v1());
		float d2 = planeEq_.distanceTo(triangle.v2());

		float min = min3(d0, d1, d2);
		float max = max3(d0, d1, d2);

		// Check the side with the first point of the triangle first.

		if (d0 < 0.f)
		{
			if (pBack_ && min < BSPNODE_TOLERANCE)
			{
				bIntersects = pBack_->intersects(triangle, ppHitTriangle);
			}

			if (min < BSPNODE_TOLERANCE && max > -BSPNODE_TOLERANCE && !bIntersects)
			{
				bIntersects = this->intersectsThisNode(triangle, ppHitTriangle);
			}

			if (pFront_ && max > -BSPNODE_TOLERANCE && !bIntersects)
			{
				bIntersects = pFront_->intersects(triangle,	ppHitTriangle);
			}
		}
		else
		{
			if (pFront_ && max > -BSPNODE_TOLERANCE)
			{
				bIntersects = pFront_->intersects(triangle,	ppHitTriangle);
			}

			if (min < BSPNODE_TOLERANCE && max > -BSPNODE_TOLERANCE && !bIntersects)
			{
				bIntersects = this->intersectsThisNode(triangle, ppHitTriangle);
			}

			if (pBack_ && min < BSPNODE_TOLERANCE && !bIntersects)
			{
				bIntersects = pBack_->intersects(triangle, ppHitTriangle);
			}
		}
	}

	return bIntersects;
}

/**
 *	This method returns whether the volume formed by moving the input
 *	triangle by a given translation intersects any triangle assigned
 *	to this node or any of its children.
 */
bool BSPNode::intersects( const WorldTriangle & triangle,
	const Vector3 & translation,
	CollisionVisitor * pVisitor ) const
{
	// if we're not partitioned it's easy
	if (pBack_ == NULL && pFront_ == NULL)
	{
		return this->intersectsThisNode( triangle, translation, pVisitor );
	}

	// ok, see if the volume crosses this plane
	float dA0 = planeEq_.distanceTo(triangle.v0());
	float dA1 = planeEq_.distanceTo(triangle.v1());
	float dA2 = planeEq_.distanceTo(triangle.v2());
	float dB0 = planeEq_.distanceTo(triangle.v0()+translation);
	float dB1 = planeEq_.distanceTo(triangle.v1()+translation);
	float dB2 = planeEq_.distanceTo(triangle.v2()+translation);

	float min = std::min( min3(dA0, dA1, dA2), min3(dB0, dB1, dB2) );
	float max = std::max( max3(dA0, dA1, dA2), max3(dB0, dB1, dB2) );

	// TODO: Unroll the recursion to increase performance.

	if (min < BSPNODE_TOLERANCE && max > -BSPNODE_TOLERANCE)
	{
		if (this->intersectsThisNode( triangle, translation, pVisitor ))
			return true;
	}

	if (pBack_ && min < BSPNODE_TOLERANCE)
	{
		if (pBack_->intersects( triangle, translation, pVisitor ))
			return true;
	}

	if (pFront_ && max > -BSPNODE_TOLERANCE)
	{
		if (pFront_->intersects( triangle, translation, pVisitor ))
			return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Section: BSP Constructor
// -----------------------------------------------------------------------------
/**
 *	This class is used to construct bsp tree from provided list of triangles.
 *  It uses random 15 splitting planes and chooses the one which results in less splits.
 *  BSPTreeTool::loadBSPLegacy(...) uses this class internals to convert legacy bsp format to the new bsp format
 */
class BSPTreeTool::BSPConstructor
{
public:
	static const uint		MAX_TRIANGLES_PER_NODE = 10;
	/**
	 *	Construction time node
	 *  Stores similar set of data as a runtime bsp node
	 */
	struct Node
	{
		int				frontIndex_;
		int				backIndex_;
		PlaneEq			splittingPlane_;
		WTriangleSet	triangles_;
		WPolygonSet		polygons_;
		bool			partitioned_;

		Node():
			frontIndex_( -1 ),
			backIndex_( -1 ),
			partitioned_( false )
		{	}
	};

	/**
	 *	This structure represents a triangles and list of nodes which include it
	 *  It is used to sort list of triangles by node id
	 */
	struct WorldTriangleRef
	{
		const WorldTriangle*	triangle;
		// indices of nodes which include this triangle
		BW::vector<int>			nodes;

		bool operator<(const WorldTriangleRef& other) const
		{
			size_t len = std::min(nodes.size(), other.nodes.size());
			for (size_t i = 0; i < len; i++)
			{
				if (nodes[i] != other.nodes[i])
				{
					return nodes[i] < other.nodes[i];
				}
			}
			return false;
		}
	};

public:
	// default constructor
	BSPConstructor( const RealWTriangleSet & triangles );
	// override to support loading legacy bsp format
	BSPConstructor( const RealWTriangleSet & triangles, const BW::vector<Node>&	nodes );
	// export constructed bsp tree into the runtime bsp structure
	void			exportAsTree(BSPTree* pBSP);

private:
	void			construct();
	void			partitionNode( Node& node, Node& frontNode, Node& backNode );

	PlaneEq			selectSplittingPlaneRandom15( const WTriangleSet& triangles,
		const WPolygonSet& polygons, uint& bestBoth, uint& bestFront, uint& bestBack, uint& bestOn );

	void			validate() const;

	const RealWTriangleSet&		triangles_;
	BW::vector<Node>			nodes_;
};

/**
 *	Default constucture.
 * Will invoke construct method and generate bsp tree from provided array of triangles
 */
BSPTreeTool::BSPConstructor::BSPConstructor( const RealWTriangleSet & triangles ):
triangles_( triangles )
{
	construct();
}

/**
 *	Overriden version of constructor which uses prebuilt nodes to support bsp legacy file format.
 */
BSPTreeTool::BSPConstructor::BSPConstructor( const RealWTriangleSet & triangles,
												const BW::vector<Node>&	nodes ):
triangles_( triangles ),
nodes_ ( nodes )
{
}

/**
 *	This method validates internal state of bsp constructor. It asserts if something
 *  is wrong or inconsistent.
 */
void BSPTreeTool::BSPConstructor::validate() const
{
#ifdef BSP_RUNTIME_VALIDATION
	MF_ASSERT(nodes_.size() > 0 );
	for (uint i = 0; i < nodes_.size(); i++)
	{
		const Node& n = nodes_[i];
		if (n.partitioned_)
		{
			// node has to have at least one child if is partitioned
			MF_ASSERT((n.frontIndex_ > 0 && n.frontIndex_ < (int)nodes_.size()) ||
						(n.backIndex_ > 0 && n.backIndex_ < (int)nodes_.size()));
			bool foundSplitPlane = false;
			for (uint triIndex = 0; triIndex < n.triangles_.size() && !foundSplitPlane; triIndex++)
			{
				PlaneEq testPlane;
				testPlane.init( n.triangles_[triIndex]->v0(),
						n.triangles_[triIndex]->v1(),
						n.triangles_[triIndex]->v2() );
				foundSplitPlane = almostEqual(testPlane.d(), n.splittingPlane_.d()) && 
								almostEqual(testPlane.normal(), n.splittingPlane_.normal() );
			}
			// if node has been partitioned, then one of its triangles
			// has to be the node's splitting plane
			MF_ASSERT(foundSplitPlane);
		}
		else
		{
			MF_ASSERT(n.frontIndex_ == -1);
			MF_ASSERT(n.backIndex_ == -1);
			PlaneEq dummyPlane;
			dummyPlane.init( Vector3( 0.f, 0.f, 0.f ), Vector3( 1.f, 0.f, 0.f ) );
			MF_ASSERT( almostEqual(n.splittingPlane_.d(), dummyPlane.d() ));
			MF_ASSERT( almostEqual(n.splittingPlane_.normal(), dummyPlane.normal() ));

		}
	}
#endif
}


/**
 *	This method constructs bsp tree using provided array of triangles.
 *  Triangle array must be provided in constructor
 */
void BSPTreeTool::BSPConstructor::construct()
{
	// create an initial triangle array in the root node
	Node rootNode;
	rootNode.triangles_.resize( triangles_.size() );

	for (uint i = 0; i < triangles_.size(); i++)
	{
		rootNode.triangles_[i] = &triangles_[i];
	}

	// we need to have the same number of polygons
	// initially all polygons are empty
	rootNode.polygons_.resize( triangles_.size() );

	nodes_.push_back( rootNode );
	uint cur = 0;
	while (cur < nodes_.size())
	{
		// attempt to split node
		Node backNode, frontNode;
		this->partitionNode( nodes_[cur], frontNode, backNode );

		if (frontNode.triangles_.size() > 0)
		{
			// add front node
			nodes_.push_back( frontNode );
			nodes_[cur].frontIndex_ = static_cast<int>( nodes_.size() - 1 );
		}

		if (backNode.triangles_.size() > 0)
		{
			// add front node
			nodes_.push_back( backNode );
			nodes_[cur].backIndex_ = static_cast<int>( nodes_.size() - 1 );
		}

		++cur;
	}

	this->validate();
}


/**
 *	This method converts result of bsp construction to runtime bsp tree
 *	structure.
 *
 *	@param pBSP	A pointer to the bsp tree to fill.
 */
void BSPTreeTool::BSPConstructor::exportAsTree( BSPTree * pBSP )
{
	// store triangle pointers into a temp structure
	BW::vector< WorldTriangleRef > triRefs;
	triRefs.resize( triangles_.size() );
	MF_ASSERT( triangles_.size() <= UINT_MAX );

	for (uint i = 0; i < triangles_.size(); ++i)
	{
		triRefs[i].triangle = &triangles_[i];
	}

	// iterate over all nodes and add which node references which triangle
	for (uint i = 0; i < nodes_.size(); i++)
	{
		Node & n = nodes_[i];
		for (uint j = 0; j < n.triangles_.size(); j++)
		{
			// we can calculate triangle index based on pointer
			const WorldTriangle* tri = n.triangles_[j];
			uint triIdx = ( uint ) ( tri - &triangles_[0] );

			triRefs[triIdx].nodes.push_back( i );
		}
	}

	// calculate number of shared triangles
	uint numSharedTriangles = 0;

	for (uint i = 0; i < triRefs.size(); ++i)
	{
		// make sure each triangle is referenced by at least one node
		MF_ASSERT( triRefs[i].nodes.size() >= 1 );
		numSharedTriangles += static_cast<uint>( triRefs[i].nodes.size() - 1 );
	}

	size_t reqMemory = nodes_.size() * sizeof( BSPNode ) +
					triangles_.size() * sizeof( WorldTriangle ) +
					numSharedTriangles * sizeof( WorldTriangle*);
	char * memBlock = new char[reqMemory];

	// sort triangles based on which node they belong
	std::sort( triRefs.begin(), triRefs.end() );
	// now we are ready to save triangles
	pBSP->numTriangles_ = static_cast<uint32>( triangles_.size() );
	pBSP->numNodes_ = static_cast<uint32>( nodes_.size() );
	pBSP->numSharedTrianglePtrs_ = numSharedTriangles;

	pBSP->pMemory_ = memBlock;
	pBSP->pNodes_ = (BSPNode*)memBlock;
	pBSP->pRoot_ = pBSP->pNodes_;

	memBlock += pBSP->numNodes_ * sizeof( BSPNode );

	if (pBSP->numTriangles_)
	{
		pBSP->pTriangles_ = (WorldTriangle*)memBlock ;
	}
	else
	{
		pBSP->pTriangles_ = NULL;
	}

	memBlock += pBSP->numTriangles_ * sizeof( WorldTriangle );

	if (pBSP->numSharedTrianglePtrs_)
	{
		pBSP->pSharedTrianglePtrs_ = (WorldTriangle**)memBlock;
	}
	else
	{
		pBSP->pSharedTrianglePtrs_ = NULL;
	}

	// fill triangles data
	uint nodeIndex = 0xFFFFFFFF;
	int numTrisPerNode = 0;

	for (uint i = 0; i < pBSP->numNodes_; ++i)
	{
		BSPNode & n = pBSP->pNodes_[i];
		n.pFront_ = NULL;
		n.pBack_ = NULL;
		n.ppSharedTriangles_ = NULL;
		n.numSharedTriangles_ = 0;
		n.pTriangles_ = NULL;
		n.numTriangles_ = 0;
	}

	WorldTriangle * pDestTri = pBSP->pTriangles_;

	// triangles are sorted by now
	// we can iterate over them and assign pTriangle and numTriangles
	// fields for each node
	// we can also calculate number of shared triangles between nodes
	for (uint i = 0; i < triRefs.size(); i++)
	{
		const WorldTriangle* pSrcTri = triRefs[i].triangle;
		memcpy( pDestTri, pSrcTri, sizeof( WorldTriangle ) );
		pDestTri++;
		// number of nodes which reference this triangle
		size_t numReferees = triRefs[i].nodes.size();
		// set up pointers to first triangle, the one we were sorting on
		MF_ASSERT(numReferees > 0);
		uint node0 = triRefs[i].nodes[0];
		MF_ASSERT(node0 < pBSP->numNodes_);
		if (node0 != nodeIndex)
		{
			pBSP->pNodes_[node0].pTriangles_ = pBSP->pTriangles_ + i;
			if (nodeIndex != 0xFFFFFFFF)
			{
				MF_ASSERT(nodeIndex < pBSP->numNodes_);
				pBSP->pNodes_[nodeIndex].numTriangles_ = numTrisPerNode;
			}
			numTrisPerNode = 0;
			nodeIndex = node0;
		}
		// calculate how many shared triangles we have for this node
		for (size_t ri = 1; ri < numReferees; ++ri)
		{
			BSPNode& n = pBSP->pNodes_[triRefs[i].nodes[ri]];
			n.numSharedTriangles_++;
		}
		numTrisPerNode++;
	}

	if (nodeIndex != 0xFFFFFFFF)
	{
		pBSP->pNodes_[nodeIndex].numTriangles_ = numTrisPerNode;
	}
	else
	{
		pBSP->pNodes_[0].numTriangles_ = 0;
		pBSP->pNodes_[0].pTriangles_ = NULL;
	}		

	WorldTriangle** ppSharedTriangles = pBSP->pSharedTrianglePtrs_;
	// fill nodes data, we can assign pointers to front and back node
	// this could be simple done by converting index to the pointer
	// we can also assign memory to shared triangles arrays
	// because now we know how much do we need for each node
	for (uint i = 0; i < nodes_.size(); i++)
	{
		BSPNode& n	= pBSP->pNodes_[i];
		// only assign frond and back pointers if it's not NULL
		if (nodes_[i].frontIndex_ != -1 )
		{
			n.pFront_	= pBSP->pNodes_ + nodes_[i].frontIndex_;
		}
		if (nodes_[i].backIndex_ != -1 )
		{
			n.pBack_	= pBSP->pNodes_ + nodes_[i].backIndex_;
		}

		n.planeEq_	= nodes_[i].splittingPlane_;
		// now when we have number of shared triangles we can assign memory
		if (n.numSharedTriangles_ > 0)
		{
			n.ppSharedTriangles_ = ppSharedTriangles;
			ppSharedTriangles += n.numSharedTriangles_;
		}
	}

	MF_ASSERT( ppSharedTriangles ==
		(pBSP->pSharedTrianglePtrs_ + pBSP->numSharedTrianglePtrs_) );

	// fill shared triangles data

	// zero num shared triangles, we gonne be incrementing it in the next loop
	for (uint i = 0; i < pBSP->numNodes_; ++i)
	{
		pBSP->pNodes_[i].numSharedTriangles_ = 0;
	}

	// assign pointers to shared triangles
	for (uint i = 0; i < triRefs.size(); ++i)
	{
		size_t numReferees = triRefs[i].nodes.size();
		for (size_t ri = 1; ri < numReferees; ++ri)
		{
			BSPNode& n = pBSP->pNodes_[triRefs[i].nodes[ri]];
			n.ppSharedTriangles_[n.numSharedTriangles_] = pBSP->pTriangles_ + i;
			n.numSharedTriangles_++;
		}
	}

	pBSP->generateBoundingBox();

	// by this point BSP should be perfectly valid
	pBSP->validate();
}


/**
 *	This method selects splitting plane for given trianle set using randomised
 *  algorithm. It tries 15 different triangles as split candidates, and chooses
 *  the one with the least number of splits.
 *	The input polygon set must be the same size as the triangle set.
 *  Each polygon corresponds to the triangle at the same index.
 *  If the polygon is non-empty, it should be a subset of the
 *	triangle and this polygon will be used for deciding how the triangle is
 *	inserted into the BSP.
 *
 *	@param triangles	input triangle set
 *	@param polygons		input polygon set
 *	@param bestBoth		output best number of triangles need to split/duplicate
 *	@param bestFront	output best number of front child triangles
 *	@param bestBack		output best number of back child triangles
 *	@param bestOn		output best number of triangles staying in the
 *						parent node
 *
 *	@return The splitting plane for the provided triangle.
 */
PlaneEq BSPTreeTool::BSPConstructor::selectSplittingPlaneRandom15( 
	const WTriangleSet & triangles, const WPolygonSet & polygons,
	uint & bestBoth, uint & bestFront, uint & bestBack, uint & bestOn )
{
	// Find the best partition.
	const uint triangleSize = static_cast<uint>( triangles.size() );
	bestFront = triangleSize;
	bestBack  = triangleSize;
	bestBoth  = triangleSize;
	bestOn    = triangleSize;

	// at this moment we should have at least 1 triangle
	MF_ASSERT(triangleSize > 0);

	// use first triangle as a start splitting plane 
	PlaneEq		splittingPlane;
	splittingPlane.init( triangles[0]->v0(),
					triangles[0]->v1(),
					triangles[0]->v2() );

	const uint MAX_TESTS = 15;

	for (uint i = 0; i < MAX_TESTS && i < triangles.size();		i++)
	{
		PlaneEq candidatePlane;
		uint randIndex = (triangles.size() < MAX_TESTS) ?
						i : rand() % triangles.size();

		MF_ASSERT( randIndex < triangles.size() );

		candidatePlane.init( triangles[randIndex]->v0(),
						triangles[randIndex]->v1(),
						triangles[randIndex]->v2() );

		uint tempFront = 0;
		uint tempBack = 0;
		uint tempBoth = 0;
		uint tempOn = 0;

		for (uint j = 0; j < triangles.size() && tempBoth < bestBoth; j++)
		{
			const WorldTriangle * pTriangle = triangles[j];
			const WorldPolygon & poly = polygons[j];

			BSPSide side =
				poly.empty() ?	whichSideOfPlane( candidatePlane, *pTriangle ) :
								whichSideOfPlane( candidatePlane, poly );

			switch (side)
			{
				case BSP_BOTH:	tempBoth++;		break;
				case BSP_FRONT:	tempFront++;	break;
				case BSP_BACK:	tempBack++;		break;
				case BSP_ON:	tempOn++;		break;
			}
		}

		// TODO: May want to change this.
		// #### Currently choose the partition with the least number of
		// duplicates.

		if (tempBoth < bestBoth)
		{
			bestBoth  = tempBoth;
			bestFront = tempFront;
			bestBack  = tempBack;
			bestOn    = tempOn;
			splittingPlane = candidatePlane;
		}
	}

	return splittingPlane;
}


/**
 *	This method partitions the set of triangles across given BSP node.
 *  It fills data for child back and front nodes if necessary.
 *
 *	@param node			Node to partition.
 *	@param frontNode	Front child node to fill.
 *	@param backNode		Back node to fill.
 */
void BSPTreeTool::BSPConstructor::partitionNode( Node& node, Node& frontNode, Node& backNode )
{
	WTriangleSet& triangles = node.triangles_;
	WPolygonSet& polygons = node.polygons_;
	IF_NOT_MF_ASSERT_DEV( triangles.size() == polygons.size() )
	{
		MF_EXIT( "BSP::partition - "
			"triangle and polygon lists different sizes" );
	}

	if (triangles.size() <= MAX_TRIANGLES_PER_NODE)
	{
		// we are done with this node
		node.splittingPlane_.init( Vector3( 0.f, 0.f, 0.f ),
			Vector3( 1.f, 0.f, 0.f ) );
		return;
	}

	const uint triangleSize = static_cast<uint>( triangles.size() );
	uint bestFront = triangleSize;
	uint bestBack  = triangleSize;
	uint bestBoth  = triangleSize;
	uint bestOn    = triangleSize;

	// Find the best partition.
	node.splittingPlane_ = this->selectSplittingPlaneRandom15(
			triangles, polygons, bestBoth, bestFront, bestBack, bestOn );

	WTriangleSet&	frontSet = frontNode.triangles_;
	WPolygonSet&	frontPolys = frontNode.polygons_;

	WTriangleSet&	backSet = backNode.triangles_;
	WPolygonSet&	backPolys = backNode.polygons_;

	frontSet.reserve(bestFront + bestBoth);
	frontPolys.reserve(bestFront + bestBoth);

	backSet.reserve(bestBack + bestBoth);
	backPolys.reserve(bestBack + bestBoth);

	WTriangleSet onNodeTriangles;
	onNodeTriangles.reserve(bestOn);

	for (uint index = 0; index < triangles.size(); index++)
	{
		const WorldTriangle * pTriangle = triangles[index];
		const WorldPolygon & poly = polygons[index];

		BSPSide side =
			poly.empty() ? whichSideOfPlane( node.splittingPlane_, *pTriangle ):
							whichSideOfPlane( node.splittingPlane_, poly );

		switch (side)
		{
		case BSP_ON:
			onNodeTriangles.push_back( pTriangle );
			break;

		case BSP_FRONT:
			frontSet.push_back( pTriangle );
			frontPolys.push_back( poly );
			break;

		case BSP_BACK:
			backSet.push_back( pTriangle );
			backPolys.push_back( poly );
			break;

		case BSP_BOTH:
			frontSet.push_back( pTriangle );
			frontPolys.push_back( WorldPolygon() );

			backSet.push_back( pTriangle );
			backPolys.push_back( WorldPolygon() );

			if (!poly.empty())
			{
				poly.split( node.splittingPlane_,
					frontPolys.back(),
					backPolys.back() );
			}
			else
			{
				WorldPolygon tempPoly(3);
				tempPoly[0] = pTriangle->v0();
				tempPoly[1] = pTriangle->v1();
				tempPoly[2] = pTriangle->v2();

				tempPoly.split( node.splittingPlane_,
					frontPolys.back(),
					backPolys.back() );
			}

			break;
		}
	}
	// update node triangles
	node.triangles_.swap( onNodeTriangles );

	if (backSet.empty() && frontSet.empty())
	{
		node.partitioned_ = false;
		node.splittingPlane_.init( Vector3( 0.f, 0.f, 0.f ),
			Vector3( 1.f, 0.f, 0.f ) );
	}
	else
	{
		node.partitioned_ = true;
	}
}


/**
 *	BSP magic tokens and supported versions
 */
const uint8 BSP_FILE_LEGACY_VERSION = 0;
// BSP version 1 was saved in 3_current without structure size verification
const uint8 BSP_FILE_CUR_VERSION = 2; 
const uint8 BSP_FILE_INVALID_VERSION = 0xFF; 
const uint32 BSP_FILE_MAGIC = 0x505342;
const uint32 BSP_FILE_LEGACY_TOKEN = BSP_FILE_MAGIC | (BSP_FILE_LEGACY_VERSION << 24);
const uint32 BSP_FILE_CUR_TOKEN = BSP_FILE_MAGIC | (BSP_FILE_CUR_VERSION << 24);

/**
 *	This method validates internal state of bsp tree and
 *  asserts if something is wrong or inconsistent.
 */
void	BSPTree::validate() const
{
#ifdef BSP_RUNTIME_VALIDATION
	MF_ASSERT(pNodes_ == pRoot_);

	uint numNodeLinks = 1;
	uint numSharedTris = 0;
	for (uint i = 0; i < numNodes_; ++i)
	{
		const BSPNode& n = pNodes_[i];
		MF_ASSERT( (n.pTriangles_ == NULL && n.numTriangles_ == 0) || (n.pTriangles_ != NULL && n.numTriangles_ > 0) );
		MF_ASSERT( (n.ppSharedTriangles_ == NULL && n.numSharedTriangles_ == 0) || (n.ppSharedTriangles_ != NULL && n.numSharedTriangles_ > 0) );
		MF_ASSERT( n.pTriangles_ == NULL || (n.pTriangles_ >= pTriangles_ && n.pTriangles_ < pTriangles_ + numTriangles_) );
		MF_ASSERT( n.ppSharedTriangles_ == NULL || (n.ppSharedTriangles_ >= pSharedTrianglePtrs_ && n.ppSharedTriangles_ < pSharedTrianglePtrs_ + numSharedTrianglePtrs_) );

		MF_ASSERT( n.pTriangles_ == NULL || (n.pTriangles_ + n.numTriangles_ <= pTriangles_ + numTriangles_) );
		MF_ASSERT( n.ppSharedTriangles_ == NULL || (n.ppSharedTriangles_ + n.numSharedTriangles_ <= pSharedTrianglePtrs_ + numSharedTrianglePtrs_) );
		if (n.pBack_)
		{
			numNodeLinks++;
			MF_ASSERT( n.pBack_ >= pNodes_ && n.pBack_ < pNodes_ + numNodes_ );

		}
		if (n.pFront_)
		{
			numNodeLinks++;
			MF_ASSERT( n.pFront_ >= pNodes_ && n.pFront_ < pNodes_ + numNodes_ );
		}
		numSharedTris += n.numSharedTriangles_;
	}
	MF_ASSERT( numNodes_ == numNodeLinks );
	MF_ASSERT( numSharedTrianglePtrs_ == numSharedTris );
#endif
}


/**  
 *	Set of functions serialise data save and load data.
 *	Pointers gonna be converted to 32bit offsets to make
 *	32bit code be able to load data generated in 64bit app and vice versa
 */
namespace
{

	/**
	 *	Convert 32 bit offset to pointer. Returns NULL pointer if given offset is OFFSET_32BIT_NULL.
	 */
	template<class T>
	T*	offsetToPtr( T* basePtr, OFFSET_32BIT offset )
	{
		T* ptr = NULL;
		if (offset != OFFSET_32BIT_NULL)
		{
			MF_ASSERT( offset >= 0 );
			ptr = basePtr + offset;
		}
		return ptr;
	}

	/**
	 *	Convert pointer to 32 bit offset.  Returns OFFSET_32BIT_NULL if given pointer is NULL
	 */
	template<class T>
	OFFSET_32BIT ptrToOffset(T* basePtr, T* ptr)
	{
		if (ptr == NULL)
		{
			return OFFSET_32BIT_NULL;
		}
		// negative offsets weren't tested and not supported yet
		MF_ASSERT( ptr - basePtr >= 0 );
		// assert on overflow in 64 bit mode
		MF_ASSERT(ptr - basePtr < MAX_OFFSET_32BIT);
		OFFSET_32BIT offset = (OFFSET_32BIT)(ptr - basePtr);
		return offset;
	}

	/**
	 *	Convert pointer to 32bit offset and write it into the output buffer
	 *  update output buffer's pointer
	 */
	template<class T>
	void	writePtrAsOffset( char*& pData, T* basePtr, T* ptr )
	{
		*(OFFSET_32BIT*)pData = ptrToOffset( basePtr, ptr );
		pData += sizeof( OFFSET_32BIT );
	}

	/**
	 *	Write block of memory to output buffer and update output buffer's pointer
	 */
	void	writeBuffer( char*& pData, const void* pSrc, size_t dataLen )
	{
		memcpy( pData, pSrc, dataLen );
		pData += dataLen;
	}
	/**
	 *	Write pod type to the output buffer and update output buffer's pointer
	 */
	template<class T>
	void	writePOD( char*& pData, const T& pod )
	{
		*(T*)pData = pod;
		pData += sizeof(pod);
	}
	/**
	 *	Read 32 bit offset, convert it to the pointer and update input buffer's pointer
	 */
	template<class T>
	void	readPtrAsOffset( T* basePtr, T*& ptr, char*& pData )
	{
		ptr = offsetToPtr( basePtr, *(OFFSET_32BIT*)pData);
		pData += sizeof( OFFSET_32BIT );
	}
	/**
	 *	Read block of memory from the input buffer and update input buffer's pointer
	 */
	void	readBuffer( void* pDst, char*& pData, size_t dataLen )
	{
		memcpy( pDst, pData, dataLen );
		pData += dataLen;
	}
	/**
	 *	Read pod type from the input buffer and update input buffer's pointer
	 */
	template<class T>
	void	readPOD( T& pod, char*& pData )
	{
		pod = *(T*)pData;
		pData += sizeof(T);
	}
}
/*
 *  New BSP format in words:
 *	Magic number as uint32 (including version number in last byte)
 *	Number of triangles as uint32
 *	Max number of triangles for a single node as uint32
 *	Number of nodes as uint32
 *	Number of shared triangles between different nodes as uint32
 *	List of triangles - (9 4-byte floats each. i.e. 3 Vector3s)
 *  List of offsets for shared triangle pointers ( offsets encoded as int32 )
 *  Nodes use this memory to store pointers to triangles which belong to more than 1 node
 *	All nodes of BSP in prefix order (front node before back node)
 *
 *	Offset of the front child of this node of as int32
 *	Offset of the back child of this node of as int32
 *	The plane equation for the node - Vector3 for normal, float for d.
 *		Plane is set of p such that: normal . p == d (where . is dot product)
 *		Normal is in direction of front.
 *	Offset of the first triangle which belongs to this node or -1 if there are no triangles
 *  Number of triangles associated with this node as int32
 *	Offset of the first shared triangle which belongs to this node or -1 if there are no shared triangles
 *  Number of shared triangles associated with this node as int32
 *	At the end of the file is user data. Each user data is a 4 byte key followed
 *	by a uint32 size prefixed blob.
 *
 *	This method saves data for inplace loading and relies
 *  on the following structure sizes
 *		PlaneEq == 16 bytes
 *		WorldTriangles == 40 bytes
 *	It also relies on BSPNode::PACKED_SIZE being 40 bytes
 *	To protect against future changes we are saving all sizes which
 *  we rely on to each bsp file. When we read file we verify saved sizes and current sizes
 *  and bail out if they do not match.
 */
BinaryPtr BSPTreeTool::saveBSPInMemory( BSPTree* pBSP )
{
	MF_ASSERT( pBSP );
	MF_ASSERT( pBSP->numNodes() >= 1 );
	pBSP->validate();

	// bsp magic and version and 3 sizes of structures we rely on encoded as uint32
	size_t headerSize = 4 * sizeof(uint32);
	// precalculate how much memory do we need to save our bsp tree
	size_t dataLen = 
		// header
		headerSize + 
		// num triangles
		// num nodes
		// num shared triangles
		3 * sizeof(uint32) +									
		// precalculated bounding box
		sizeof(BoundingBox) +
		// bsp nodes
		pBSP->numNodes_ * BSPNode::PACKED_SIZE +
		// triangles
		pBSP->numTriangles_ * sizeof(WorldTriangle) +
		// shared triangles pointers
		pBSP->numSharedTrianglePtrs_ * sizeof(OFFSET_32BIT);

	// calculate how much space we need to save user data
	size_t userDataSize = 0;
	if (pBSP->pUserData_)
	{
		for (BSPTree::UserDataMap::const_iterator dataIt = pBSP->pUserData_->begin();
			dataIt != pBSP->pUserData_->end(); ++dataIt)
		{
			int32 len = dataIt->second->len();
			userDataSize += sizeof( BSPTree::UserDataKey ) + sizeof( int32 ) + len;
		}
	}
	dataLen += userDataSize;

	const int dataLenInt = static_cast<int>( dataLen );
	BinaryPtr bp = new BinaryBlock( NULL, dataLenInt, "BSPTreeTool::saveBSP" );
	char* pData = bp->cdata();
	char* pOrigData = pData;

	// write bsp header
	writePOD( pData, BSP_FILE_CUR_TOKEN );
	writePOD( pData, (uint32)sizeof(PlaneEq) );
	writePOD( pData, (uint32)sizeof(WorldTriangle) );
	writePOD( pData, (uint32)BSPNode::PACKED_SIZE );

	writePOD( pData, pBSP->numTriangles_ );
	writePOD( pData, pBSP->numNodes_ );
	writePOD( pData, pBSP->numSharedTrianglePtrs_ );

	// generate bounding box and write it
	pBSP->generateBoundingBox();
	writePOD( pData, pBSP->boundingBox_ );
	
	// write triangles 
	writeBuffer( pData, pBSP->pTriangles_, sizeof( WorldTriangle ) * pBSP->numTriangles_ );

	MF_ASSERT( pData + sizeof(OFFSET_32BIT) * pBSP->numSharedTrianglePtrs_ <= pOrigData + dataLen );
	// write shared triangles
	for (uint i = 0; i < pBSP->numSharedTrianglePtrs_; i++)
	{
		writePtrAsOffset( pData, pBSP->pTriangles_, pBSP->pSharedTrianglePtrs_[i] );
	}

	// write bsp nodes
	MF_ASSERT( pData + BSPNode::PACKED_SIZE * pBSP->numNodes_ <= pOrigData + dataLen );
	for (uint i = 0; i < pBSP->numNodes_; i++)
	{
		BSPNode& n = pBSP->pNodes_[i];
		// memory is already reserved, all we need to do is just fill it
		writePtrAsOffset( pData, pBSP->pNodes_, n.pFront_ );
		writePtrAsOffset( pData, pBSP->pNodes_, n.pBack_ );
		writePOD( pData, n.planeEq_ );
		writePtrAsOffset( pData, pBSP->pTriangles_, n.pTriangles_ );
		writePOD( pData, n.numTriangles_ );
		writePtrAsOffset( pData, pBSP->pSharedTrianglePtrs_, n.ppSharedTriangles_ );
		writePOD( pData, n.numSharedTriangles_ );
	}

	// save user data
	if (pBSP->pUserData_)
	{
		// save user data
		for (BSPTree::UserDataMap::const_iterator dataIt = pBSP->pUserData_->begin();
			dataIt != pBSP->pUserData_->end(); ++dataIt)
		{
			int32 len = dataIt->second->len();
			writePOD( pData, dataIt->first );
			writePOD( pData, len );
			writeBuffer( pData, dataIt->second->cdata(), len );
		}
	}
	MF_ASSERT( pOrigData + dataLen == pData );
	return bp;
}


bool BSPTreeTool::saveBSPInFile( BSPTree* pBSP, const char* fileName )
{
	BinaryPtr bp = BSPTreeTool::saveBSPInMemory( pBSP );
	bool bSaveRes = false;
	if (bp)
	{
		FILE * pFile = BWResource::instance().fileSystem()->posixFileOpen( fileName, "wb" );
		if (pFile)
		{
			bSaveRes = (1 == fwrite( bp->cdata(), bp->len(), 1, pFile ));
			bSaveRes &= (0 == fclose( pFile ));
		}
	}

	if (!bSaveRes)
	{
		ERROR_MSG("BSPTreeTool::saveBSPInFile failed to save a bsp file %s\n", fileName );
	}
	return bSaveRes;
}

/**
 *	This method gets the current version of supported bsp.
 */
uint8 BSPTreeTool::getCurrentBSPVersion()
{
	return BSP_FILE_CUR_VERSION;
}

/**
 *	This method gets the version of the bsp in the given binary block.
 */
uint8 BSPTreeTool::getBSPVersion( const BinaryPtr& bp )
{
	BW_GUARD;

	MF_ASSERT( (size_t)bp->len() >= sizeof(uint32) );
	uint encodedMagic = *(int32*)bp->cdata();

	switch (encodedMagic)
	{
	case BSP_FILE_CUR_TOKEN:
		return BSP_FILE_CUR_VERSION;
	case BSP_FILE_LEGACY_TOKEN:
		return BSP_FILE_LEGACY_VERSION;
	default:
		if ((encodedMagic & 0xffffff00) == BSP_FILE_MAGIC)
		{
			return encodedMagic & 0xffffff;
		}
		else
		{
			return BSP_FILE_INVALID_VERSION;
		}
	}
}

/**
 *	This method loads bsp tree from the given binary block.
 */
BSPTree* BSPTreeTool::loadBSP( const BinaryPtr& bp )
{
	BW_GUARD;

	uint8 bspVersion = getBSPVersion( bp );

	BSPTree* pBSP = NULL;
	switch (bspVersion)
	{
		case BSP_FILE_CUR_VERSION:
			pBSP = BSPTreeTool::loadBSPCurrent( bp );
			break;
		case BSP_FILE_LEGACY_VERSION:
			pBSP = BSPTreeTool::loadBSPLegacy( bp );
			break;
		case BSP_FILE_INVALID_VERSION:
			ERROR_MSG( "BSPTree::load: Bad bsp magic\n" );
			break;
		default:
			ERROR_MSG( "BSPTree::load: "
				"Unsupported bsp version. Expected %d or %d. Got %u.\n",
				BSP_FILE_LEGACY_VERSION, BSP_FILE_CUR_VERSION, bspVersion );
	}
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("BSPTree/BSP", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pBSP->usedMemory(), 0 );
#endif
	return pBSP;
}

/**
 *	This method loads current version of BSPTree from the given binary block.
 *	It uses inplace loading and relies on the following structure sizes:
 *		PlaneEq == 16 bytes
 *		WorldTriangles == 40 bytes
 *	It also relies on BSPNode::packedSize being 40 bytes
 *	To protect against possible future changes we are saving all sizes which
 *  we rely on to each bsp file. When we read file we verify saved sizes and current sizes
 *  and bail out if they do not match.
 */
BSPTree*	BSPTreeTool::loadBSPCurrent( const BinaryPtr& bp )
{
	char* pData = bp->cdata();
	uint dataLen = bp->len();
	char* pOrigData = pData;

	// read header
	// skip magic
	uint32 bspMagic = 0;
	readPOD( bspMagic, pData );

	uint32 planeSize = 0;
	uint32 triangleSize = 0;
	uint32 bspNodePackedSize = 0;

	readPOD( planeSize, pData );
	readPOD( triangleSize, pData );
	readPOD( bspNodePackedSize, pData );

	MF_ASSERT( planeSize == sizeof(PlaneEq) );
	MF_ASSERT( triangleSize == sizeof(WorldTriangle) );
	MF_ASSERT( bspNodePackedSize == BSPNode::PACKED_SIZE );

	if (planeSize != sizeof(PlaneEq) || triangleSize != sizeof(WorldTriangle) ||
		bspNodePackedSize != BSPNode::PACKED_SIZE)
	{
		ERROR_MSG("Failed to load bsp due to base structure sizes mismatch");
		return NULL;
	}
	BSPTree* pBSP = new BSPTree;
	// read number of triangles and number of nodes
	readPOD( pBSP->numTriangles_, pData );
	readPOD( pBSP->numNodes_, pData );
	readPOD( pBSP->numSharedTrianglePtrs_, pData );

	// read precalculated bounding box
	readPOD( pBSP->boundingBox_, pData );

	// allocate memory for nodes, triangles and pointers to shared triangles
	size_t sizeRuntimeMemBlock = pBSP->numNodes_ * sizeof(BSPNode) + 
						pBSP->numTriangles_ * sizeof( WorldTriangle ) +
						pBSP->numSharedTrianglePtrs_ * sizeof( WorldTriangle* );


	char* pRuntimeMemBlock = new char[sizeRuntimeMemBlock];
	pBSP->pMemory_ = pRuntimeMemBlock;

	// read triangles
	if (pBSP->numTriangles_ > 0)
	{
		pBSP->pTriangles_ = (WorldTriangle*)pRuntimeMemBlock ;
		pRuntimeMemBlock += pBSP->numTriangles_ * sizeof( WorldTriangle );
		// triangles are pod and not 32bit/64bit dependent, we can use memcpy to read them
		readBuffer( pBSP->pTriangles_, pData, pBSP->numTriangles_ * sizeof( WorldTriangle ) );
	}
	else
	{
		pBSP->pTriangles_ = NULL;
	}
	// read shared triangles
	if (pBSP->numSharedTrianglePtrs_ > 0)
	{
		pBSP->pSharedTrianglePtrs_ = (WorldTriangle**)pRuntimeMemBlock;
		pRuntimeMemBlock += pBSP->numSharedTrianglePtrs_ * sizeof( WorldTriangle* );
		// pSharedTrianglePtrs_ is an array of pointers to triangles which are shared between multiple nodes
		// pointers are encoded as int32 offsets, which means we have to convert them back to pointers
		MF_ASSERT( pData + sizeof(OFFSET_32BIT) * pBSP->numSharedTrianglePtrs_ <= pOrigData + dataLen );
		for (uint i = 0; i < pBSP->numSharedTrianglePtrs_; ++i)
		{
			readPtrAsOffset( pBSP->pTriangles_, pBSP->pSharedTrianglePtrs_[i], pData );
		}
	}
	else
	{
		pBSP->pSharedTrianglePtrs_ = NULL;
	}
	// read bsp nodes
	MF_ASSERT( pBSP->numNodes_ > 0 );
	pBSP->pNodes_ = (BSPNode*)pRuntimeMemBlock;
	pBSP->pRoot_ = pBSP->pNodes_;
	pRuntimeMemBlock += pBSP->numNodes_ * sizeof( BSPNode );
	MF_ASSERT( pRuntimeMemBlock == pBSP->pMemory_ + sizeRuntimeMemBlock );

	MF_ASSERT( pData + BSPNode::PACKED_SIZE * pBSP->numNodes_ <= pOrigData + dataLen );

	// nodes are non pod and contains pointers which makes structure size different for 32 and 64 bit
	// need to fill'em manually
	for (uint i = 0; i < pBSP->numNodes_; i++)
	{
		BSPNode& n = pBSP->pNodes_[i];
		readPtrAsOffset( pBSP->pNodes_, n.pFront_, pData );
		readPtrAsOffset( pBSP->pNodes_, n.pBack_, pData);
		readPOD( n.planeEq_, pData );
		readPtrAsOffset( pBSP->pTriangles_, n.pTriangles_, pData );
		readPOD( n.numTriangles_, pData );
		readPtrAsOffset( pBSP->pSharedTrianglePtrs_, n.ppSharedTriangles_, pData );
		readPOD( n.numSharedTriangles_, pData );
	}

	// read user data
	size_t userDataLength = dataLen - (pData - pOrigData);
	MF_ASSERT( userDataLength <= UINT_MAX );
	BSPTreeTool::loadUserData( pBSP, pData, ( uint ) userDataLength );

	pBSP->validate();

	return pBSP;
}

/**
 *	This method loads legacy BSPTree from the given binary block
 */
BSPTree*	BSPTreeTool::loadBSPLegacy( const BinaryPtr& bp)
{
	// backwards compatibility
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( sizeof( WorldTriangle ) == 40 )
	{
		MF_EXIT( "sizeof( WorldTriangle ) must be 40!" );
	}


	const uint8 BSP_HAS_FRONT		= 0x02;
	const uint8 BSP_HAS_BACK		= 0x04;

	char* pData = bp->cdata();
	uint dataLen = bp->len();
	char* pOrigData = pData;

	// skip magic
	pData += sizeof(int32);
	// read header
	uint32 numTriangles = *(int32*)pData;
	pData += sizeof(uint32);
	uint32 numNodes = *(int32*)pData;
	pData += sizeof(uint32);
	// skip max triangles
	pData += sizeof(int32);

	// read triangles
	RealWTriangleSet triangles;
	triangles.resize( numTriangles );
	if (numTriangles > 0)
	{
		memcpy(&triangles[0], pData, sizeof( WorldTriangle ) * numTriangles );
	}
	pData += sizeof( WorldTriangle ) * numTriangles;

	BW::vector<BSPConstructor::Node>	nodes;
	nodes.resize( numNodes );

	// read nodes data
	for (uint i = 0; i < numNodes; ++i)
	{
		BSPConstructor::Node& node = nodes[i];
		char* nodeInFile = pData;
		uint8 flags = *(uint8*)pData;
		pData += sizeof(uint8);
		// skip plane eq
		node.splittingPlane_ = *(PlaneEq*)pData;
		pData += sizeof(PlaneEq);
		node.frontIndex_ = -1;
		node.backIndex_ = -1;
		if (flags & BSP_HAS_FRONT)
		{
			node.frontIndex_ = i + 1;
			if (flags & BSP_HAS_BACK)
			{
				int depth = 1;
				int numSkippedNodes = 0;
				char* nodemem = nodeInFile;
				while (depth > 0)
				{
					// skip node
					nodemem += sizeof(uint8) + sizeof(PlaneEq);
					uint16 numTris = *(uint16*)nodemem;
					nodemem += sizeof(uint16) + numTris * sizeof(uint16);
					// check next node flags
					uint8 nflags = *(uint8*)nodemem;
					if (nflags & BSP_HAS_FRONT)
					{
						depth++;
					}
					if (nflags & BSP_HAS_BACK)
					{
						depth++;
					}
					depth--;
					numSkippedNodes++;

				}
				node.backIndex_ = i + numSkippedNodes + 1;
			}
		}
		else if (flags & BSP_HAS_BACK)
		{
			node.backIndex_ = i + 1;

		}
		uint16 numRefTris = *(uint16*)pData;
		pData += sizeof(uint16);
		node.triangles_.resize( numRefTris );
		for (uint ni = 0; ni < numRefTris; ++ni)
		{
			uint16 triIndex = *(uint16*)pData;
			MF_ASSERT(triIndex < numTriangles);
			node.triangles_[ni] = &triangles[triIndex];
			pData += sizeof(uint16);
		}
	}

	// initialise bsp constructor which we gonna use to generate bsp tree
	BSPConstructor constructor( triangles , nodes );

	BSPTree* pBSP = new BSPTree();
	constructor.exportAsTree( pBSP );
	// load user data, it's in the same format so we can reuse existing loading function
	size_t userDataLen = dataLen - (pData - pOrigData);
	MF_ASSERT( userDataLen <= UINT_MAX );
	BSPTreeTool::loadUserData( pBSP, pData, (uint ) userDataLen );
	pBSP->validate();

	return pBSP;
}

/**
 *	Load user data from the provided memory block to the bsp tree
 *	@param BSPTree*		BSP tree we want to load user data into
 *	@param char*		data block to load user data from
 *	@param dataLen*		size of data block to load data from
 */
void	BSPTreeTool::loadUserData( BSPTree* pBSP, char* pData, uint dataLen )
{
	char* pOrigData = pData;
	while (pData < pOrigData + dataLen)
	{
		BSPTree::UserDataKey type;
		readPOD( type, pData );
		int32 len;
		readPOD( len, pData );
		BinaryPtr userData = new BinaryBlock( NULL, len, "BinaryBlock/BSPTree");
		readBuffer( userData->cdata(), pData, len );
		pBSP->setUserData( type, userData );
		MF_ASSERT(pData <= pOrigData + dataLen);
	}
	MF_ASSERT(pData == pOrigData + dataLen);
}

/**
 *	This method build bsp tree from the provided triangle array
 *	@param triangles		Array of triangles to build bsp tree from.
 *	@return					Generate bsp tree.
 */
BSPTree*	BSPTreeTool::buildBSP( RealWTriangleSet & triangles )
{
	BSPConstructor constructor( triangles );

	BSPTree* pNewTree = new BSPTree();
	constructor.exportAsTree( pNewTree );
	pNewTree->validate();
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("BSPTree/BSP", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pNewTree->usedMemory(), 0 );
#endif
	return pNewTree;
}

/**
 *	Default constructor and destructor
 *
 */
BSPTree::BSPTree():
pNodes_(NULL),
numNodes_(0),
pRoot_(NULL),
pTriangles_(NULL),
numTriangles_(0),
pSharedTrianglePtrs_(NULL),
numSharedTrianglePtrs_(0),
pMemory_(NULL),
pUserData_(NULL)
{
}

BSPTree::~BSPTree()
{
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("BSPTree/BSP", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		usedMemory(), 0 );
#endif
	if (pMemory_)
	{
		bw_safe_delete_array( pMemory_ );
	}

	if (pUserData_)
	{
		bw_safe_delete( pUserData_ );
	}
}

/**
 *	This method returns the amount of memory used by bsp tree
 *
 */
size_t	BSPTree::usedMemory() const
{
	return sizeof(BSPTree) + 
			numNodes_ * sizeof(BSPNode) +
			numTriangles_* sizeof(WorldTriangle) +
			numSharedTrianglePtrs_ * sizeof(WorldTriangle*);
}

/**
 *	Get user data from the bsp tree
 *
 */
BinaryPtr	BSPTree::getUserData(UserDataKey type)
{
	if (pUserData_)
	{
		UserDataMap::iterator it = pUserData_->find(type);
		if (it != pUserData_->end())
		{
			return (*it).second;
		}
	}
	return NULL;
}

/**
 *	Add user data to the bsp tree
 *
 */
void		BSPTree::setUserData(UserDataKey type, const BinaryPtr& pData)
{
	if (!pUserData_)
	{
		pUserData_ = new UserDataMap;
	}
	pUserData_->insert( std::make_pair( type, pData ) );
}


/**
 *	Used to find out if one or more triangles in the BSP are collideable
 *
 *  @return false if no triangle is collideable, true otherwise
 */
bool BSPTree::canCollide() const
{
	for (uint i = 0; i < numTriangles_; ++i)
	{
		if ( pTriangles_[i].collisionFlags() != TRIANGLE_NOT_IN_BSP )
		{
			return true;
		}
	}
	return false;
}

/**
 *	This method is necessary for all bsps exported using BigWorld 1.8,
 *	which will have a "bsp2" section in the primitves file.  The flags member
 *	of such primtives file is the material group ID and not the actual flags.
 *	This method converts material group IDs to WorldTriangle flags.
 *
 *	Note - it is entirely reasonable to have IDs in the BSP data that do not
 *	have entries in the flagsMap.  This occurs when customBSPs are used.  In this
 *	case, the customBSP was not given a material mapping, and so the default
 *	is used - the object's material kind (passed in via the defaultFlags param)
 */
void BSPTree::remapFlags( BSPFlagsMap& flagsMap )
{
	for (uint i = 0; i < numTriangles_; ++i)
	{
		WorldTriangle& tri = pTriangles_[i];
		uint32 idx = (uint32)tri.flags();
		tri.flags( flagsMap[idx % flagsMap.size()] );
	}
}
/**
 *	Generates a bounding box for this BSP.
 */
void BSPTree::generateBoundingBox()
{
	boundingBox_ = BoundingBox::s_insideOut_;

	for (uint i = 0; i < numTriangles_ ; ++i)
	{
		WorldTriangle& tri = pTriangles_[i];
		boundingBox_.addBounds( tri.v0() );
		boundingBox_.addBounds( tri.v1() );
		boundingBox_.addBounds( tri.v2() );
	}
}

/**
 *	This method returns whether the input triangle intersects any triangle in
 *	the entire bsp tree
 *
 *	@param triangle			The triangle to test for collision with the BSP.
 *	@param ppHitTriangle	If not NULL and a collision occurs, the pointer
 *							pointed to by this variable is set to point to the
 *							triangle that was collided with. If the triangle
 *							that is hit first is important, you should pass in
 *							the "starting" vertex as the first vertex of the
 *							test triangle.
 *
 *	@return True if an intersection occurred, otherwise false.
 */
bool BSPTree::intersects( const WorldTriangle & triangle,
						const WorldTriangle ** ppHitTriangle ) const
{
	return pRoot_->intersects( triangle, ppHitTriangle );
}


// This structure is used to get rid of recursion in intersection check
class BSPStackNode
{
public:
	BSPStackNode() {}
	BSPStackNode( const BSPNode * pBSP, int eBack, float sDist, float eDist ) :
		pNode_( pBSP ), eBack_( eBack ), sDist_( sDist ), eDist_( eDist )
	{ }

	const BSPNode	* pNode_;
	int			eBack_;		// or -1 for unseen
	float		sDist_;
	float		eDist_;
};

/**
 *	This method returns whether the input interval intersects any triangle in
 *	the entire bsp tree.  If it does, dist is set to the fraction of the
 *	distance along the vector that the intersection point lies.
 *
 *	Note: In general, you will want to pass in a float that is set to 1 for the
 *	dist parameter.
 *
 *	@param start	The start point of the interval to test for collision.
 *	@param end		The end point of the interval to test for collision.
 *	@param dist		A reference to a float. Usually this should be set to 1
 *					before the method is called
 *
 *					The input value of this float is the fraction of the
 *					interval that should be considered. For example, if dist is
 *					0.5, only the interval from <i>start</i> to the midpoint of
 *					<i>start</i> and <i>end</i> is considered for collision.
 *
 *					After the call, if a collision occurred, this value is set
 *					to the fraction along the interval that the collision
 *					occurred. Thus, the collision point can be calculated as
 *					start + dist * (end - start).
 *	@param ppHitTriangle
 *					A pointer to a world triangle pointer. If not NULL, the
 *					pointer pointed to by this parameter is set to the pointer
 *					to the triangle that caused the collision.
 *	@param pVisitor
 *					A pointer to a visitor object. If not NULL, this object's
 *					'visit' method is called for each triangle hit.
 *
 *	@return			True if there was a collision, otherwise false.
 */
bool BSPTree::intersects( const Vector3 & start,
						const Vector3 & end,
						float & dist,
						const WorldTriangle ** ppHitTriangle,
						CollisionVisitor * pVisitor ) const
{
	const WorldTriangle * pHitTriangle = NULL;

	if (ppHitTriangle == NULL) ppHitTriangle = &pHitTriangle;
	if (pVisitor == NULL) pVisitor = &s_nullCV;

	float origDist = dist;
	const WorldTriangle * origHT = *ppHitTriangle;

	Vector3 delta = end - start;
	float tolerancePct = BSPNODE_TOLERANCE / delta.length();

	DynamicEmbeddedArray<BSPStackNode, 32> stack;
	stack.push_back( BSPStackNode( pRoot_, -1, 0, 1 ) );

	while (!stack.empty())
	{
		// get the next node to look at
		BSPStackNode cur = stack.back();
		stack.pop_back();

		// set default / initial values for the line segment range
		float sDist = cur.sDist_;
		float eDist = cur.eDist_;

		// set up for plane intersection
		float iDist = 0.f;
		const PlaneEq & pe = cur.pNode_->planeEq_;

		// variables saying is points are on back side of plane
		int sBack, eBack;

		// see if this is a really simple node
		if (cur.pNode_->pFront_ == NULL && cur.pNode_->pBack_ == NULL)
		{
			// just look at the triangles and don't try to add to the stack
			sBack = -2;
			eBack = -2;
		}
		// see if this is the first time we've seen it
		else if (cur.eBack_ == -1)
		{
			// ok, this is the first time at this node
			float sOut = pe.distanceTo( start + delta * (sDist - tolerancePct) );
			float eOut = pe.distanceTo( start + delta * (eDist + tolerancePct) );
			sBack = int(sOut < 0.f);
			eBack = int(eOut < 0.f);

			// find which side the start is on
			BSPNode * pStartSide = (&cur.pNode_->pFront_)[sBack];

			// are they both on the same side?
			if (sBack == eBack)
			{
				// ok, easy - go down that side then, if it's there

				// but first check if either are within tolerance
				if (fabs(sOut) < BSPNODE_TOLERANCE || fabs(eOut) < BSPNODE_TOLERANCE)
				{
					// We need to check both the front and back side, because some
					// triangles might slightly cross the partition plane and considered as
					// belongs to front side or back side.
					// See whichSideOfPlane() implementation
					stack.push_back( BSPStackNode(
						cur.pNode_, !sBack, sDist, eDist ) );
				}

				// now go down the start side
				if (pStartSide != NULL)
				{
					stack.push_back( BSPStackNode(
						pStartSide, -1, sDist, eDist ) );
				}

				continue;
			}

			// ok, points are on different sides. find intersect distance
			iDist = pe.intersectRayHalf( start, pe.normal().dotProduct( delta ) );

			// if there's anything on the start side we'll have to do it first
			if (pStartSide != NULL)
			{
				// remember to come to the end side
				stack.push_back( BSPStackNode(
					cur.pNode_, eBack, sDist, eDist ) );

				// and then look at start side
				stack.push_back( BSPStackNode(
					pStartSide, -1, sDist, iDist ) );

				continue;
			}

			// ok, there's nothing on the start side, so fall through to
			//  check the triangles on the plane (against the line between
			//  between sDist and eDist), and later add the end side
			//  between iDist and eDist, if it's not null (phew!)
		}
		// ok we've been here before
		else
		{
			sBack = cur.eBack_;
			eBack = cur.eBack_;

			iDist = pe.intersectRayHalf( start, pe.normal().dotProduct( delta ) );

			// fall through to check the triangles on the plane (against the
			//  line between sDist and eDist), and later add the end
			//  side between iDist and eDist, if it's not null
		}

		// ok, check the triangles on this node, and get out immediately
		//  if we get a (confirmed) hit
		if (cur.pNode_->intersectsThisNode( start, end, dist, ppHitTriangle, pVisitor ) &&
				dist <= (cur.eDist_+tolerancePct) )
		{
			return true;
		}

		// reset stuff (hmmm)
		dist = origDist;
		*ppHitTriangle = origHT;

		// now add the other side if it's not null (and eBack is ok)
		if (eBack >= 0)
		{
			BSPNode * pEndSide = (&cur.pNode_->pFront_)[eBack];
			if (pEndSide != NULL) stack.push_back( BSPStackNode(
				pEndSide, -1, iDist, eDist ) );
		}
	}

	return false;
	
}


/**
 *	This method returns whether the volume formed by moving the input
 *	triangle by a given translation intersects any triangle in the entire
 *	bsp tree.
 */
bool BSPTree::intersects( const WorldTriangle & triangle,
						const Vector3 & translation,
						CollisionVisitor * pVisitor ) const
{
	return pRoot_->intersects( triangle, translation, pVisitor );
}

BW_END_NAMESPACE

// bsp.cpp
