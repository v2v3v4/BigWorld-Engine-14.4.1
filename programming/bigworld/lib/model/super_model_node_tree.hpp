#pragma once
#ifndef SUPER_MODEL_NODE_TREE_HPP
#define SUPER_MODEL_NODE_TREE_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "moo/node.hpp"
#include "moo/moo_math.hpp"

BW_BEGIN_NAMESPACE

class SuperModel;


/**
 *	This stores the data required for a Node in the SuperModelNodeTree class.
 */
class SuperModelNode
{
public:
	explicit SuperModelNode( const BW::string & identifier );

	const BW::string & getIdentifier() const;
	Moo::NodePtr & getMooNode();
	const Moo::NodePtr & getMooNode() const;

	int getParentIndex() const;
	void setParentIndex( int parentIndex );

	int getNumChildren() const;
	void addChild( int treeIndex );
	int getChild( int childIndex ) const;

	BW::string desc() const;

private:
	///The identifier for this node.
	BW::string identifier_;
	///Cached Moo::Node from the NodeCatalogue. This is only necessary until
	///SuperModel can be updated to set the transform directly in the output
	///transforms array (generated during dressing). For now it makes it so we
	///can look up a node by index for assigning its transform.
	Moo::NodePtr cachedMooNode_;
	///The index of the parent for this node in the owning SuperModelTree (or
	///negative if no parent).
	int parentIndex_;

	///Stores all indices of the child nodes in the owning SuperModelTree.
	BW::vector<int> childNodes_;
};


/**
 *	This class defines the node hierarchy for a SuperModel.
 *	
 *	This is required even though we can search for a Moo::Node (in the
 *	NodeCatalogue, or using findNode on the SuperModel instance), and those
 *	nodes include parent/child relationships. This is because (due to the
 *	sharing via NodeCatalogue) the child/parent relationships for the Moo::Node
 *	returned may not be the ones used for the particular mesh we're using.
 *	For instance, if there are two skeletons that have a node named "head",
 *	they will share the same node in NodeCatalogue (whichever happens to be
 *	loaded first). It is the node in NodeCatalogue that gets its transforms
 *	updated during the animation processing. This situation is problematic
 *	if we need to traverse the graph (for IK, for example) because it means we
 *	need to use something other than the usual Node from NodeCatalogue.
 *	
 *	This class is used by the SuperModel (once it has loaded all its models)
 *	to create a node tree that can then be used by users of the SuperModel to
 *	traverse the hierarchy.
 *	Note that within this class node identifiers are unique. When a new node
 *	subtree is added using addAllNodes the structure is overlaid and merged
 *	with the current tree. The addition will fail if any node in the subtree
 *	getting added already exists in this tree but has a different parent (unless
 *	either of them have no parent). This means that this structure will only be
 *	valid for SuperModels where the constituent models have non-intersecting
 *	node trees, they all have the same node tree, and/or each possesses
 *	subsections of a node tree with unique identifier names. Cycles are also
 *	disallowed.
 *	
 *	To modify node transforms, see the SuperModel member functions
 *	(get/set)Node(World/Relative)Transform.
 */
class SuperModelNodeTree
{
public:
	bool addAllNodes( const Moo::Node & rootFromVisual );
	int getNumNodes() const;
	int getNodeIndex( const BW::string & identifier ) const;
	const SuperModelNode & getNode( int index ) const;

	int findChild( int nodeIndex, const BW::string & identifier ) const;
	int getNumRootNodes() const;
	bool isDescendantOf( int parentIndex, int queryIsDescendantIndex ) const;
	bool getPathIndices( BW::vector<int> & pathIndices,
		int startIndex, int endIndex ) const;

	BW::string desc() const;
	void draw( const SuperModel & superModel,
		Moo::PackedColour baseColour = Moo::PackedColours::MAGENTA,
		float drawSize = 1.0f ) const;

private:
	SuperModelNode & getNode( int index );
	bool addAllNodesInternal(
		const Moo::Node & nodeToAdd, int nodeParentIndex );

	///Stores all nodes in the tree.
	BW::vector<SuperModelNode> nodes_;

	typedef BW::unordered_map<BW::string, int> NodeIdToIndexMap;
	///Stores a lookup from node identifier to its index in nodes_.
	NodeIdToIndexMap nodeLookup_;
};

BW_END_NAMESPACE

#endif // SUPER_MODEL_NODE_TREE_HPP
