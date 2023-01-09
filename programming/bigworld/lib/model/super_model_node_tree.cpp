
#include "pch.hpp"

#include <utility>
#include <iterator>
#include <algorithm>
#include "cstdmf/guard.hpp"
#include "super_model_node_tree.hpp"
#include "moo/geometrics.hpp"
#include "model/super_model.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 )

//If this is non-zero, some debug messages will be printed when nodes fail
//to add to the tree.
#define SMNT_DEBUG !CLIENT_CONSUMER_BUILD

BW_BEGIN_NAMESPACE

/**
 *	@pre A node for @a identifier exists in Moo::NodeCatalogue.
 *	@post Created a node for the tree with the specified identifier.
 */
SuperModelNode::SuperModelNode( const BW::string & identifier )
	:
	identifier_( identifier )
	, cachedMooNode_( Moo::NodeCatalogue::find( identifier.c_str() ) )
	, parentIndex_(-1)
{
	BW_GUARD;

	MF_ASSERT( cachedMooNode_.hasObject() );
}


/**
 *	@return The name identifying this node.
 */
const BW::string & SuperModelNode::getIdentifier() const
{
	BW_GUARD;

	return identifier_;
}


/**
 *	@return The Moo::NodeCatalogue node cached for the identifier of this
 *		node at construction.
 */
Moo::NodePtr & SuperModelNode::getMooNode()
{
	return cachedMooNode_;
}


///Const version.
const Moo::NodePtr & SuperModelNode::getMooNode() const
{
	return cachedMooNode_;
}


/**
 *	@return The index for this node's parent in the owning SuperModelNodeTree,
 *		or negative if no parent.
 */
int SuperModelNode::getParentIndex() const
{
	BW_GUARD;

	return parentIndex_;
}


/**
 *	@pre True.
 *	@post Set the parent index for this node in the owning
 *		SuperModelNodeTree. Negative indicates no parent.
 */
void SuperModelNode::setParentIndex( int parentIndex )
{
	BW_GUARD;

	parentIndex_ = parentIndex;
}


/**
 *	@return The number of children for this node.
 */
int SuperModelNode::getNumChildren() const
{
	BW_GUARD;

	return static_cast<int>(childNodes_.size());
}


/**
 *	@pre @a treeIndex is positive.
 *	@post Registered that this node has a child with the index in the owning
 *		SuperModelNodeTree of @a treeIndex.
 */
void SuperModelNode::addChild( int treeIndex )
{
	BW_GUARD;

	MF_ASSERT( treeIndex >= 0 );

	childNodes_.push_back( treeIndex );
}


/**
 *	@pre @a childIndex in the range [0, getNumChildren()).
 *	@post The index (in the owning SuperModelTree) of the specified child.
 */
int SuperModelNode::getChild( int childIndex ) const
{
	BW_GUARD;

	MF_ASSERT( 0 <= childIndex );
	MF_ASSERT( childIndex < static_cast<int>(childNodes_.size()) );

	return childNodes_[childIndex];
}


/**
 *	@return A description (intended to be used only for debug purposes)
 *		of this node.
 */
BW::string SuperModelNode::desc() const
{
	BW_GUARD;

	BW::ostringstream resultStream;
	resultStream << "SuperModelNode(identifier=" <<
		identifier_ << ";parentIndex=" << parentIndex_ <<
		";children(" << getNumChildren() << ")=(";
	
	for (BW::vector<int>::const_iterator itr = childNodes_.begin();
		itr != childNodes_.end();
		)
	{
		resultStream << (*itr);

		++itr;

		if (itr != childNodes_.end())
		{
			resultStream << ";";
		}
	}
	resultStream << "))";

	return resultStream.str();
}


/**
 *	@param superModel The owning SuperModel for this tree.
 *	@param baseColour The dominant colour for the tree.
 *	@param drawSize The length of axis lines, etc.
 *	@pre @a superModel owns this tree and all node data is currently
 *		synchronised.
 *	@post Drew the nodes of the tree connected by lines for debug viewing
 *		purposes.
 */
