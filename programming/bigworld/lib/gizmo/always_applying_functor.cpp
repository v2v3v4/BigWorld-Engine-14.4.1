#include "pch.hpp"
#include "always_applying_functor.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
AlwaysApplyingFunctor::AlwaysApplyingFunctor( bool allowedToDiscardChanges,
	PyTypeObject * pType ) :
	ToolFunctor( allowedToDiscardChanges, pType )
{
	BW_GUARD;
}


/**
 *	Update the constant apply functor.
 *	It should be applying in every update.
 *	@param dTime the change in time since last update.
 *	@param tool the tool.
 */
void AlwaysApplyingFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;

	MF_ASSERT( applying_ );

	// see if we want to commit this action
	if (this->checkStopApplying())
	{
		this->stopApplying( tool, true );
	}
	else
	{
		this->doApply( dTime, tool );
	}
}


/**
 *	Check if we should stop applying.
 *	@return true if we should stop applying.
 */
bool AlwaysApplyingFunctor::checkStopApplying() const
{
	BW_GUARD;
	// Left mouse button released
	return (!InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ));
}


/**
 *	Called when the tool becomes the currently used tool.
 *	@param tool
 */
void AlwaysApplyingFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
	this->beginApply();
}


/**
 *	Called when the tool ceases to be the currently used tool.
 *	@param tool
 */
void AlwaysApplyingFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;
	// Don't call stopApplying here as
	// onEndUsing -> stopApplying -> popTool -> onEndUsing *loop*
	MF_ASSERT( !this->applying() );
}


/**
 *	Called when we want to force the tool to stop.
 *	As this functor must be always applying, then this must be the top tool
 *	in the stack. The only way to stop is to pop the tool from the stack.
 *	When the tool manager pops the tool, onEndUsing will be called.
 *	Don't call stopApplying inside onEndUsing or it will loop.
 *	@param tool
 */
void AlwaysApplyingFunctor::stopApplying( Tool & tool, bool saveChanges )
{
	BW_GUARD;

	// Save changes unless the force-discard flag is set
	if (saveChanges || !this->isAllowedToDiscardChanges())
	{
		this->stopApplyCommitChanges( tool, true );
	}
	else
	{
		this->stopApplyDiscardChanges( tool );
	}

	// and this tool's job is over
	ToolManager::instance().popTool();
}


/**
 *	Key event method.
 *	@param event key event.
 *	@param tool the tool.
 *	@return true if handled.
 */
bool AlwaysApplyingFunctor::handleKeyEvent( const KeyEvent & event, Tool& tool )
{
	BW_GUARD;

	if (!event.isKeyDown())
	{
		return false;
	}

	if (event.key() == KeyCode::KEY_ESCAPE)
	{
		// Don't keep changes
		this->stopApplying( tool, false );
		return true;
	}

	return false;
}

bool AlwaysApplyingFunctor::handleMouseEvent( const MouseEvent & event,
	Tool& tool )
{
	BW_GUARD;
	return false;
}

BW_END_NAMESPACE

// always_applying_functor.cpp
