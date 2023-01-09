
#include "pch.hpp"
#include "edge.hpp"
#include "node.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


/**
 *	Constructor.
 *
 *	@param start	Starting node.
 *	@param end		Ending node.
 */
Edge::Edge( const NodePtr & start, const NodePtr & end ) :
	start_( start ),
	end_( end )
{
}


/**
 *	Destructor.
 */
Edge::~Edge()
{
}


} // namespace Graph
BW_END_NAMESPACE