void SuperModelNodeTree::draw( const SuperModel & superModel,
	Moo::PackedColour baseColour, float drawSize ) const
{
	const Moo::PackedColour STD_NODE_COL = baseColour;
	const Moo::PackedColour HP_NODE_COL = Moo::PackedColours::CYAN;
	const Moo::PackedColour BLEND_NODE_COL = Moo::PackedColours::YELLOW;

	//For every node.
	for(int nodeIndex = 0; nodeIndex < getNumNodes(); ++nodeIndex)
	{
		const SuperModelNode & node = getNode( nodeIndex );

		if (node.getParentIndex() < 0)
		{
			//Lines drawn parent->child, so just draw a star for root nodes.
			const Matrix & nodeWorldTrans =
				superModel.getNodeWorldTransform( nodeIndex );
			const Vector3 nodePos = nodeWorldTrans.applyToOrigin();
			Geometrics::drawStar( nodePos, drawSize,
				baseColour, Moo::PackedColours::RED );
			Geometrics::drawAxesCentreColoured( nodeWorldTrans, drawSize );

			continue;
		}

		const BW::string & nodeId = node.getIdentifier();
		Moo::PackedColour colour = STD_NODE_COL;
		if (nodeId.compare( 0, 3, "HP_" ) == 0)
		{
			colour = HP_NODE_COL;
			continue;
		}
		else if (nodeId.find( "BlendBone", 0 ) != nodeId.npos)
		{
			colour = BLEND_NODE_COL;
			continue;
		}

		const Vector3 fromPos = superModel.getNodeWorldTransform(
			node.getParentIndex() ).applyToOrigin();

		const Vector3 toPos = superModel.getNodeWorldTransform(
			nodeIndex ).applyToOrigin();

		Geometrics::drawLineTwoColour( 
			fromPos, toPos, baseColour, colour );
		Geometrics::drawAxesCentreColoured(
			superModel.getNodeWorldTransform( nodeIndex ),
			drawSize );
	}
}










/**
 *	@param rootFromVisual Node subtree to add. Should be the root node from an
 *		actual visual that was loaded for the owning SuperModel, to ensure
 *		that it has the correct parent/child relationships.
 *	@pre @a rootFromVisual has no parent.
 *	@pre The identifiers and parent/children of nodes that are added (and their
 *		children, recursively) must not change. If they do the tree structure
 *		will be inconsistent and must be regenerated.
 *	@pre A node with the same identifier as @a rootFromVisual exists in
 *		the Moo::NodeCatalogue.
 *	@post On true return: added @a rootFromVisual and all children recursively
 *			to this tree structure.
 *		On false return: the subtree for @a rootFromVisual was incompatible with
 *			the pre-existing graph (different non-missing parent/child
 *			relationship for at least one node). The state of the structure is
 *			indeterminate. It shouldn't be used now because the data has not
 *			met the necessary requirements.
 */
bool SuperModelNodeTree::addAllNodes( const Moo::Node & rootFromVisual )
{
	BW_GUARD;

	MF_ASSERT( !rootFromVisual.parent().hasObject() );

	if (!addAllNodesInternal( rootFromVisual, -1 ))
	{
		return false;
	}
	
	return true;
}


/**
 *	@return The total number of nodes in the tree.
 */
int SuperModelNodeTree::getNumNodes() const
{
	BW_GUARD;

	return static_cast<int>(nodes_.size());
}


/**
 *	@return The index of the node named @a identifier, or negative if no
 *		such node exists.
 */
int SuperModelNodeTree::getNodeIndex( const BW::string & identifier ) const
{
	BW_GUARD;

	const NodeIdToIndexMap::const_iterator srchRes =
		nodeLookup_.find( identifier );

	return (srchRes == nodeLookup_.end()) ? -1 : srchRes->second;
}


/**
 *	@pre @ index in the range [0, getNumNodes()).
 *	@post Returned reference to the node for @a index.
 */
const SuperModelNode & SuperModelNodeTree::getNode( int index ) const
{
	BW_GUARD;

	MF_ASSERT( 0 <= index );
	MF_ASSERT( index < getNumNodes() );

	return nodes_[index];
}


/**
 *	@param nodeIndex The index of a node in this tree.
 *	@param identifier The identifier for the child of the node referred to
 *		by @a nodeIndex that we want to search for.
 *	@pre @a nodeIndex is in the range [0, getNumNodes()).
 *	@post Returned the index (in this tree) of the child of @a nodeIndex with
 *		identifier @a identifier, or negative if no such child exists.
 */
int SuperModelNodeTree::findChild(
	int nodeIndex, const BW::string & identifier ) const
{
	const int childNodeIndex = getNodeIndex( identifier );
	if ((childNodeIndex < 0)
		||
		(getNode( childNodeIndex ).getParentIndex() != nodeIndex))
	{
		return -1;
	}
	return childNodeIndex;
}


/**
 *	@return The number of nodes in the tree that have no parent.
 */
