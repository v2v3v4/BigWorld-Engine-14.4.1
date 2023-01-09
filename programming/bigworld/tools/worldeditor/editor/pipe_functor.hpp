#ifndef PIPE_FUNCTOR_HPP
#define PIPE_FUNCTOR_HPP

#include "gizmo/tool_functor.hpp"

BW_BEGIN_NAMESPACE

/*~ class Functor.PipeFunctor
 *	@components{ worldeditor }
 *
 *	A "pipe" functor applies two tools at once.
 */
class PipeFunctor : public ToolFunctor
{
	Py_Header( PipeFunctor, ToolFunctor )

public:
	PipeFunctor( ToolFunctor* f1,
		ToolFunctor* f2,
		PyTypeObject * pType = &s_type_ );

	virtual void update( float dTime, Tool& tool );
	virtual bool handleKeyEvent( const KeyEvent & event, Tool& tool );
	virtual bool handleMouseEvent( const MouseEvent & event, Tool& tool );
	virtual bool applying() const;
	virtual void stopApplying( Tool & tool, bool saveChanges );
	virtual void onBeginUsing( Tool & tool );
	virtual void onEndUsing( Tool & tool );
	virtual bool isAllowedToDiscardChanges() const;

	PY_FACTORY_DECLARE()

private:
	ToolFunctor* f1_;
	ToolFunctor* f2_;
};

BW_END_NAMESPACE

#endif // PIPE_FUNCTOR_HPP
