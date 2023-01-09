#ifndef NODE_HPP
#define NODE_HPP


#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


// Forward declarations.
class Node;
typedef SmartPointer< Node > NodePtr;


/**
 *	This class stores info about an node in a Graph. In most cases it will be
 *	derived by application-specific edge classes.
 */
class Node : public ReferenceCount
{
public:
	Node();
	virtual ~Node();

private:
	Node( const Node & other );
};


} // namespace Graph

BW_END_NAMESPACE

#endif // NODE_HPP
