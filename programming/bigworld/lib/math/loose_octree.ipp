
BW_BEGIN_NAMESPACE

inline LooseOctree_DataTypes::Node::Node()
{
	// Initialize all child nodes to invalid
	for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
	{
		child[i] = INVALID_NODE;
	}
}


inline LooseOctree_DataTypes::Configuration::Configuration() :
	maxDepth_( 0 ),
	worldCenter_( Vector3::ZERO ),
	worldSize_( 1.0f ),
	numDataEntries_( 0 )
{
}


template <class StorageTypess>
typename LooseOctreeStorage< StorageTypess >::NodeCenter&
	LooseOctreeStorage< StorageTypess >::centerOfNode( NodeIndex index )
{
	MF_ASSERT( index < centers_.size() );
	return centers_[index];
}


template <class StorageTypes>
const typename LooseOctreeStorage< StorageTypes >::NodeCenter&
	LooseOctreeStorage< StorageTypes >::centerOfNode( NodeIndex index ) const
{
	LooseOctreeStorage * nonConst = const_cast<LooseOctreeStorage*>(this);
	return nonConst->centerOfNode( index );
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeBound&
	LooseOctreeStorage< StorageTypes >::boundOfNode( NodeIndex index )
{
	MF_ASSERT( index < bounds_.size() );
	return bounds_[index].box;
}


template <class StorageTypes>
const typename LooseOctreeStorage< StorageTypes >::NodeBound&
	LooseOctreeStorage< StorageTypes >::boundOfNode( 
	NodeIndex index ) const
{
	LooseOctreeStorage * nonConst = const_cast<LooseOctreeStorage*>(this);
	return nonConst->boundOfNode( index );
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::Node&
	LooseOctreeStorage< StorageTypes >::childrenOfNode( NodeIndex index )
{
	MF_ASSERT( index < nodes_.size() );
	return nodes_[index];
}


template <class StorageTypes>
const typename LooseOctreeStorage< StorageTypes >::Node&
	LooseOctreeStorage< StorageTypes >::childrenOfNode( 
	NodeIndex index ) const
{
	LooseOctreeStorage * nonConst = const_cast<LooseOctreeStorage*>(this);
	return nonConst->childrenOfNode( index );
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeIndex&
	LooseOctreeStorage< StorageTypes >::parentOfNode( NodeIndex index )
{
	MF_ASSERT( index < nodes_.size() );
	return parents_[index];
}


template <class StorageTypes>
const typename LooseOctreeStorage< StorageTypes >::NodeIndex&
	LooseOctreeStorage< StorageTypes >::parentOfNode( 
	NodeIndex index ) const
{
	LooseOctreeStorage * nonConst = const_cast<LooseOctreeStorage*>(this);
	return nonConst->parentOfNode( index );
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeDataReference&
	LooseOctreeStorage< StorageTypes >::dataReferenceOfNode( NodeIndex index )
{
	MF_ASSERT( index < dataReferences_.size() );
	return dataReferences_[index];
}


template <class StorageTypes>
const typename LooseOctreeStorage< StorageTypes >::NodeDataReference&
	LooseOctreeStorage< StorageTypes >::dataReferenceOfNode( NodeIndex index ) const
{
	LooseOctreeStorage * nonConst = const_cast<LooseOctreeStorage*>(this);
	return nonConst->dataReferenceOfNode( index );
}


template <class StorageTypes>
bool LooseOctreeStorage< StorageTypes >::isLeaf( NodeIndex index ) const
{
	return dataReferenceOfNode( index ) != INVALID_DATAREFERENCE;
}


template <class StorageTypes>
bool LooseOctreeStorage< StorageTypes >::isLeafRef( NodeIndex childReference )
{
	return (childReference & LEAF_REF_FLAG) != 0;
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeIndex 
	LooseOctreeStorage< StorageTypes >::childIndexFromReference( 
		NodeIndex childReference )
{
	return childReference & INDEX_MASK;
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeIndex
	LooseOctreeStorage< StorageTypes >::newNode( NodeIndex parent,
	const NodeCenter& center, 
	bool isLeaf )
{
	BW_STATIC_ASSERT( StorageTypes::s_supportsInsertion, 
		Insertion_Not_Supported_By_Octree_Storage_Type);

	nodes_.push_back( Node() );
	bounds_.push_back( NodeBoundData() );
	centers_.push_back( center );
	parents_.push_back( parent );

	NodeDataReference dataIndex = INVALID_DATAREFERENCE;
	if (isLeaf)
	{
		dataIndex = assignNewData();
	}
	dataReferences_.push_back( dataIndex );

	NodeIndex newIndex = static_cast<NodeIndex>(nodes_.size() - 1);

	// If this assertion is reached, then the number of nodes
	// in the octree has exceeded the amount of nodes that can
	// be represented. At this point you would need to consider
	// reducing the maxDepth of your tree, or alternatively
	// increase the size of the type "NodeIndex" to support more nodes.
	MF_ASSERT( (newIndex & INDEX_MASK) == newIndex );

	return newIndex;
}


template <class StorageTypes>
typename LooseOctreeStorage< StorageTypes >::NodeIndex
	LooseOctreeStorage< StorageTypes >::assignNewData()
{
	return config_.numDataEntries_++;
}


template <class StorageTypes>
void LooseOctreeStorage< StorageTypes >::translateDataReferences( 
		RuntimeNodeDataReferences& references ) const
{
	// Batch translation of node references into a flat list
	size_t numReferences = references.size();
	for (size_t i = 0; i < numReferences; ++i)
	{
		NodeDataReference nodeIndex = references[i];
		references[i] = dataReferenceOfNode( nodeIndex );
	}
}


template <class StorageType>
LooseOctree< StorageType >::LooseOctree() :
	data_()
{
}


template <class StorageType>
LooseOctree< StorageType >::LooseOctree( const Vector3 & center, 
	float worldSize, uint32 maxDepth )
{
	initialise( center, worldSize, maxDepth );
}


template <class StorageType>
void LooseOctree< StorageType >::initialise( 
	const Vector3 & center, float worldSize, uint32 maxDepth )
{
	// Must always have a depth of at least 1
	// Due to the way traversals use the node references
	// of their children to work out if they're leaves or not
	// therefore there must always be one parent.
	MF_ASSERT( maxDepth >= 1 );

	// Must not already have data stored.
	MF_ASSERT( data_.nodes_.empty() );

	data_.config_.maxDepth_ = maxDepth;
	data_.config_.worldCenter_ = center;
	data_.config_.worldSize_ = worldSize;
	data_.config_.numDataEntries_ = 0;

	// Add the root node first
	data_.newNode( INVALID_NODE, center, 0 == data_.config_.maxDepth_ );
}


template <class StorageType>
void LooseOctree< StorageType >::reinitialise()
{
	reset();

	initialise( data_.config_.worldCenter_,
		data_.config_.worldSize_,
		data_.config_.maxDepth_ );
}


template <class StorageType>
void LooseOctree< StorageType >::reset()
{
	data_.bounds_.clear();
	data_.centers_.clear();
	data_.nodes_.clear();
	data_.dataReferences_.clear();
	data_.parents_.clear();
}


template <class StorageType>
StorageType& LooseOctree< StorageType >::storage()
{
	return data_;
}


template <class StorageType>
const StorageType& LooseOctree< StorageType >::storage() const
{
	return data_;
}


template <class StorageType>
typename LooseOctree< StorageType >::NodeIndex
	LooseOctree< StorageType >::insert( 
		const Vector3 & objCenter, float objRadius )
{
	// Convert new object's bound sphere into a bb
	Vector3 radiusOffset( objRadius, objRadius, objRadius );
	AABB objBB( objCenter - radiusOffset, objCenter + radiusOffset );

	return insert( objBB );
}


template <class StorageType>
typename LooseOctree< StorageType >::NodeIndex 
	LooseOctree< StorageType >::insert( const AABB& objBB )
{
	if (objBB.insideOut())
	{
		return INVALID_NODE;
	}

	NodeIndex curNode = fetchLeaf( objBB.centre() );

	AABB& bb = data_.boundOfNode( curNode );
	bb.addBounds( objBB );

	return curNode;
}


template <class StorageType>
typename LooseOctree< StorageType >::NodeIndex 
	LooseOctree< StorageType >::findData( const Vector3 & objCenter )
	 const
{
	NodeIndex curNode = ROOT_NODE;
	NodeIndex depth = 0;
	while (!isLeaf( curNode )) 
	{
		// Which child node of this one should we place it?
		const Vector3 &center = data_.centerOfNode( curNode );
		int x = objCenter.x < center.x ? LEFT_CHILD_SIDE : RIGHT_CHILD_SIDE;
		int y = objCenter.y < center.y ? BOTTOM_CHILD_SIDE : TOP_CHILD_SIDE;
		int z = objCenter.z < center.z ? FRONT_CHILD_SIDE : BACK_CHILD_SIDE;

		const Node& node = data_.childrenOfNode( curNode );
		NodeIndex childRef = node.child[x | y | z];

		if (childRef == INVALID_NODE)
		{
			return INVALID_DATAREFERENCE;
		}

		// Traverse deeper into the tree
		curNode = StorageType::childIndexFromReference( childRef );
	}

	return curNode;
}

template <class StorageType>
typename LooseOctree< StorageType >::NodeDataReference 
	LooseOctree< StorageType >::dataOnLeaf( NodeIndex nodeID ) const
{
	return data_.dataReferenceOfNode( nodeID );
}

template <class StorageType>
typename LooseOctree< StorageType >::NodeIndex 
	LooseOctree< StorageType >::fetchLeaf( const Vector3 & objCenter )
{
	NodeIndex curNode = ROOT_NODE;
	NodeIndex depth = 0;
	while (!isLeaf( curNode )) 
	{
		// Which child node of this one should we place it?
		const Vector3 &center = data_.centerOfNode( curNode );
		int x = objCenter.x < center.x ? LEFT_CHILD_SIDE : RIGHT_CHILD_SIDE;
		int y = objCenter.y < center.y ? BOTTOM_CHILD_SIDE : TOP_CHILD_SIDE;
		int z = objCenter.z < center.z ? FRONT_CHILD_SIDE : BACK_CHILD_SIDE;

		const Node& node = data_.childrenOfNode( curNode );
		NodeIndex childRef = node.child[x | y | z];

		if (childRef == INVALID_NODE)
		{
			// Create a new node then! 
			float halfSize = data_.config_.worldSize_ / (2 << depth);
			float quarterSize = halfSize / 2;
			float offset = quarterSize;

			// What should it's center be?
			float cx = center.x + ((x == LEFT_CHILD_SIDE) ? -offset : offset);
			float cy = center.y + ((y == BOTTOM_CHILD_SIDE) ? -offset : offset);
			float cz = center.z + ((z == FRONT_CHILD_SIDE) ? -offset : offset);

			const NodeIndex childDepth = depth + 1;
			bool isLeaf = childDepth == data_.config_.maxDepth_;
			childRef = data_.newNode( curNode, Vector3(cx, cy, cz), isLeaf );

			// Add on the child flag
			childRef |= (isLeaf) ? (LEAF_REF_FLAG) : (0);

			// Node is now invalidated! (newNode caused resize)
			Node& parentNode = data_.childrenOfNode( curNode );
			parentNode.child[x | y | z] = childRef;
		}

		// Traverse deeper into the tree
		++depth;
		curNode = StorageType::childIndexFromReference( childRef );
	}

#if 0
	// Usefull code for validating the structure of the octree.
	MF_ASSERT( validateHeirachy() );
#endif

	return curNode;
}


template <class StorageType>
void LooseOctree< StorageType >::intersect( const ConvexHull & hull, 
	RuntimeNodeDataReferences & dataReferences ) const
{
	LOG_QUERY( hull );

	struct HullIntersectShape
	{
		bool intersects( const NodeBound& bb, uint16& state )
		{
			// Optimisation by disabling planes for children
			// that the parent has determined it is fully inside
			return hull_.intersects( bb, state );
		}

		uint16 defaultState()
		{
			// Enable all planes by default
			return ~0;
		}

		const ConvexHull & hull_;
	};

	HullIntersectShape shape = { hull };
	intersect( shape, dataReferences );
}


template <class StorageType>
void LooseOctree< StorageType >::intersect( const Vector3 & start, 
	const Vector3 & stop, RuntimeNodeDataReferences & dataReferences ) const
{
	LOG_QUERY( start, stop );

	struct RayIntersectShape
	{
		bool intersects( const NodeBound& bb, uint16& state  )
		{
			// NOTE: Possible optimisation by allowing traversal to 
			// visit data references of each node as the intersection is performed.
			// This may allow early outs at the expense of context switching
			// between the traversal and content testing.

			Vector3 hitPoint; 	
			int res = bb.intersectsRay( start_, raySegmentDir_, hitPoint );
			bool recurseNode = false;
			if (res == AABB::ORIGIN_INSIDE) 
			{
				return true;					
			}
			else if (res == AABB::INTERSECTION)
			{
				// check node intersection is closer than our max ray length
				if ((hitPoint - start_).lengthSquared() < endRayDistSqr_)
				{
					return true;
				}
			}

			return false;
		}

		uint16 defaultState()
		{
			return 0;
		}

		const Vector3 & start_;
		const Vector3 & raySegmentDir_;
		float endRayDistSqr_;
	};

	Vector3 rayDir = stop - start;
	float endRayDistSqr = rayDir.lengthSquared();
	RayIntersectShape shape = { start, rayDir, endRayDistSqr };
	intersect( shape, dataReferences );
}


template <class StorageType>
void LooseOctree< StorageType >::intersect( const AABB & aabb,
	RuntimeNodeDataReferences & dataReferences ) const
{
	LOG_QUERY( aabb );

	struct BBIntersectShape
	{
		bool intersects( const NodeBound& bb, uint16& state )
		{
			// NOTE: Possible optimisation by disabling planes for children
			// that the parent has determined it is fully inside
			// (use the planeFlags parameter to hull.intersects and add flags
			// to the node stack)

			return bb_.intersects( bb );
		}

		uint16 defaultState()
		{
			return 0;
		}

		const AABB & bb_;
	};

	BBIntersectShape shape = { aabb };
	intersect( shape, dataReferences );
}


template <class TStorageType>
struct LooseOctree_TraversalHelpers
{
	typedef TStorageType StorageType;
	typedef typename StorageType::Node Node;
	typedef typename StorageType::NodeIndex NodeIndex;

	static const uint32 MAX_DEPTH = 10;
	static const uint32 MAX_STACK_ENTRIES = 
		StorageType::NUM_CHILDREN * MAX_DEPTH;
	
	// Defining this struct using bitfields
	// as it allows the visual studio compiler to treat the 16 bit accesses as
	// operations on a 32bit location and allows the 'pushValidChildren' 
	// iterations to no longer have load-hit-store performance issues, 
	// as temporary NodeEntrys are kept in a register instead of stack memory.
	struct NodeEntry
	{
		uint32 node : 16;
		uint32 state : 16;
	};

	static NodeEntry makeEntry(NodeIndex index, uint16 state)
	{
		NodeEntry entry;
		entry.state = state;
		entry.node = index;
		return entry;
	}

	typedef StaticArray<NodeEntry, MAX_STACK_ENTRIES> TraversalStack;

	static inline void pushValidChildren(const uint16& state, 
		const Node& node, 
		TraversalStack& stack);	
};


template <class StorageType>
inline void LooseOctree_TraversalHelpers<StorageType>::pushValidChildren(
	const uint16& state, const Node& node, TraversalStack& stack)
{
	for (uint32 i = 0; i < StorageType::NUM_CHILDREN; ++i) 
	{
		const NodeIndex& childRef = node.child[i];
		if (childRef != StorageType::INVALID_NODE) 
		{
			stack.push_back( makeEntry( childRef, state ) );
		}
	}
}


template <>
inline void LooseOctree_TraversalHelpers<LooseOctree_StaticData>::pushValidChildren(
	const uint16& state, const Node& node, TraversalStack& stack)
{
	for (uint32 i = 0; i < StorageType::NUM_CHILDREN; ++i) 
	{
		const NodeIndex& childRef = node.child[i];
		if (childRef == StorageType::INVALID_NODE)
		{
			break;
		}

		stack.push_back( makeEntry( childRef, state ) );
	}
}


template <class StorageType>
template <class IntersectShape>
void LooseOctree< StorageType >::intersect( IntersectShape& shape, 
	RuntimeNodeDataReferences & dataReferences ) const
{
	if (empty())
	{
		return;
	}

	// Preallocate some reference entries
	const uint32 DEFAULT_REFERENCES_ALLOCATION = 128;
	dataReferences.reserve( DEFAULT_REFERENCES_ALLOCATION );

	typedef typename LooseOctree_TraversalHelpers<StorageType>::TraversalStack TraversalStack;
	typedef typename LooseOctree_TraversalHelpers<StorageType>::NodeEntry NodeEntry;

	TraversalStack stack;
	NodeEntry entry = LooseOctree_TraversalHelpers<StorageType>::makeEntry( 
		ROOT_NODE, shape.defaultState() );
	stack.push_back( entry );

	while (!stack.empty()) 
	{
		entry = stack.back();
		stack.pop_back();

		const NodeIndex nodeRef = entry.node;
		const NodeIndex curNode = StorageType::childIndexFromReference( nodeRef );
		uint16 state = entry.state;

		const NodeBound& bb = data_.boundOfNode( curNode );
		const bool includeNode = shape.intersects( bb, state );
		if (!includeNode) 
		{
			continue;
		}

		const bool isLeafNode = StorageType::isLeafRef( nodeRef );
		if (isLeafNode) 
		{
			dataReferences.push_back( curNode );
		}
		else
		{
			const Node& node = data_.childrenOfNode( curNode );

			// Select the appropriate implementation of 
			// pushing valid children for different storage types.
			// Dynamic storage types have sparse children
			LooseOctree_TraversalHelpers<StorageType>::pushValidChildren( state, node, stack );
		}
	}

	// Batch reference translation
	data_.translateDataReferences( dataReferences );
}


template <class StorageType>
template <class Visitor>
void LooseOctree< StorageType >::traverse( Visitor& visitor, 
	NodeIndex nodeIndex, size_t level )
{
	if (empty())
	{
		return;
	}

	bool recurseNode = visitor.visit( nodeIndex,
		data_.boundOfNode( nodeIndex ),
		level,
		data_.dataReferenceOfNode( nodeIndex ) );

	if (recurseNode) 
	{
		if (!isLeaf(nodeIndex)) 
		{
			const Node& node = data_.childrenOfNode( nodeIndex );
			for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
			{
				const NodeIndex childNodeRef = node.child[i];
				if (childNodeRef != INVALID_NODE) 
				{
					traverse( visitor, 
						StorageType::childIndexFromReference(childNodeRef), 
						level + 1 );
				}
			}
		}
	}
}


template <class StorageType>
bool LooseOctree< StorageType >::empty() const
{
	return data_.config_.numDataEntries_ == 0;
}


template <class StorageType>
void LooseOctree< StorageType >::mergeChildBounds( 
	NodeIndex index, AABB& bb )
{
	const Node &node = data_.childrenOfNode( index );
	for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
	{
		const NodeIndex childNodeRef = node.child[i];
		
		if (childNodeRef != INVALID_NODE) 
		{	
			const NodeIndex childNode = StorageType::childIndexFromReference( childNodeRef );
			bb.addBounds( data_.boundOfNode( childNode ) );
		}
	}
}


template <class StorageType>
void LooseOctree< StorageType >::updateHeirarchy()
{
	if (data_.nodes_.size() == 1 && 
		data_.config_.maxDepth_ > 0)
	{
		// Only a root node exists. Otherwise it is empty
		return;
	}

	updateHeirarchy( ROOT_NODE );
}


template <class StorageType>
typename LooseOctree< StorageType >::NodeBound
	LooseOctree< StorageType >::updateHeirarchy( NodeIndex baseIndex )
{
	// get bounds of children of this node and merge 
	NodeBound& bb = data_.boundOfNode( baseIndex );

	if (isLeaf( baseIndex )) 
	{
		MF_ASSERT( !bb.insideOut() );
		return bb;
	}

	NodeBound newBB;
	// recurse into children
	const Node &node = data_.childrenOfNode( baseIndex );
	for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
	{
		const NodeIndex childNodeRef = node.child[i];

		if (childNodeRef != INVALID_NODE) 
		{
			const NodeIndex childNode = StorageType::childIndexFromReference( childNodeRef );
			newBB.addBounds( updateHeirarchy( childNode ) );
		}
	}

	bb = newBB;

	// If this BB is inside out then this nodes children are also inside out.
	// Or there are no children - so why is this node even here?
	MF_ASSERT( !bb.insideOut() );

	return bb;
}


template <class StorageType>
void LooseOctree< StorageType >::updateNodeBound( 
	NodeIndex nodeID )
{
	if (nodeID == INVALID_NODE)
	{
		return;
	}

	// Update the heirachy around this particular node.
	// It's bound has likely been just updated by an insertion operation.
	// (alternative to the batch update)

	// Propagate the changes up the tree
	while (nodeID != ROOT_NODE)
	{
		// Move on to the parent node
		nodeID = data_.parentOfNode( nodeID );

		// Calculate the node's BB
		AABB newBB;
		mergeChildBounds( nodeID, newBB );

		// Save the new BB
		data_.boundOfNode( nodeID ) = newBB;
	}
}


template <class StorageType>
void LooseOctree< StorageType >::updateNodeBound( 
	NodeIndex nodeID, const AABB& bb )
{
	if (nodeID == INVALID_NODE)
	{
		return;
	}

	// Update the heirachy around this particular node having this new BB.
	// This allows implementation of removal or update operations on 
	// dynamic octree contents.
	data_.boundOfNode( nodeID ) = bb;

	updateNodeBound( nodeID );
}


template <class StorageType>
bool LooseOctree< StorageType >::isLeaf( NodeIndex index ) const
{
	return data_.isLeaf( index );
}


template <class StorageType>
uint32 LooseOctree< StorageType >::numChildren( NodeIndex index ) const
{
	uint32 count = 0;
	const Node &node = data_.childrenOfNode(index);
	for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
	{
		const NodeIndex childNodeRef = node.child[i];

		if (childNodeRef != INVALID_NODE)
		{
			++count;
		}
	}
	return count;
}


template <class StorageType>
const AABB&  LooseOctree< StorageType >::rootBoundingBox() const
{
	return data_.boundOfNode( ROOT_NODE );
}

#if 0

template <class StorageType>
bool LooseOctree< StorageType >::validateHeirachy() const
{
	for (NodeIndex index = 0; index < data_.nodes_.size(); ++index )
	{
		// Constraints that root node doesn't have to abide by
		if (index != 0)
		{
			// Make sure this node has a parent
			if (findParent( index ) == INVALID_NODE)
			{
				return false;
			}

			// Make sure leaves don't have children and 
			// nodes always have children.
			if (isLeaf(index) != (numChildren(index) == 0))
			{
				return false;
			}
		}

		// Make sure children are pointing at valid locations
		const Node& node = data_.nodes_[index];
		for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
		{
			const NodeIndex childNodeRef = node.child[i];
			if (childNodeRef != INVALID_NODE)
			{
				const NodeIndex childNode = StorageType::childIndexFromReference( childNodeRef );
				const NodeIndex parent = findParent( childNode );
				if ( parent != index )
				{
					return false;
				}
			}
		}
	}

	return true;
}


template <class StorageType>
typename LooseOctree< StorageType >::NodeIndex
	LooseOctree< StorageType >::findParent( NodeIndex childIndex ) const
{
	// Slow!
	// Only for use during validation
	for (NodeIndex index = 0; index < data_.nodes_.size(); ++index )
	{
		const Node& node = data_.nodes_[index];

		for (uint32 i = 0; i < NUM_CHILDREN; ++i) 
		{
			const NodeIndex childNodeRef = node.child[i];
			const NodeIndex childNode = StorageType::childIndexFromReference( childNodeRef );
			if (childNode == childIndex)
			{
				return index;
			}
		}
	}

	return INVALID_NODE;
}
#endif

BW_END_NAMESPACE

// loose_octree.ipp