int SuperModelNodeTree::getNumRootNodes() const
{
	BW_GUARD;

	int result = 0;

	for (BW::vector<SuperModelNode>::const_iterator itr = nodes_.begin();
		itr != nodes_.end();
		++itr)
	{
		if (itr->getParentIndex() < 0)
		{
			++result;
		}
	}

	MF_ASSERT( (result > 0) || (getNumNodes() == 0) );
	return result;
}

/**
 *	@pre @a parentIndex and @a queryIsDescendantIndex are in the range
 *		[0, getNumNodes()).
 *	@post Returned true iff the node with index @a queryIsDescendantIndex is
 *		a direct or indirect child of @a parentIndex.
 */
bool SuperModelNodeTree::isDescendantOf(
	int parentIndex, int queryIsDescendantIndex ) const
{
	BW_GUARD;

	MF_ASSERT( 0 <= parentIndex );
	MF_ASSERT( parentIndex < getNumNodes() );
	MF_ASSERT( 0 <= queryIsDescendantIndex );
	MF_ASSERT( queryIsDescendantIndex < getNumNodes() );

	const SuperModelNode & parentNode = getNode( parentIndex );
	for (int childIndex = 0;
		childIndex < parentNode.getNumChildren();
		++childIndex)
	{
		const int childTreeIndex = parentNode.getChild( childIndex );
		if ((childTreeIndex == queryIsDescendantIndex)
			||
			isDescendantOf( childTreeIndex, queryIsDescendantIndex ))
		{
			return true;
		}
	}

	return false;
}


namespace
{
	/**
	 *	Helper function used by SuperModelNodeTree::getPathIndices.
	 *	It is called recursively to perform the search.
	 *	
	 *	This function adds to @a pathIndices starting from the end index and
	 *	finishing one before the start index, because it's simpler. Reverse the
	 *	arguments to reverse the order.
	 *	
	 *	@param inputIndex Is the index of the node that @a startIndex was
	 *		reached from (or negative if start node). We do not search along
	 *		a connection to this index.
	 */
	bool getPathIndices(
		BW::vector<int> & pathIndices,
		const SuperModelNodeTree & tree,
		int inputIndex, int startIndex, int endIndex )
	{
		MF_ASSERT( (inputIndex < 0)
			||
			(inputIndex < tree.getNumNodes()) );
		MF_ASSERT( 0 <= startIndex );
		MF_ASSERT( startIndex < tree.getNumNodes() );
		MF_ASSERT( 0 <= endIndex );
		MF_ASSERT( endIndex < tree.getNumNodes() );

		const SuperModelNode & node = tree.getNode( startIndex );

		//For every potential connection to another node.
		for(int conn = 0; conn <= node.getNumChildren(); ++conn)
		{
			int connIndex = 0;
			if (conn == node.getNumChildren())
			{
				connIndex = node.getParentIndex();
				if (connIndex < 0)
				{
					continue;
				}
			}
			else
			{
				connIndex = node.getChild( conn );
			}
			if (connIndex == inputIndex)
			{
				continue;
			}

			//Check to see whether this node has a path to the target.
			if ((connIndex == endIndex)
				||
				getPathIndices(
					pathIndices, tree, startIndex, connIndex, endIndex ))
			{
				pathIndices.push_back( connIndex );
				return true;
			}
		}

		return false;
	}
}


/**
 *	@pre @a startIndex and @a endIndex are in the range
 *		[0, getNumNodes()).
 *	@post On true return: Found a path. Assigned to @a pathIndices the
 *			sequence of node indices starting at @a startIndex and proceeding
 *			to @a endIndex.
 *		On false: No path was found, contents of @a pathIndices is empty.
 *			This can only happen if @a startIndex and @a endIndex exist
 *			in separate unconnected trees.
 */
bool SuperModelNodeTree::getPathIndices( BW::vector<int> & pathIndices,
	int startIndex, int endIndex ) const
{
	MF_ASSERT( 0 <= startIndex );
	MF_ASSERT( startIndex < getNumNodes() );
	MF_ASSERT( 0 <= endIndex );
	MF_ASSERT( endIndex < getNumNodes() );

	if (startIndex == endIndex)
	{
		pathIndices.resize( 1 );
		pathIndices[0] = startIndex;
		return true;
	}

	pathIndices.clear();
	if (BW_NAMESPACE_NAME ::getPathIndices(
		pathIndices, *this, -1, endIndex, startIndex ))
	{
		pathIndices.push_back( endIndex );
		return true;
	}

	MF_ASSERT( pathIndices.empty() );
	return false;
}


