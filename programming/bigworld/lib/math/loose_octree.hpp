#ifndef LOOSE_OCTREE_HPP
#define LOOSE_OCTREE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/static_array.hpp"

#include "boundbox.hpp"
#include "sphere.hpp"
#include "vector3.hpp"

// Import logging macros incase we enable logging
#include "loose_octree_logging.hpp"


BW_BEGIN_NAMESPACE

class AABB;
class ConvexHull;

class LooseOctree_DataTypes
{
public:
	// Definition of NodeIndex is currently uint16, allowing it to support
	// 2^15 nodes (as one bit is used internally to flag leaves).
	// While this is only enough nodes to support a full tree of depth 4,
	// in practice, these trees are quite sparse, and a large number of the
	// potential nodes never exist. This means often trees of depth 5 and 
	// higher are fine. An assertion inside ::newNode will trigger if the
	// number of nodes exceeds this limitation.
	typedef uint16 NodeIndex;
	
	typedef uint32 NodeDataReference;
	typedef BW::vector< NodeDataReference > RuntimeNodeDataReferences;

	static const NodeIndex INVALID_NODE = NodeIndex(~0);
	static const NodeIndex ROOT_NODE = NodeIndex(0);
	static const NodeDataReference INVALID_DATAREFERENCE = NodeDataReference(~0);
	static const NodeIndex LEAF_REF_FLAG = NodeIndex(~0) ^ (NodeIndex(~0) >> 1);
	static const NodeIndex INDEX_MASK = NodeIndex(~LEAF_REF_FLAG);

	static const NodeIndex LEFT_CHILD_SIDE = 0;
	static const NodeIndex RIGHT_CHILD_SIDE = 1 << 0;
	static const NodeIndex FRONT_CHILD_SIDE = 0;
	static const NodeIndex BACK_CHILD_SIDE = 1 << 1;
	static const NodeIndex BOTTOM_CHILD_SIDE = 0; 
	static const NodeIndex TOP_CHILD_SIDE = 1 << 2;
	
	static const NodeIndex NUM_CHILDREN = 8;

	struct Node
	{
		Node();

		NodeIndex child[NUM_CHILDREN];
	};

	typedef AABB NodeBound;

	struct NodeBoundData
	{
		NodeBound box;
	};

	typedef Vector3 NodeCenter;

	struct Configuration
	{
		Configuration();

		uint32 maxDepth_;
		NodeCenter worldCenter_;
		float worldSize_;
		NodeIndex numDataEntries_;
	};
};


class LooseOctree_DynamicTypes : 
	public LooseOctree_DataTypes
{
public:
	typedef BW::vector<Node> Nodes;
	typedef BW::vector<NodeBoundData> NodeBounds;
	typedef BW::vector<NodeCenter> NodeCenters;
	typedef BW::vector<NodeDataReference> NodeDataReferences;
	typedef BW::vector<NodeIndex> NodeParents;

	static const bool s_supportsInsertion = true;
};


class LooseOctree_StaticTypes : 
	public LooseOctree_DataTypes
{
public:
	typedef BW::ExternalArray<Node> Nodes;
	typedef BW::ExternalArray<NodeBoundData> NodeBounds;
	typedef BW::ExternalArray<NodeCenter> NodeCenters;
	typedef BW::ExternalArray<NodeDataReference> NodeDataReferences;
	typedef BW::ExternalArray<NodeIndex> NodeParents;

	static const bool s_supportsInsertion = false;
};


template <class StorageTypes = LooseOctree_DynamicTypes >
struct LooseOctreeStorage
{
	typedef typename StorageTypes::Configuration Configuration;
	typedef typename StorageTypes::NodeIndex NodeIndex;
	typedef typename StorageTypes::NodeDataReference NodeDataReference;
	typedef typename StorageTypes::NodeDataReferences NodeDataReferences;
	typedef typename StorageTypes::Node Node;
	typedef typename StorageTypes::NodeBound NodeBound;
	typedef typename StorageTypes::NodeBoundData NodeBoundData;
	typedef typename StorageTypes::NodeCenter NodeCenter;
	typedef typename StorageTypes::NodeBounds NodeBounds;
	typedef typename StorageTypes::NodeCenters NodeCenters;
	typedef typename StorageTypes::Nodes Nodes;
	typedef typename StorageTypes::RuntimeNodeDataReferences RuntimeNodeDataReferences;
	typedef typename StorageTypes::NodeParents NodeParents;
	static const bool s_supportsInsertion = StorageTypes::s_supportsInsertion;

