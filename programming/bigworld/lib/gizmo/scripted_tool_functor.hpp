#ifndef SCRIPTED_TOOL_FUNCTOR_HPP
#define SCRIPTED_TOOL_FUNCTOR_HPP

#include "gizmo/tool_functor.hpp"

BW_BEGIN_NAMESPACE

/*~ class NoModule.ScriptedToolFunctor
 *	@components{ tools }
 *	
 *	The scriptedToolFunctor delegates all toolFunctor
 *	methods to a script object.
 */
class ScriptedToolFunctor : public ToolFunctor
{
	Py_Header( ScriptedToolFunctor, ToolFunctor )
public:
	ScriptedToolFunctor( PyObject* pScriptObject = NULL,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	void update( float dTime, Tool& tool );
	bool handleKeyEvent( const KeyEvent & event, Tool& tool );
	bool handleMouseEvent( const MouseEvent & event, Tool& tool );
	bool handleContextMenu( Tool & tool );
	void stopApplying( Tool & tool, bool saveChanges );
	void onPush( Tool & tool );
	void onPop( Tool & tool );
	void onBeginUsing( Tool & tool );
	void onEndUsing( Tool & tool );
	bool isAllowedToDiscardChanges() const;

	/**
	 *	Python attributes
	 */
	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( applying_, applying )
	PY_RW_ATTRIBUTE_DECLARE( pScriptObject_, script )

protected:
	void beginApply();
	void doApply( float dTime, Tool & tool );
	void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	void stopApplyDiscardChanges( Tool& tool );

private:
	///pluggable script event handling
	SmartPointer<PyObject>		pScriptObject_;

	FUNCTOR_FACTORY_DECLARE( ScriptedToolFunctor() )
};

typedef SmartPointer<ScriptedToolFunctor>	ScriptedToolFunctorPtr;

BW_END_NAMESPACE
#endif // SCRIPTED_TOOL_FUNCTOR_HPP