namespace
{
	/**
	 *	Local helper for SuperModelNodeTree::desc.
	 *	@return Num printed nodes.
	 */
	int printNodeHierarchy( BW::ostringstream & outputStream,
		const SuperModelNodeTree & tree,
		const SuperModelNode & node, int numLineTabs )
	{
		BW_GUARD;

		MF_ASSERT( numLineTabs >= 0 );
		MF_ASSERT( tree.getNodeIndex( node.getIdentifier() ) >= 0 );

		std::ostream_iterator<BW::string::value_type> tabWriter( outputStream );
		std::fill_n( tabWriter, numLineTabs, BW::string::value_type( '\t' ) );

		outputStream << "identifier=" << node.getIdentifier() << "; "
			"parentIndex=" << node.getParentIndex() << "; "
			"numChildren=" << node.getNumChildren() << "\n";

		int result = 1;

		for(int childIndex = 0;
			childIndex < node.getNumChildren();
			++childIndex)
		{
			result += printNodeHierarchy(
				outputStream,
				tree,
				tree.getNode( node.getChild( childIndex ) ),
				numLineTabs + 1);
		}

		return result;
	}
}


/**
 *	@return A description (intended to be used only for debug purposes)
 *		of this tree.
 */
BW::string SuperModelNodeTree::desc() const
{
	BW_GUARD;

	BW::ostringstream resultStream;
	resultStream << "SuperModelNodeTree:"
		"\tnumNodes=" << getNumNodes() << "\n"
		"\tflat node list:\n";
	
	for (int nodeIndex = 0; nodeIndex < getNumNodes(); ++nodeIndex)
	{
		resultStream << "\t\t" << nodeIndex << "->" <<
			getNode( nodeIndex ).desc() << "\n";
	}

	resultStream << "\tnode hierarchies:\n";
	int numSeparateHierarches = 0;
	int numPrintedNodes = 0;
	for (NodeIdToIndexMap::const_iterator itr = nodeLookup_.begin();
		itr != nodeLookup_.end();
		++itr)
	{
		const SuperModelNode & node = getNode( itr->second );
		
		MF_ASSERT( node.getIdentifier() == itr->first );

		if (node.getParentIndex() < 0)
		{
			resultStream << "\t\thierarchy " << numSeparateHierarches << ":\n";
			numPrintedNodes += printNodeHierarchy(
				resultStream, *this, node, 3 );
			++numSeparateHierarches;
		}
	}
	resultStream << "})";

	MF_ASSERT( numPrintedNodes == getNumNodes() );

	return resultStream.str();
}


///Internal non-const version of getNode.
SuperModelNode & SuperModelNodeTree::getNode( int index )
{
	BW_GUARD;

	//Call const version of the function.
	return const_cast<SuperModelNode &>(
		const_cast<const SuperModelNodeTree *>(this)->getNode( index ) );
}


/**
 *	Internal helper for addAllNodes that is called recursively to add nodes
 *	into the tree.
 *	
 *	@pre @a nodeToAdd has no parent if @a nodeParentIndex is negative.
 *	@pre A node with the same identifier as @a nodeToAdd exists in
 *		the Moo::NodeCatalogue.
 *	@post On true return: added @a nodeToAdd and all children recursively
 *			to this tree structure.
 *		On false return: incompatible subtree. The state of the structure is
 *			indeterminate. It shouldn't be used now because the data has not
 *			met the necessary requirements.
 */
