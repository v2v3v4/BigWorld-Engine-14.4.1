#include "pch.hpp"

#include "loose_octree.hpp"

#include "cstdmf/bw_deque.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace 
{

typedef LooseOctree_DataTypes::NodeIndex NodeIndex;
typedef LooseOctree_DataTypes::Node Node;
typedef BW::vector< NodeIndex > TranslationVector;

NodeIndex translateNodeIndex( const TranslationVector& srcToDst,
	NodeIndex srcIndex )
{
	if (srcIndex == LooseOctree_DataTypes::INVALID_NODE)
	{
		return srcIndex;
	}
	else
	{
		return srcToDst[ srcIndex ];
	}
}

template <typename DestinationStorage, typename SourceStorage>
NodeIndex translateNodeReference( const TranslationVector& srcToDst,
	NodeIndex srcRef )
{
	if (srcRef == LooseOctree_DataTypes::INVALID_NODE)
	{
		return srcRef;
	}
	else
	{
		NodeIndex srcIndex = SourceStorage::childIndexFromReference( srcRef );
		NodeIndex dstIndex = translateNodeIndex( srcToDst, srcIndex );

		NodeIndex dstRef = dstIndex;
		if (SourceStorage::isLeafRef( srcRef ))
		{
			dstRef |= DestinationStorage::LEAF_REF_FLAG;
		}
		return dstRef;
	}
}

template <typename DestinationStorage, typename SourceStorage>
void migrateNodeData( const TranslationVector& srcToDst,
	DestinationStorage& dst, NodeIndex dstNode, 
	SourceStorage& src, NodeIndex srcNode )
{
	dst.centerOfNode( dstNode ) = src.centerOfNode( srcNode );
	dst.boundOfNode( dstNode ) = src.boundOfNode( srcNode );
	dst.dataReferenceOfNode( dstNode ) = src.dataReferenceOfNode( srcNode );

	Node& dstChildren = dst.childrenOfNode( dstNode );
	Node& srcChildren = src.childrenOfNode( srcNode );
	dstChildren = srcChildren;

	// Now translate the node references
	for (size_t i = 0; i < LooseOctree_DataTypes::NUM_CHILDREN; ++i)
	{
		dstChildren.child[i] = 
			translateNodeReference<DestinationStorage, SourceStorage>(
				srcToDst, srcChildren.child[i] );
	}

	dst.parentOfNode( dstNode ) = 
		translateNodeIndex( srcToDst, src.parentOfNode( srcNode ) );
}

template <typename DestinationStorage, typename SourceStorage>
void migrateStorageData(const TranslationVector& srcToDst,
	DestinationStorage& dst,
	SourceStorage& src)
{
	size_t numNodes = src.nodes_.size();

	// Reserve room for all node data
	dst.config_ = src.config_;
	dst.bounds_.resize( numNodes );
	dst.centers_.resize( numNodes );
	dst.nodes_.resize( numNodes );
	dst.dataReferences_.resize( numNodes );
	dst.parents_.resize( numNodes );

	for (size_t i = 0; i < numNodes; ++i)
	{
		NodeIndex srcIndex = (NodeIndex)i;
		NodeIndex dstIndex = translateNodeIndex( srcToDst, srcIndex );
		migrateNodeData( srcToDst, dst, dstIndex, src, srcIndex );
	}
}

template <typename DestinationStorage, typename SourceStorage>
void copyStorageData( DestinationStorage& dst,
	SourceStorage& src )
{
	TranslationVector srcToDst;
	generateIdentityMapping( src.nodes_.size(), srcToDst );

	migrateStorageData( srcToDst, dst, src );
}

template <typename StorageType>
void generateBreadthFirstOrdering( const StorageType& storage,
	TranslationVector& order )
{
	BW::deque<NodeIndex> queue;
	queue.push_back( LooseOctree_DataTypes::ROOT_NODE );

	while (!queue.empty()) 
	{
		const NodeIndex curNode = 
			StorageType::childIndexFromReference( queue.front() );
		queue.pop_front();

		// Visited this node
		order.push_back( curNode );

		const Node& node = storage.childrenOfNode( curNode );
		for (uint32 i = 0; i < LooseOctree_DataTypes::NUM_CHILDREN; ++i) 
		{
			const NodeIndex childNode = node.child[i];
			if (childNode != LooseOctree_DataTypes::INVALID_NODE) 
			{
				queue.push_back( childNode );
			}
		}
	}
}

template <typename StorageType>
void generateDepthFirstOrdering( const StorageType& storage,
	TranslationVector& order )
{
	BW::vector<NodeIndex> stack;
	stack.push_back( LooseOctree_DataTypes::ROOT_NODE );

	while (!stack.empty()) 
	{
		const NodeIndex curNode = 
			StorageType::childIndexFromReference( stack.back() );
		stack.pop_back();

		// Visited this node
		order.push_back( curNode );

		const Node& node = storage.childrenOfNode( curNode );
		for (uint32 i = 0; i < LooseOctree_DataTypes::NUM_CHILDREN; ++i) 
		{
			const NodeIndex childNode = node.child[i];
			if (childNode != LooseOctree_DataTypes::INVALID_NODE) 
			{
				stack.push_back( childNode );
			}
		}
	}
}

void invertMapping( TranslationVector& order, TranslationVector& inversion )
{
	inversion.resize( order.size() );
	for (size_t i = 0; i < order.size(); ++i)
	{
		inversion[order[i]] = (NodeIndex)i;
	}
}

void generateIdentityMapping( size_t numNodes,
	TranslationVector& order )
{
	order.resize( numNodes );
	for (size_t i = 0; i < numNodes; ++i)
	{
		order[i] = (NodeIndex)i;
	}
}

void generateRandomOrdering( size_t numNodes,
	TranslationVector& order )
{
	generateIdentityMapping( numNodes, order );

	if (numNodes <= 1)
	{
		return;
	}

	std::random_shuffle( order.begin() + 1, order.end() );
}

template <typename StorageType>
void generateInsertionLocalChildrenOrder( const StorageType& storage,
	TranslationVector& order )
{
	// Iterate over all child lists of each node and remove 
	size_t numNodes = storage.nodes_.size();

	for (size_t i = 0; i < numNodes; ++i)
	{
		NodeIndex curNode = (NodeIndex)i;

		if (std::find( order.begin(), order.end(), curNode ) != order.end())
		{
			// Already placed this node in the destination ordering
			continue;
		}

		// Place this node directly in the array (insertion ordering)
		order.push_back( curNode );

		// Check if it has children
		if (storage.isLeaf( curNode ))
		{
			// Nothing to be done here
			continue;
		}

		// Check if any of it's children are leaves
		bool hasLeaves = false;
		const StorageType::Node& node = storage.childrenOfNode( curNode );
		for (size_t childIndex = 0; childIndex < StorageType::NUM_CHILDREN; ++childIndex)
		{
			NodeIndex nodeRef = node.child[childIndex];
			if (nodeRef == StorageType::INVALID_NODE)
			{
				continue;
			}

			NodeIndex childNode = StorageType::childIndexFromReference( nodeRef );
			if (storage.isLeaf( childNode ))
			{
				hasLeaves = true;
				break;
			}
		}

		if (!hasLeaves)
		{
			// Only want to play with leaf ordering
			continue;
		}

		// Group this node's children in a group directly after it
		for (size_t childIndex = 0; childIndex < StorageType::NUM_CHILDREN; ++childIndex)
		{
			NodeIndex nodeRef = node.child[childIndex];
			if (nodeRef == StorageType::INVALID_NODE)
			{
				continue;
			}

			NodeIndex childNode = StorageType::childIndexFromReference( nodeRef );
			order.push_back( childNode );
		}
	}

	MF_ASSERT( order.size() == numNodes );
}

void sortNodes( DynamicLooseOctree& tree )
{
	DynamicLooseOctree::StorageType newStorage;

	DynamicLooseOctree::StorageType& oldStorage = tree.storage();
	size_t numNodes = oldStorage.nodes_.size();
	if (numNodes == 0)
	{
		return;
	}

	// Perform breadth first search iteration and record the order
	// that we visited nodes in
	TranslationVector dstToSrc;
	//generateInsertionLocalChildrenOrder( oldStorage, dstToSrc );
	//generateBreadthFirstOrdering( oldStorage, dstToSrc );
	generateDepthFirstOrdering( oldStorage, dstToSrc );
	//generateRandomOrdering( numNodes, dstToSrc );
	
	// Find the inverse of the translation
	TranslationVector srcToDst;
	invertMapping( dstToSrc, srcToDst );

	// Migrate all the nodes to their new locations
	migrateStorageData( srcToDst, newStorage, oldStorage );
	
	// Now put the data back in the original place
	copyStorageData( oldStorage, newStorage );
}

void compressSparseChildLists( DynamicLooseOctree& tree )
{
	DynamicLooseOctree::StorageType& storage = tree.storage();

	// Iterate over all child lists of each node and remove 
	size_t numNodes = storage.nodes_.size();

	for (size_t i = 0; i < numNodes; ++i)
	{
		NodeIndex curNode = (NodeIndex)i;
		Node& node = storage.childrenOfNode( curNode );

		// Iterate over all children and shift up any blank children
		for (size_t i = 0; i < LooseOctree_DataTypes::NUM_CHILDREN; ++i)
		{
			NodeIndex childRef = node.child[i];

			// If the current child is not blank, then look at the next one
			if (childRef != LooseOctree_DataTypes::INVALID_NODE)
			{
				continue;
			}

			// We need to find another node to put in this place then, 
			// iterate until we find one
			for (size_t candidateIndex = i + 1; 
				candidateIndex < LooseOctree_DataTypes::NUM_CHILDREN; 
				++candidateIndex)
			{
				NodeIndex candidateRef = node.child[candidateIndex];

				// If the current child is not blank, then look at the next one
				if (candidateRef != LooseOctree_DataTypes::INVALID_NODE)
				{
					node.child[i] = candidateRef;
					node.child[candidateIndex] = LooseOctree_DataTypes::INVALID_NODE;
					break;
				}
			}
		}
	}
}

} // Anonymous namespace

namespace LooseOctreeHelpers
{

void copyStorage( DynamicLooseOctree& dst, StaticLooseOctree& src )
{
	DynamicLooseOctree::StorageType& dstStorage = dst.storage();
	StaticLooseOctree::StorageType& srcStorage = src.storage();

	copyStorageData( dstStorage, srcStorage );
}

void copyStorage( StaticLooseOctree& dst, DynamicLooseOctree& src )
{
	StaticLooseOctree::StorageType& dstStorage = dst.storage();
	DynamicLooseOctree::StorageType& srcStorage = src.storage();

	copyStorageData( dstStorage, srcStorage );
}

// Sorting octree nodes for static loose octrees
void prepareStaticTreeStorage( DynamicLooseOctree& tree )
{
	// sort all the nodes into parents followed by their children. 
	// keep siblings beside eachother.
	// Breadth first traversal ordering
	sortNodes( tree );

	// Compress sparse child lists
	// means insertion is no longer possible
	compressSparseChildLists( tree );
}

} // LooseOctreeHelpers namespace
BW_END_NAMESPACE

// loose_octree.cpp
