
#ifndef GUITABS_SPLITTER_NODE_HPP
#define GUITABS_SPLITTER_NODE_HPP

BW_BEGIN_NAMESPACE

namespace GUITABS
{

/**
 *  This class represents a splitter window in the dock tree. It mantains a
 *	splitter window, its orientation (vertical or horizontal), and pointers to
 *  the child nodes, which can be of any class inherited from DockNode.
 *  Additionally, this class switches from a splitter window to a single
 *  container window when one of the children is not visible.
 */
class SplitterNode : public DockNode, public SplitterEventHandler
{
public:
	SplitterNode();
	SplitterNode( Orientation dir, CWnd* parent, int wndID );
	void init( Orientation dir, CWnd* parent, int wndID );
	~SplitterNode();

	void setLeftChild( DockNodePtr child );
	void setRightChild( DockNodePtr child );

	void finishInsert( const CRect* destRect, int leftChildSize, int rightChildSize );

	DockNodePtr getLeftChild();
	DockNodePtr getRightChild();

	bool isLeaf();

	bool isExpanded();

	CWnd* getCWnd();
	
	bool adjustSizeToNode( DockNodePtr newNode, bool nodeIsNew );

	void recalcLayout();

	bool load( DataSectionPtr section, CWnd* parent, int wndID );
	bool save( DataSectionPtr section );

	Orientation getSplitOrientation();

	void setParentWnd( CWnd* parent );

	void getPreferredSize( int& w, int& h );

	bool getNodeByWnd( CWnd* ptr, DockNodePtr& childNode, DockNodePtr& parentNode );
	DockNodePtr getNodeByPoint( int x, int y );

	void destroy();

	// Event handler methods
	void resizeSplitter( int lastWidth, int lastHeight, int width, int height );

private:
	NiceSplitterWnd splitterWnd_;

	DockNodePtr leftChild_;
	DockNodePtr rightChild_;

	Orientation dir_;

	// These members are used to better resize panes by delaying the resize.
	int delayedLeftSize_;
	int delayedRightSize_;

	int getLeftSize();
	int getRightSize();
	void setLeftSize( int size );
	void setRightSize( int size );

	void resizeTreeDimension( Orientation dir, int lastSize, int size );
};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_SPLITTER_NODE_HPP