bool SuperModelNodeTree::addAllNodesInternal(
	const Moo::Node & nodeToAdd, int nodeParentIndex )
{
	BW_GUARD;

	MF_ASSERT( (nodeParentIndex >= 0) == nodeToAdd.parent().hasObject() );
	MF_ASSERT( nodeParentIndex < getNumNodes() );

	int nodeToAddTreeIndex = getNodeIndex( nodeToAdd.identifier() );
	bool addToParent = false;
	if (nodeToAddTreeIndex < 0)
	{
		//nodeToAdd doesn't exist in the tree yet.
		
		nodeToAddTreeIndex = static_cast<int>(nodes_.size());
		nodeLookup_.insert(std::pair<BW::string, int>(
			nodeToAdd.identifier(), nodeToAddTreeIndex ) );
		nodes_.push_back( SuperModelNode( nodeToAdd.identifier() ) );
		nodes_.back().setParentIndex( nodeParentIndex );

		MF_ASSERT( nodeLookup_.size() == nodes_.size() );
		MF_ASSERT( nodeToAddTreeIndex < getNumNodes() );

		//Check that any children it has that already exist in the tree have
		//no parent.
		for (uint32 nodeToAddChildIndex = 0;
			nodeToAddChildIndex < nodeToAdd.nChildren();
			++nodeToAddChildIndex)
		{
			const Moo::ConstNodePtr nodeToAddChild =
				nodeToAdd.child( nodeToAddChildIndex );
			MF_ASSERT( nodeToAddChild.hasObject() );

			const int treeChildIndex = getNodeIndex(
				nodeToAddChild->identifier() );

			if ((treeChildIndex >= 0) &&
				(getNode( treeChildIndex ).getParentIndex() >= 0))
			{
#if SMNT_DEBUG
				DEBUG_MSG( "SuperModelNodeTree: incompatible for new node, "
					"child already exists with a current parent.\n" );
#endif
				return false;
			}
		}
	}
	else //A pre-existing node has the same identifier as nodeToAdd.
	{
		//We need to check that nodeToAdd's children are either not in the
		//tree already or their parent is this pre-existing tree node (or no
		//parent).
		for (uint32 nodeToAddChildIndex = 0;
			nodeToAddChildIndex < nodeToAdd.nChildren();
			++nodeToAddChildIndex)
		{
			const Moo::ConstNodePtr nodeToAddChild =
				nodeToAdd.child( nodeToAddChildIndex );
			MF_ASSERT( nodeToAddChild.hasObject() );

			const int treeChildIndex = getNodeIndex(
				nodeToAddChild->identifier() );

			if (treeChildIndex >= 0)
			{
				const int treeChildParentIndex =
					getNode( treeChildIndex ).getParentIndex();

				if (treeChildParentIndex >= 0)
				{
					if (treeChildParentIndex != nodeToAddTreeIndex)
					{
#if SMNT_DEBUG
						DEBUG_MSG( "SuperModelNodeTree: incompatible for "
							"add of existing node, child already exists with "
							"a different parent.\n" );
#endif
						return false;
					}
				}
				else
				{
					//We're adding a child to this node that already exists
					//on the graph and has no parent. We check that we're not
					//adding a cycle.
					if (isDescendantOf( treeChildIndex, nodeToAddTreeIndex ))
					{
#if SMNT_DEBUG
						DEBUG_MSG( "SuperModelNodeTree: incompatible for "
							"add of existing node, this would add a cycle to "
							"the graph.\n" );
#endif
						return false;
					}
				}
			}
		}

		//Set parent id if required.
		if (nodeParentIndex >= 0)
		{
			if (getNode( nodeToAddTreeIndex ).getParentIndex() < 0)
			{
				getNode( nodeToAddTreeIndex ).setParentIndex( nodeParentIndex );
			}
			else
			{
				//Pre-existing node parent assignment should be the same.
				if ( getNode( nodeToAddTreeIndex ).getParentIndex() !=
					nodeParentIndex )
				{
#if SMNT_DEBUG
					DEBUG_MSG( "SuperModelNodeTree: incompatible for new node, "
						"node already exists with a different parent.\n" );
#endif
					return false;
				}

				//This pre-existing node should already be added to its parent.
				MF_ASSERT(
					findChild( nodeParentIndex, nodeToAdd.identifier() ) ==
					nodeToAddTreeIndex );
			}
		}
	}

	MF_ASSERT( nodeToAddTreeIndex >= 0 );
	MF_ASSERT( getNodeIndex( nodeToAdd.identifier() ) == nodeToAddTreeIndex );
	MF_ASSERT( (nodeParentIndex < 0) ||
		(getNode( nodeToAddTreeIndex ).getParentIndex() == nodeParentIndex ));

	//Call add on each of the children (passing parent index), in turn,
	//adding them to this node if it didn't already have them.
	for (uint32 nodeToAddChildIndex = 0;
		nodeToAddChildIndex < nodeToAdd.nChildren();
		++nodeToAddChildIndex)
	{
		const Moo::ConstNodePtr nodeToAddChild =
			nodeToAdd.child( nodeToAddChildIndex );
		MF_ASSERT( nodeToAddChild.hasObject() );

		const bool needToAdd =
			(findChild( nodeToAddTreeIndex, nodeToAddChild->identifier() ) < 0);
		if (!addAllNodesInternal( *nodeToAddChild, nodeToAddTreeIndex ))
		{
			return false;
		}

		if (needToAdd)
		{
			const int childTreeIndex =
				getNodeIndex( nodeToAddChild->identifier() );
			MF_ASSERT( childTreeIndex >= 0 );

			getNode( nodeToAddTreeIndex ).addChild( childTreeIndex );
		}
	}

	return true;
}

BW_END_NAMESPACE
