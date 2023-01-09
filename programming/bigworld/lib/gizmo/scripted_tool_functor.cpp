
#include "pch.hpp"

#include "scripted_tool_functor.hpp"
#include "gizmo/tool.hpp"

BW_BEGIN_NAMESPACE

//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE ScriptedToolFunctor::

PY_TYPEOBJECT( ScriptedToolFunctor )

PY_BEGIN_METHODS( ScriptedToolFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ScriptedToolFunctor )
	/*~ attribute ScriptedToolFunctor.script
	 *	@components{ tools }
	 *
	 *	Get the script object.
	 *
	 *	@type	PyObject
	 */
	PY_ATTRIBUTE( script )
	/*~ attribute ScriptedToolFunctor.applying
	 *	@components{ tools }
	 *
	 *	If the tool is currently being applied.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( applying )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( ScriptedToolFunctor, "ScriptedFunctor", Functor )

/// declare our C++ factory method
FUNCTOR_FACTORY( ScriptedToolFunctor )

ScriptedToolFunctor::ScriptedToolFunctor( PyObject* pScriptObject,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ):

	ToolFunctor( allowedToDiscardChanges, pType ),
	pScriptObject_( pScriptObject )
{
}


void ScriptedToolFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call(
			PyObject_GetAttrString( pScriptObject_.getObject(), "update" ),
			Py_BuildValue( "(fO)", dTime, static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::update",
			true );
	}
}



/**
 *	This method handles key events for the ScriptedToolFunctor.
 *	It does this by delegating to our script object.
 *
 *	@param event	the key event to process.
 *	@param tool		the tool to use.
 */
bool ScriptedToolFunctor::handleKeyEvent( const KeyEvent & event, Tool& tool )
{
	BW_GUARD;

	bool handled = false;
	if (pScriptObject_)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onKeyEvent" ),
			Py_BuildValue( "(NO)", Script::getData( event ),
				static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::handleKeyEvent",
			true );

		Script::setAnswer( pResult, handled,
			"ScriptedToolFunctor::handleKeyEvent" );
	}

	return handled;
}


/**
 *	This method handles mouse events for the ScriptedToolFunctor.
 *	It does this by delegating to our script object.
 *
 *	@param event	the mouse event to process.
 *	@param tool		the tool to use.
 */
bool ScriptedToolFunctor::handleMouseEvent( const MouseEvent & event, Tool& tool )
{
	BW_GUARD;

	bool handled = false;

	if (pScriptObject_)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onMouseEvent" ),
			Py_BuildValue( "(NO)", Script::getData( event ),
				static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::handleMouseEvent",
			true );

		Script::setAnswer( pResult, handled,
			"ScriptedToolFunctor::handleMouseEvent" );
	}

	return handled;
}


/**
 *	This method handles right mouse button click for the ScriptedToolFunctor.
 *	It does this by delegating to our script object.
 *
 *	@param event	the mouse event to process.
 *	@param tool		the tool to use.
 */
bool ScriptedToolFunctor::handleContextMenu( Tool& tool )
{
	BW_GUARD;

	bool handled = false;

	if (pScriptObject_)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onContextMenu" ),
			Py_BuildValue( "(O)",
				static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::handleContextMenu",
			true );

		Script::setAnswer( pResult, handled,
			"ScriptedToolFunctor::handleContextMenu" );
	}

	return handled;
}


void ScriptedToolFunctor::stopApplying( Tool & tool, bool saveChanges )
{
	BW_GUARD;
	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString( pScriptObject_.getObject(), "stopApplying" ),
			Py_BuildValue( "Ob",
			static_cast<PyObject*>( &tool ), saveChanges ),
			"ScriptedToolFunctor::stopApplying",
			true
			);
	}
}


void ScriptedToolFunctor::onPush( Tool & tool )
{
	BW_GUARD;
	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onPush" ),
			Py_BuildValue( "O", static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::onPush",
			true
			);
	}
}


void ScriptedToolFunctor::onPop( Tool & tool )
{
	BW_GUARD;
	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onPop" ),
			Py_BuildValue( "O", static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::onPop",
			true
			);
	}
}


/** 
 *	This gets called when the functor's tool is about to be used.
 */
void ScriptedToolFunctor::onBeginUsing( Tool &tool )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
		(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onBeginUsing" ),
			Py_BuildValue( "(O)", static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::onBeginUsing",
			true
		);
	}
}


/** 
 *	This gets called when the functor's tool is not being used any more.
 */
void ScriptedToolFunctor::onEndUsing( Tool &tool )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
		(
			PyObject_GetAttrString( pScriptObject_.getObject(), "onEndUsing" ),
			Py_BuildValue( "(O)", static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::onEndUsing",
			true
		);
	}
}


bool ScriptedToolFunctor::isAllowedToDiscardChanges() const
{
	BW_GUARD;

	bool result = ToolFunctor::isAllowedToDiscardChanges();

	if (pScriptObject_)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_.getObject(),
				"isAllowedToDiscardChanges" ),
			PyTuple_New(0),
			"ScriptedToolFunctor::isAllowedToDiscardChanges",
			true );

		Script::setAnswer( pResult, result,
			"ScriptedToolFunctor::isAllowedToDiscardChanges" );
	}

	return result;
}


void ScriptedToolFunctor::beginApply()
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
		(
			PyObject_GetAttrString( pScriptObject_.getObject(), "beginApply" ),
			PyTuple_New(0),
			"ScriptedToolFunctor::beginApply",
			true
		);
	}
}


void ScriptedToolFunctor::doApply( float dTime, Tool & tool )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString( pScriptObject_.getObject(), "doApply" ),
			Py_BuildValue( "(fO)", dTime, static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::doApply",
			true
			);
	}
}


void ScriptedToolFunctor::stopApplyCommitChanges( Tool &tool,
	bool addUndoBarrier )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString(
			pScriptObject_.getObject(), "stopApplyCommitChanges" ),
			Py_BuildValue( "(Ob)",
				static_cast<PyObject*>( &tool ), addUndoBarrier ),
			"ScriptedToolFunctor::stopApplyCommitChanges",
			true
			);
	}
}


void ScriptedToolFunctor::stopApplyDiscardChanges( Tool &tool )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		Script::call
			(
			PyObject_GetAttrString(
			pScriptObject_.getObject(), "stopApplyDiscardChanges" ),
			Py_BuildValue( "(O)", static_cast<PyObject*>( &tool ) ),
			"ScriptedToolFunctor::stopApplyDiscardChanges",
			true
			);
	}
}


/**
 *	Static python factory method
 */
PyObject * ScriptedToolFunctor::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pInst;
	bool allowedToDiscardChanges = true;
	if (!PyArg_ParseTuple( args, "O|b", &pInst, &allowedToDiscardChanges ) ||
		!PyInstance_Check( pInst ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.ScriptedFunctor() "
			"expects a class instance object" );
		return NULL;
	}

	This * pFun = new This( pInst, allowedToDiscardChanges );
	return pFun;
}
BW_END_NAMESPACE

