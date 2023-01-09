#ifndef TOOL_FUNCTOR_HPP
#define TOOL_FUNCTOR_HPP

#include "cstdmf/named_object.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "input/input.hpp"

BW_BEGIN_NAMESPACE

class KeyEvent;
class MouseEvent;
class Tool;

/**
 *	This class keeps the factory methods for all types of Tool Functors
 */
class ToolFunctor;
typedef NamedObject<ToolFunctor * (*)()> FunctorFactory;

#define FUNCTOR_FACTORY_DECLARE( CONSTRUCT )								\
	static FunctorFactory s_factory;										\
	virtual FunctorFactory & factory() { return s_factory; }				\
	static ToolFunctor * s_create() { return new CONSTRUCT; }				\

#define FUNCTOR_FACTORY( CLASS )											\
	FunctorFactory CLASS::s_factory( #CLASS, CLASS::s_create );				\


/*~ class NoModule.ToolFunctor
 *	@components{ tools }
 *	
 *	The ToolFunctor class handles inputs and performs some operation.
 *	It must release any chunk items it has collected when onEndUsing is called.
 */
class ToolFunctor : public PyObjectPlus
{
	Py_Header( ToolFunctor, PyObjectPlus )

public:
	ToolFunctor( bool allowedToDiscardChanges,
		PyTypeObject * pType = &s_type_ );
	virtual ~ToolFunctor();

	virtual void update( float dTime, Tool& tool );
	virtual bool handleKeyEvent( const KeyEvent & event, Tool& tool ) = 0;
	virtual bool handleMouseEvent( const MouseEvent & event, Tool& tool ) = 0;
	virtual bool handleContextMenu( Tool& tool );
	virtual bool applying() const;
	virtual void stopApplying( Tool& tool, bool saveChanges ) = 0;
	virtual void onPush( Tool & tool );
	virtual void onPop( Tool & tool );

	/**
	 *	Called when this tool becomes the current tool.
	 *	@param tool the tool.
	 */
	virtual void onBeginUsing( Tool & tool ) = 0;

	/**
	 *	Called when this tool ceases to be the current tool
	 *	Stop applying and release any chunk items that have been collected
	 *	while using.
	 *	@param tool the tool.
	 */
	virtual void onEndUsing( Tool & tool ) = 0;

	virtual bool isAllowedToDiscardChanges() const;

	PY_RO_ATTRIBUTE_DECLARE( undoName_, undoName )
	PY_RO_ATTRIBUTE_DECLARE( applying_, applying )
	PY_RO_ATTRIBUTE_DECLARE( allowedToDiscardChanges_, allowedToDiscardChanges )

protected:
	virtual void beginApply();
	virtual void doApply( float dTime, Tool & tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

	/// If the tool is currently being applied to something in the scene
	bool applying_;

	/// What the barrier name will be set to
	BW::string undoName_;

private:
	/// Whether we should keep or discard changes
	const bool allowedToDiscardChanges_;
};


typedef SmartPointer<ToolFunctor>	ToolFunctorPtr;

PY_SCRIPT_CONVERTERS_DECLARE( ToolFunctor )

BW_END_NAMESPACE
#endif // TOOL_FUNCTOR_HPP
