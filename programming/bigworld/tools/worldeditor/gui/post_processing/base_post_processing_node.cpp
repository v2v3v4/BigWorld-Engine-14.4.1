
#include "pch.hpp"
#include "base_post_processing_node.hpp"
#include "node_callback.hpp"

BW_BEGIN_NAMESPACE

//	Base post processing node statics 
/*static*/ bool BasePostProcessingNode::s_changed_ = false;
/*static*/ bool BasePostProcessingNode::s_previewMode_ = false;
/*static*/ int BasePostProcessingNode::s_chainPosCounter_ = 0;


/**
 *	Constructor.
 *
 *	@param callback	Object that wishes to handle messages sent by a node.
 */
BasePostProcessingNode::BasePostProcessingNode( NodeCallback * callback ) :
	callback_( callback ),
	active_( true ),
	dragging_( false ),
	chainPos_( s_chainPosCounter_++ )
{
	BW_GUARD;
}


/**
 *	Denstructor.
 */
BasePostProcessingNode::~BasePostProcessingNode()
{
	BW_GUARD;
}


/**
 *	This method returns whether or not the node is active.
 *
 *	@return Whether or not the node is active.
 */
bool BasePostProcessingNode::active() const
{
	BW_GUARD;

	return active_;
}


/**
 *	This method sets whether or not the node is active and tells the callback
 *	object.
 *
 *	@param active Whether or not the node is active.
 */
void BasePostProcessingNode::active( bool active )
{
	BW_GUARD;

	changed( true );

	activeNoCallback( active );

	if (callback_)
	{
		callback_->nodeActive( this, active );
	}
}


/**
 *	This internal method sets whether or not the node is active without
 *	notifying the callback object.
 *
 *	@param active Whether or not the node is active.
 */
void BasePostProcessingNode::activeNoCallback( bool active )
{
	BW_GUARD;

	active_ = active;
}

BW_END_NAMESPACE

