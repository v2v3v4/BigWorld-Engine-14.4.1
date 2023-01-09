#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "range_list_node.hpp"

#include <cstddef>
#include <math.h>

#include "entity.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Cell", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RangeListNode
// -----------------------------------------------------------------------------

/**
 *	This method makes sure that this entity is in the correct place in the
 *	global sorted position lists. If a shuffle is performed, crossedX or
 *	crossedZ is called on both nodes.
 *
 *	@param oldX The old value of the x position.
 *	@param oldZ The old value of the z position.
 */
void RangeListNode::shuffleXThenZ( float oldX, float oldZ )
{
	this->shuffleX( oldX, oldZ );
	this->shuffleZ( oldX, oldZ );
	// this->shuffleZ( this->x() );
}


/**
 *	This method makes sure that this entity is in the correct place in X
 *	position lists. If a shuffle is performed, crossedX is called on both nodes.
 *
 *	@param oldX The old value of the x position.
 *	@param oldZ The old value of the z position.
 */
void RangeListNode::shuffleX( float oldX, float oldZ )
{
	MF_ASSERT( !Entity::callbacksPermitted() );
	static bool inShuffle = false;
	MF_ASSERT( !inShuffle );	// make sure we are not reentrant
	inShuffle = true;

	float ourPosX = this->x();
	float othPosX;

	// Shuffle to the left(negative X)...
	while (pPrevX_ != NULL &&
			(ourPosX < (othPosX = pPrevX_->x()) ||
				(isEqual( ourPosX, othPosX ) &&
				order_ <= pPrevX_->order_)))
	{
		if (this->wantsCrossingWith( pPrevX_ ))
		{
			this->crossedX( pPrevX_, true, pPrevX_->x(), pPrevX_->z() );
		}

		if (pPrevX_->wantsCrossingWith( this ))
		{
			pPrevX_->crossedX( this, false, oldX, oldZ );
		}

		// unlink us
		if (pNextX_!= NULL)
		{
			pNextX_->pPrevX_ = pPrevX_;
		}

		pPrevX_->pNextX_ = pNextX_;

		// fix our pointers
		pNextX_ = pPrevX_;
		pPrevX_ = pPrevX_->pPrevX_;

		// relink us
		if (pPrevX_ != NULL)
		{
			pPrevX_->pNextX_= this;
		}

		pNextX_->pPrevX_= this;
	}

	// Shuffle to the right(positive X)...
	while (pNextX_ != NULL &&
			(ourPosX > (othPosX = pNextX_->x()) ||
				(isEqual( ourPosX, othPosX ) &&
				order_ >= pNextX_->order_)))
	{
		if (this->wantsCrossingWith( pNextX_ ))
		{
			this->crossedX( pNextX_, false, pNextX_->x(), pNextX_->z() );
		}

		if (pNextX_->wantsCrossingWith( this ))
		{
			pNextX_->crossedX( this, true, oldX, oldZ );
		}

		// unlink us
		if (pPrevX_ != NULL)
		{
			pPrevX_->pNextX_ = pNextX_;
		}

		pNextX_->pPrevX_ = pPrevX_;
		// fix our pointers
		pPrevX_ = pNextX_;
		pNextX_ = pNextX_->pNextX_;

		// relink us
		pPrevX_->pNextX_ = this;

		if (pNextX_ != NULL)
		{
			pNextX_->pPrevX_ = this;
		}
	}

	inShuffle = false;
}


/**
 *	This method makes sure that this entity is in the correct place in Z
 *	position lists. If a shuffle is performed, crossedZ is called on both nodes.
 */
