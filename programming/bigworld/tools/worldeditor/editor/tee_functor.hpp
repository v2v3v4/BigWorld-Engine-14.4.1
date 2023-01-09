
#ifndef TEE_FUNCTOR_HPP
#define TEE_FUNCTOR_HPP

#include "gizmo/tool_functor.hpp"

BW_BEGIN_NAMESPACE

/*~ class Functor.TeeFunctor
 *	@components{ worldeditor }
 *
 *	A "tee" functor allows you to toggle between two tools using an alternate
 *	key.
 */
class TeeFunctor : public ToolFunctor
{
	Py_Header( TeeFunctor, ToolFunctor )

public:
	TeeFunctor( ToolFunctor* f1,
		ToolFunctor* f2,
		KeyCode::Key altKey,
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
	KeyCode::Key altKey_;

	ToolFunctor* activeFunctor() const;
};

BW_END_NAMESPACE

#endif // TEE_FUNCTOR_HPP
