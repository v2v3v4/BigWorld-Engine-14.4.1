#include "pch.hpp"

#include "tool_functor.hpp"
#include "tool.hpp"
#include "input/input.hpp"
#include "input/py_input.hpp"

BW_BEGIN_NAMESPACE

/*~ module Functor
 *	@components{ tools }
 *
 *	The Functor Module is a Python module that provides an interface to
 *	the various tool functors for moving, rotating, scaling etc.
 */

//----------------------------------------------------
//	Section : ToolFunctor ( Base class )
//----------------------------------------------------

/// static factory map initialiser
template<> FunctorFactory::ObjectMap * FunctorFactory::pMap_;

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ToolFunctor::

PY_TYPEOBJECT( ToolFunctor )

PY_BEGIN_METHODS( ToolFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ToolFunctor )
	/*~ function ToolFunctor.undoName
	 *	@components{ tools }
	 *
	 *	Get and set the text that appears when you undo applying the tool.
	 *	If the string is empty, then we cannot add an undo barrier.
	 *
	 *	@param	String
	 */
	PY_ATTRIBUTE( undoName )
	/*~ attribute ToolFunctor.applying
	 *	@components{ tools }
	 *
	 *	If the tool is currently being applied.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( applying )
	/*~ attribute ToolFunctor.allowedToDiscardChanges
	 *	@components{ tools }
	 *
	 *	If the tool is allowed to discard its changes when cancelled.
	 *	If false, then the tool must commit its changes. Some tools cannot
	 *	discard changes, such as the terrain painting tools so they cannot
	 *	have this set to true.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( allowedToDiscardChanges )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
ToolFunctor::ToolFunctor( bool allowedToDiscardChanges, PyTypeObject * pType ):
	PyObjectPlus( pType ),
	applying_( false ),
	undoName_( "" ),
	allowedToDiscardChanges_( allowedToDiscardChanges )
{
}


/**
 *	Destructor.
 */
ToolFunctor::~ToolFunctor()
{
}


/**
 *	Update the tool on a regular tick, check for inputs and apply.
 *	
 *	@param dTime the time since the last update.
 *	@param tool the tool that the functor is using.
 */
void ToolFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;
}


/**
 *	Handle context menu.
 *	
 *	@param tool the tool that the functor is using.
 *	@return true if handled.
 */
bool ToolFunctor::handleContextMenu( Tool& tool ) 
{ 
	return false; 
};


/**
 *	Check if the tool functor is allowed to discard changes.
 *	@return true if this functor can discard its changes that were being
 *		applied.
 */
bool ToolFunctor::isAllowedToDiscardChanges() const 
{
	return allowedToDiscardChanges_;
}


/**
 *	Called when the tool begins applying.
 */
void ToolFunctor::beginApply()
{
	BW_GUARD;
	applying_ = true;
}


/**
 *	Called during update, while the tool is applying.
 */
void ToolFunctor::doApply( float dTime, Tool & tool )
{
	BW_GUARD;
	MF_ASSERT( applying_ );
}


/**
 *	This returns whether the tool is being applied.
 *	
 *	@return True if the tool is currently being applied, false otherwise.
 */
bool ToolFunctor::applying() const 
{ 
	return applying_; 
}


/**
 *	Finish applying and keep changes.
 */
void ToolFunctor::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;
	MF_ASSERT( applying_ );
	applying_ = false;

	if (addUndoBarrier && !undoName_.empty())
	{
		UndoRedo::instance().barrier( undoName_, true );
	}
}


/**
 *	Finish applying and discard changes.
 */
void ToolFunctor::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;
	MF_ASSERT( applying_ );
	applying_ = false;
}


/**
 *	This is a callback for when the tool is pushed onto the tools list.
 *	@param tool
 */
void ToolFunctor::onPush( Tool & tool )
{
}


/**
 *	This is a callback for when the tool is popped from the tools list.
 *	@param tool
 */
void ToolFunctor::onPop( Tool & tool )
{
}

PY_SCRIPT_CONVERTERS( ToolFunctor )
BW_END_NAMESPACE

