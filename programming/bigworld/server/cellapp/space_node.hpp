#ifndef SPACE_NODE_HPP
#define SPACE_NODE_HPP

#include "cstdmf/binary_stream.hpp"
#include "math/rectt.hpp"


BW_BEGIN_NAMESPACE

class CellInfo;
class CellInfoVisitor;


/**
 *	This base class is used to represent the nodes in the BSP tree that is
 *	used to partition a space.
 */
class SpaceNode
{
public:
	virtual void deleteTree() = 0;

	virtual const CellInfo * pCellAt( float x, float z ) const = 0;
	virtual void visitRect(
			const BW::Rect & rect, CellInfoVisitor & visitor ) = 0;
	virtual void addToStream( BinaryOStream & stream ) const = 0;

protected:
	virtual ~SpaceNode() {};
};

BW_END_NAMESPACE

#endif // SPACE_NODE_HPP