	static const NodeIndex INVALID_NODE = StorageTypes::INVALID_NODE;
	static const NodeIndex ROOT_NODE = StorageTypes::ROOT_NODE;
	static const NodeDataReference INVALID_DATAREFERENCE = 
		StorageTypes::INVALID_DATAREFERENCE;
	static const NodeIndex LEAF_REF_FLAG = StorageTypes::LEAF_REF_FLAG;
	static const NodeIndex INDEX_MASK = StorageTypes::INDEX_MASK;

	static const NodeIndex LEFT_CHILD_SIDE = StorageTypes::LEFT_CHILD_SIDE;
	static const NodeIndex RIGHT_CHILD_SIDE = StorageTypes::RIGHT_CHILD_SIDE;
	static const NodeIndex FRONT_CHILD_SIDE = StorageTypes::FRONT_CHILD_SIDE;
	static const NodeIndex BACK_CHILD_SIDE = StorageTypes::BACK_CHILD_SIDE;
	static const NodeIndex TOP_CHILD_SIDE = StorageTypes::TOP_CHILD_SIDE;
	static const NodeIndex BOTTOM_CHILD_SIDE = StorageTypes::BOTTOM_CHILD_SIDE;
	
	static const NodeIndex NUM_CHILDREN = StorageTypes::NUM_CHILDREN;

	Configuration config_;
	
	NodeBounds bounds_;
	NodeCenters centers_;
	Nodes nodes_;
	NodeDataReferences dataReferences_;
	NodeParents parents_;

	NodeCenter& centerOfNode( NodeIndex index );
	const NodeCenter& centerOfNode( NodeIndex index ) const;

	NodeBound& boundOfNode( NodeIndex index );
	const NodeBound& boundOfNode( NodeIndex index ) const;

	Node& childrenOfNode( NodeIndex index );
	const Node& childrenOfNode( NodeIndex index ) const;

	NodeIndex& parentOfNode( NodeIndex index );
	const NodeIndex& parentOfNode( NodeIndex index ) const;

	NodeDataReference& dataReferenceOfNode( NodeIndex index );
	const NodeDataReference& dataReferenceOfNode( NodeIndex index ) const;

	bool isLeaf( NodeIndex index ) const;
	static bool isLeafRef( NodeIndex childReference );
	static NodeIndex childIndexFromReference( NodeIndex childReference );
	
	NodeIndex newNode( NodeIndex parent, const NodeCenter& center, bool isLeaf );
	NodeIndex assignNewData();

	void translateDataReferences( RuntimeNodeDataReferences& references ) const;
};

typedef LooseOctreeStorage< LooseOctree_StaticTypes > LooseOctree_StaticData;
typedef LooseOctreeStorage< LooseOctree_DynamicTypes > LooseOctree_DynamicData;

/*
 * This class implements a loose fitting octree
 */
template < class TStorageType = LooseOctree_DynamicData >
class LooseOctree
{
public:
	typedef TStorageType StorageType;
	typedef typename StorageType::NodeIndex NodeIndex;
	typedef typename StorageType::NodeDataReference NodeDataReference;
	typedef typename StorageType::NodeDataReferences NodeDataReferences;
	typedef typename StorageType::Node Node;
	typedef typename StorageType::NodeBound NodeBound;
	typedef typename StorageType::NodeBoundData NodeBoundData;
	typedef typename StorageType::NodeCenter NodeCenter;
	typedef typename StorageType::NodeBounds NodeBounds;
	typedef typename StorageType::NodeCenters NodeCenters;
	typedef typename StorageType::NodeParents NodeParents;
	typedef typename StorageType::Nodes Nodes;
	typedef typename StorageType::RuntimeNodeDataReferences 
		RuntimeNodeDataReferences;

