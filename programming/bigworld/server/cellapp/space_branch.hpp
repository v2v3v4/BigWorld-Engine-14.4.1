#ifndef SPACE_BRANCH_HPP

#include "space_node.hpp"


BW_BEGIN_NAMESPACE

class Space;


/**
 *	This class is used to represent an internal node of the BSP. It
 *	corresponds to a partitioning plane.
 */
class SpaceBranch : public SpaceNode
{
public:
	SpaceBranch( Space & space, const BW::Rect & rect,
			BinaryIStream & stream, bool isHorizontal );
	virtual ~SpaceBranch();
	virtual void deleteTree();

	virtual const CellInfo * pCellAt( float x, float z ) const;
	virtual void visitRect( const BW::Rect & rect, CellInfoVisitor & visitor );
	virtual void addToStream( BinaryOStream & stream ) const;

private:
	float	position_;
	bool	isHorizontal_;
	SpaceNode *	pLeft_;
	SpaceNode *	pRight_;
};

BW_END_NAMESPACE

#endif // SPACE_BRANCH_HPP
