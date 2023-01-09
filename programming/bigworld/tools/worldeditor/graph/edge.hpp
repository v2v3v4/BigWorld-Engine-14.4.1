#ifndef EDGE_HPP
#define EDGE_HPP


#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


// Forward declarations.
class Node;
typedef SmartPointer< Node > NodePtr;


class Edge;
typedef SmartPointer< Edge > EdgePtr;


/**
 *	This class stores info about an edge between two nodes of a Graph. In most
 *	cases it will be derived by application-specific edge classes.
 */
class Edge : public ReferenceCount
{
public:
	Edge( const NodePtr & start, const NodePtr & end );
	virtual ~Edge();

	virtual NodePtr start() const { return start_; }
	virtual void start( const NodePtr & node ) { start_ = node; }

	virtual NodePtr end() const { return end_; }
	virtual void end( const NodePtr & node ) { end_ = node; }

private:
	Edge();
	Edge( const Edge & other );

	NodePtr start_;
	NodePtr end_;
};


} // namespace Graph

BW_END_NAMESPACE

#endif // EDGE_HPP