	static const NodeIndex INVALID_NODE = StorageType::INVALID_NODE;
	static const NodeIndex ROOT_NODE = StorageType::ROOT_NODE;
	static const NodeDataReference INVALID_DATAREFERENCE = 
		StorageType::INVALID_DATAREFERENCE;
	static const NodeIndex LEAF_REF_FLAG = StorageType::LEAF_REF_FLAG;
	static const NodeIndex INDEX_MASK = StorageType::INDEX_MASK;

	static const NodeIndex LEFT_CHILD_SIDE = StorageType::LEFT_CHILD_SIDE;
	static const NodeIndex RIGHT_CHILD_SIDE = StorageType::RIGHT_CHILD_SIDE;
	static const NodeIndex FRONT_CHILD_SIDE = StorageType::FRONT_CHILD_SIDE;
	static const NodeIndex BACK_CHILD_SIDE = StorageType::BACK_CHILD_SIDE;
	static const NodeIndex TOP_CHILD_SIDE = StorageType::TOP_CHILD_SIDE;
	static const NodeIndex BOTTOM_CHILD_SIDE = StorageType::BOTTOM_CHILD_SIDE;

	static const NodeIndex NUM_CHILDREN = StorageType::NUM_CHILDREN;

	LooseOctree();
	LooseOctree( const Vector3 & center, float worldSize, uint32 maxDepth );
	
	void initialise( const Vector3 & center, float worldSize, uint32 maxDepth );
	void reinitialise();
	void reset();

	StorageType& storage();
	const StorageType& storage() const;

	NodeIndex insert( const Vector3 & objCenter, float objRadius );
	NodeIndex insert( const AABB& bb );
	NodeIndex findData( const Vector3 & objCenter ) const;

	NodeDataReference dataOnLeaf( NodeIndex nodeID ) const;

	void updateNodeBound( NodeIndex nodeID, const AABB& bb );
	void updateNodeBound( NodeIndex nodeID );
	void updateHeirarchy();
	bool empty() const;

	void intersect( const ConvexHull & hull, RuntimeNodeDataReferences & dataReferences ) 
		const;
	void intersect( const Vector3 & start, const Vector3 & stop,
		RuntimeNodeDataReferences & dataReferences ) const;
	void intersect( const AABB & bb, 
		RuntimeNodeDataReferences & dataReferences ) const;
	
	template <class IntersectShape>
	void intersect( IntersectShape& shape, RuntimeNodeDataReferences & dataReferences ) 
		const;

	template <class Visitor>
	void traverse( Visitor& visitor, NodeIndex nodeIndex = ROOT_NODE, 
		size_t level = 0u );

	const AABB& rootBoundingBox() const;

private:
	
	bool isLeaf( NodeIndex index ) const;
	uint32 numChildren( NodeIndex index ) const;

	void mergeChildBounds( NodeIndex index, AABB& bb );
	NodeBound updateHeirarchy( NodeIndex baseIndex );
	NodeIndex fetchLeaf( const Vector3 & objCenter );

	// Validation helpers (must uncomment the implementations to use them)
	bool validateHeirachy() const;
	NodeIndex findParent( NodeIndex index ) const;

	StorageType data_;
};

typedef LooseOctree< LooseOctree_StaticData > StaticLooseOctree;
typedef LooseOctree< LooseOctree_DynamicData > DynamicLooseOctree;

namespace LooseOctreeHelpers
{
	void copyStorage( DynamicLooseOctree& dst, StaticLooseOctree& src );
	void copyStorage( StaticLooseOctree& dst, DynamicLooseOctree& src );
	void prepareStaticTreeStorage( DynamicLooseOctree& tree );
}

BW_END_NAMESPACE

#include "loose_octree.ipp"

#endif // LOOSE_OCTREE_HPP

