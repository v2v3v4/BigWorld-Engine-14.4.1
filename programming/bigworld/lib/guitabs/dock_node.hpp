
#ifndef GUITABS_DOCK_NODE_HPP
#define GUITABS_DOCK_NODE_HPP


BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *  This abstract class is the base that all node objects inherit from. It
 *  includes default implementation for some methods, giving the a default
 *  behaviour of a tree leaf for classes that don't override these methods.
 */
class DockNode : public ReferenceCount
{
public:
	virtual void setLeftChild( DockNodePtr child );
	virtual void setRightChild( DockNodePtr child );

	virtual DockNodePtr getLeftChild();
	virtual DockNodePtr getRightChild();

	virtual bool isLeaf();
	virtual bool hitTest( int x, int y );
	virtual Orientation getSplitOrientation();

	// Each node must return its CWnd pointer.
	virtual CWnd* getCWnd() = 0;

	virtual bool isVisible();
	virtual bool isExpanded();
	virtual void setParentWnd( CWnd* parent );
	virtual bool adjustSizeToNode( DockNodePtr newNode, bool nodeIsNew = true );
	virtual void recalcLayout();
	virtual void getPreferredSize( int& w, int& h );

	virtual bool getNodeByWnd( CWnd* ptr, DockNodePtr& childNode, DockNodePtr& parentNode );
	virtual DockNodePtr getNodeByPoint( int x, int y );

	virtual void destroy();

	// Also, all nodes must implement a load and a save method for serialising.
	virtual bool load( DataSectionPtr section, CWnd* parent, int wndID ) = 0;
	virtual bool save( DataSectionPtr section ) = 0;

};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_DOCK_NODE_HPP