void RangeListNode::shuffleZ( float oldX, float oldZ )
{
	MF_ASSERT( !Entity::callbacksPermitted() );
	static bool inShuffle = false;
	MF_ASSERT( !inShuffle );	// make sure we are not reentrant
	inShuffle = true;

	float ourPosZ = this->z();
	float othPosZ;

	while (pPrevZ_ != NULL &&
			(ourPosZ < (othPosZ = pPrevZ_->z()) ||
				(isEqual( ourPosZ, othPosZ ) &&
				order_ <= pPrevZ_->order_)))
	{
		if (this->wantsCrossingWith( pPrevZ_ ))
		{
			this->crossedZ( pPrevZ_, true, pPrevZ_->x(), pPrevZ_->z() );
		}

		if (pPrevZ_->wantsCrossingWith( this ))
		{
			pPrevZ_->crossedZ( this, false, oldX, oldZ );
		}

		// unlink us
		if (pNextZ_!= NULL)
		{
			pNextZ_->pPrevZ_ = pPrevZ_;
		}

		pPrevZ_->pNextZ_ = pNextZ_;

		// fix our pointers
		pNextZ_ = pPrevZ_;
		pPrevZ_ = pPrevZ_->pPrevZ_;

		// relink us
		if (pPrevZ_ != NULL)
		{
			pPrevZ_->pNextZ_= this;
		}

		pNextZ_->pPrevZ_= this;
	}

	// Shuffle to the right(positive Z)...
	while (pNextZ_ != NULL &&
			(ourPosZ > (othPosZ = pNextZ_->z()) ||
				(isEqual( ourPosZ, othPosZ ) &&
				order_ >= pNextZ_->order_)))
	{
		if (this->wantsCrossingWith( pNextZ_ ))
		{
			this->crossedZ( pNextZ_, false, pNextZ_->x(), pNextZ_->z() );
		}

		if (pNextZ_->wantsCrossingWith( this ))
		{
			pNextZ_->crossedZ( this, true, oldX, oldZ );
		}

		// unlink us
		if (pPrevZ_ != NULL)
		{
			pPrevZ_->pNextZ_ = pNextZ_;
		}

		pNextZ_->pPrevZ_ = pPrevZ_;

		// fix our pointers
		pPrevZ_ = pNextZ_;
		pNextZ_ = pNextZ_->pNextZ_;

		// relink us
		pPrevZ_->pNextZ_ = this;

		if (pNextZ_ != NULL)
		{
			pNextZ_->pPrevZ_ = this;
		}
	}

	inShuffle = false;
}


/**
 *	This method removes the node from the range list. It attaches its previous
 *	node to its next node. It does not delete itself.
 */
void RangeListNode::removeFromRangeList()
{
	if (pPrevX_!=NULL) pPrevX_->pNextX_ = pNextX_;
	if (pNextX_!=NULL) pNextX_->pPrevX_ = pPrevX_;

	if (pPrevZ_!=NULL) pPrevZ_->pNextZ_ = pNextZ_;
	if (pNextZ_!=NULL) pNextZ_->pPrevZ_ = pPrevZ_;

	pPrevX_ = NULL;
	pNextX_ = NULL;
	pPrevZ_ = NULL;
	pNextZ_ = NULL;
}


/**
 *	This method inserts a range node before another one in the X list.
 *
 *	@param pNode The node to insert before.
 */
void RangeListNode::insertBeforeX( RangeListNode * pNode )
{
	MF_ASSERT( this != pNode );

	if (pPrevX_!=NULL)
	{
		pPrevX_->pNextX_ = pNode;
	}

	pNode->pPrevX_ = pPrevX_;

	this ->pPrevX_ = pNode;
	pNode->pNextX_ = this;
}


/**
 *	This method inserts a range node before another one in the Z list.
 *
 *	@param pNode The node to insert before.
 */
void RangeListNode::insertBeforeZ( RangeListNode * pNode )
{
	MF_ASSERT( this != pNode );

	if (pPrevZ_!=NULL)
	{
		pPrevZ_->pNextZ_ = pNode;
	}

	pNode->pPrevZ_ = pPrevZ_;

	this ->pPrevZ_ = pNode;
	pNode->pNextZ_ = this;
}


/**
 *	This debugging method prints out information about the node in the X list.
 */
void RangeListNode::debugRangeX() const
{
	DEBUG_MSG( "RangeListNode::debugX(%p): "
					"me.PosX=%10.7f, me=[%s], prevX=[%s], nextX=[%s], "
					"wants = %x, makes = %x\n",
				this, this->x(),
				this->debugString().c_str(),
				(pPrevX_==NULL ? "NULL" : pPrevX_->debugString().c_str()),
				(pNextX_==NULL ? "NULL" : pNextX_->debugString().c_str()),
				wantsFlags_, makesFlags_ );
}


/**
 *	This debugging method prints out information about the node in the Z list.
 */
void RangeListNode::debugRangeZ() const
{
	DEBUG_MSG( "RangeListNode::debugZ(%p): "
					"me.PosZ=%10.7f, me=[%s], prevZ=[%s], nextZ=[%s], "
					"wants = %x, makes = %x\n",
				this, this->z(),
				this->debugString().c_str(),
				(pPrevZ_==NULL ? "NULL" : pPrevZ_->debugString().c_str()),
				(pNextZ_==NULL ? "NULL" : pNextZ_->debugString().c_str()),
				wantsFlags_, makesFlags_ );
}

BW_END_NAMESPACE

// range_list_node.cpp
