#ifdef _MSC_VER 
#pragma once
#endif

#ifndef NODE_TREE_HPP
#define NODE_TREE_HPP

#include "math/forward_declarations.hpp"
#include "moo/forward_declarations.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace{
	static const int NODE_STACK_SIZE = 128;
}

/**
 *	The NodeTreeData struct.
 */
struct NodeTreeData
{
	NodeTreeData( Moo::Node * pN, int nc ) : pNode( pN ), nChildren( nc ) {}
	Moo::Node	* pNode;
	int			nChildren;
};



typedef BW::vector<NodeTreeData>	NodeTree;



/**
 *	NodeTreeDataItem struct gives access to the current node data during 
 *	a node tree traversal (using NodeTreeIterator). The node data can be 
 *	accessed through the pData member pointer, A nodes's parent world 
 *	transformation matrix is available through the pParentTransform 
 *	member pointer. 
 */
struct NodeTreeDataItem
{
	const struct NodeTreeData *	pData;
	const Matrix *				pParentTransform;
};


/**
 *	Use the NodeTreeIterator class to traverse the model's node hierarchy 
 *	(skeleton). The class models a unidirectional iterator and allows 
 *	linear traversal of the node tree, providing access to each node in 
 *	the tree through the NodeTreeDataItem class (returned by the and -> 
 *	operators). See pymodel.cpp for and example of how to use the iterator.
 */
class NodeTreeIterator
{
public:
	NodeTreeIterator( 
		const NodeTreeData * begin, 
		const NodeTreeData * end, 
		const Matrix * root );

	const NodeTreeDataItem & operator*() const;
	const NodeTreeDataItem * operator->() const;

	void operator++( int );
	bool operator==( const NodeTreeIterator & other ) const;
	bool operator!=( const NodeTreeIterator & other ) const;

private:
	const NodeTreeData *	curNodeData_;
	const NodeTreeData *	endNodeData_;
	NodeTreeDataItem		curNodeItem_;
	int	sat_;

	struct 
	{
		const Matrix *	trans;
		int				pop;
	} stack_[NODE_STACK_SIZE];
};

BW_END_NAMESPACE


#endif // NODE_TREE_HPP
