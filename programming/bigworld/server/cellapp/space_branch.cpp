#include "space_branch.hpp"

#include "space.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
SpaceBranch::SpaceBranch( Space & space,
		const BW::Rect & rect,
		BinaryIStream & stream, bool isHorizontal ) :
	isHorizontal_( isHorizontal )
{
	stream >> position_;

	BW::Rect leftRect = rect;
	BW::Rect rightRect = rect;

	if (isHorizontal_)
	{
		leftRect.yMax_ = position_;
		rightRect.yMin_ = position_;
	}
	else
	{
		leftRect.xMax_ = position_;
		rightRect.xMin_ = position_;
	}

	pLeft_ = space.readTree( stream, leftRect );
	pRight_ = space.readTree( stream, rightRect );
}


/**
 *	Destructor.
 */
SpaceBranch::~SpaceBranch()
{
}


/**
 *	This method deletes this subtree.
 */
void SpaceBranch::deleteTree()
{
	pLeft_->deleteTree();
	pRight_->deleteTree();
	delete this;
}


/**
 *	This method return the CellInfo at the input point in this subtree.
 */
const CellInfo * SpaceBranch::pCellAt( float x, float z ) const
{
	const bool onLeft = ((isHorizontal_ ? z : x) < position_);

	const SpaceNode * pGoodSide  = onLeft ? pLeft_ : pRight_;
	return pGoodSide->pCellAt( x, z );
}


/**
 *	This method visits the cells that overlaps the input rectangle.
 */
void SpaceBranch::visitRect( const BW::Rect & rect, CellInfoVisitor & visitor )
{
	if (isHorizontal_)
	{
		if (rect.yMin_ <= position_)
		{
			pLeft_->visitRect( rect, visitor );
		}

		if (rect.yMax_ >= position_)
		{
			pRight_->visitRect( rect, visitor );
		}
	}
	else
	{
		if (rect.xMin_ <= position_)
		{
			pLeft_->visitRect( rect, visitor );
		}

		if (rect.xMax_ >= position_)
		{
			pRight_->visitRect( rect, visitor );
		}
	}
}


/**
 *	This method adds this subtree to the input stream.
 */
void SpaceBranch::addToStream( BinaryOStream & stream ) const
{
	stream << false << position_ << isHorizontal_;
	pLeft_->addToStream( stream );
	pRight_->addToStream( stream );
}

BW_END_NAMESPACE

// space_branch.cpp
