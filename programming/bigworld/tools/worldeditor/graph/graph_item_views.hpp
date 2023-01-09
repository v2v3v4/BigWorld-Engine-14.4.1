#ifndef GRAPH_ITEM_VIEWS_HPP
#define GRAPH_ITEM_VIEWS_HPP


#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{

// Forward declarations.
class Graph;
typedef SmartPointer< Graph > GraphPtr;


class Node;
typedef SmartPointer< Node > NodePtr;


class Edge;
typedef SmartPointer< Edge > EdgePtr;


/**
 *	This abstract class specifies a common interface for drawable Graph View
 *	objects.
 */
class CommonGraphViewInterface
{
public:
	virtual const CRect & rect() const = 0;

	virtual int zOrder() const { return 0; }

	virtual int alpha() const { return 255; }

	virtual bool hitTest( const CPoint & pt ) const { return rect().PtInRect( pt ) ? true : false; }

	virtual void leftBtnDown( const CPoint & pt ) {}

	virtual void leftClick( const CPoint & pt ) {}

	virtual void rightClick( const CPoint & pt ) {}

	virtual void doDrag( const CPoint & pt ) {}

	virtual void endDrag( const CPoint & pt, bool canceled = false ) {}
};


/**
 *	This abstract class specifies the interface for an object that represents
 *	a node in the Graph View.
 */
class NodeView : public CommonGraphViewInterface, public ReferenceCount
{
public:
	typedef uint32 STATE;
	static const uint32 NORMAL = 0;
	static const uint32 SELECTED = 1;
	static const uint32 HOVER = 2;

	NodeView() {}
	virtual ~NodeView() {}

	virtual void position( const CPoint & pos ) = 0;

	virtual void draw( CDC & dc, uint32 frame, STATE state ) = 0;
};

typedef SmartPointer< NodeView > NodeViewPtr;


/**
 *	This abstract class specifies the interface for an object that represents
 *	an edge in the Graph View.
 */
class EdgeView : public CommonGraphViewInterface, public ReferenceCount
{
public:
	EdgeView() {}
	virtual ~EdgeView() {}

	virtual void draw( CDC & dc, uint32 frame, const CRect & rectStartNode, const CRect & rectEndNode ) = 0;
};

typedef SmartPointer< EdgeView > EdgeViewPtr;


} // namespace Graph

BW_END_NAMESPACE

#endif // GRAPH_ITEM_VIEWS_HPP
