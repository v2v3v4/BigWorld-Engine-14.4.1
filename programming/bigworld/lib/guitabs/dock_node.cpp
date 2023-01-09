
#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	This method sets the left child node of this node.
 *	This defualt implementation makes sure leaf nodes are not used as nodes
 *	that have children.
 *
 *	@param child	New child.
 */
void DockNode::setLeftChild( DockNodePtr child )
{
	BW_GUARD;

	ASSERT( 0 );
}


/**
 *	This method sets the right child node of this node.
 *	This defualt implementation makes sure leaf nodes are not used as nodes
 *	that have children.
 *
 *	@param child	New child.
 */
void DockNode::setRightChild( DockNodePtr child )
{
	BW_GUARD;

	ASSERT( 0 );
}


/**
 *	This method returns the left child node of this node.
 *	This defualt implementation makes sure leaf nodes are not used as nodes
 *	that have children.
 *
 *	@return	The left child of this node.
 */
DockNodePtr DockNode::getLeftChild()
{
	BW_GUARD;

	ASSERT( 0 );
	return 0;
}


/**
 *	This method returns the left child node of this node.
 *	This defualt implementation makes sure leaf nodes are not used as nodes
 *	that have children.
 *
 *	@return	The right child of this node.
 */
DockNodePtr DockNode::getRightChild()
{
	BW_GUARD;

	ASSERT( 0 );
	return 0;
}


/**
 *	This method returns whether or not the node is a leaf.
 *
 *	@return	True if the node is a childless leaf node, false otherwise.
 */
bool DockNode::isLeaf()
{
	return true;
}


/**
 *	This method returns whether or not the specified point is contained within
 *	this node's rectangle.
 *
 *	@param x	X coordinate of the point to test.
 *	@param y	Y coordinate of the point to test.
 *	@return	True if the node contains the point x,y, false if not.
 */
bool DockNode::hitTest( int x, int y )
{
	BW_GUARD;

	CRect rect;

	getCWnd()->GetWindowRect(&rect);

	return ( rect.PtInRect( CPoint( x, y ) ) == TRUE );
}


/**
 *	This method returns a node's orientation, which is only meaningful for 
 *	splitter nodes.
 *
 *	@return	Node's orientation, or UNDEFINED_ORIENTATION if the node doesn't
 *			have an orientation.
 */
Orientation DockNode::getSplitOrientation()
{
	return UNDEFINED_ORIENTATION;
}


/**
 *	This method returns whether or not the node is visible.
 *
 *	@return	True if the node is visible, false if not.
 */
bool DockNode::isVisible()
{
	BW_GUARD;

	return ( getCWnd()->GetStyle() & WS_VISIBLE ) > 0;
}


/**
 *	This method returns whether or not the node is expanded, as opposed to 
 *	rolled up.
 *
 *	@return	True if the node is expanded, false if not.
 */
bool DockNode::isExpanded()
{
	return true;
}


/**
 *	This method adjustes the size of the node to allow for room to fit in the
 *	node specified in the "newNode" param.
 *
 *	@param newNode	Node to adjust this node's size.
 *	@param nodeIsNew	True if the new is a new node, false if not.
 *	@return	True if the node was found, false if not.
 */
bool DockNode::adjustSizeToNode( DockNodePtr newNode, bool nodeIsNew )
{
	if ( newNode == this )
		return true;
	else
		return false;
}


/**
 *	This method recalculates the layout of the subtree, making sure all the
 *	node sizes get recursively adjusted properly.
 */
void DockNode::recalcLayout()
{
	return;
}


/**
 *	This method sets the parent window of this node.
 *
 *	@param parent	New parent for the node.
 */
void DockNode::setParentWnd( CWnd* parent )
{
	BW_GUARD;

	getCWnd()->SetParent( parent );
}


/**
 *	This method is used to ask the node what its preferred default size is.
 *
 *	@param w	Return param, the default preferred width for the node.
 *	@param h	Return param, the default preferred height for the node.
 */
void DockNode::getPreferredSize( int& w, int& h )
{
	w = 0;
	h = 0;
}


/**
 *	This method searches a subtree of nodes for a node by CWnd.
 *
 *	@param ptr	CWnd corresponding to the desired node.
 *	@param childNode	Return param, the node (panel or splitter) matching the
 *						CWnd pointer.
 *	@param parentNode	Return param, the parent of the node.
 *	@return		True if found, false if not.
 */
bool DockNode::getNodeByWnd( CWnd* ptr, DockNodePtr& childNode, DockNodePtr& parentNode )
{
	BW_GUARD;

	if ( getCWnd() == ptr )
	{
		parentNode = 0;
		childNode = this;
		return true;
	}

	return false;
}


/**
 *	This method searches a subtree of nodes for a node that contains a point.
 *
 *	@param x	Screen X coordinate.
 *	@param y	Screen Y coordinate.
 *	@return		The dock node under the screen position x,y.
 */
DockNodePtr DockNode::getNodeByPoint( int x, int y )
{
	BW_GUARD;

	if ( hitTest( x, y ) )
	{
		return this;
	}
	return 0;
}


/**
 *	This method is called when destroying this subtree of nodes.
 */
void DockNode::destroy()
{
	return;
}


}	// namespace
BW_END_NAMESPACE

