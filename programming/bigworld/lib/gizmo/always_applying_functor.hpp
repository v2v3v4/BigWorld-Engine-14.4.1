#ifndef ALWAYS_APPLYING_FUNCTOR_HPP
#define ALWAYS_APPLYING_FUNCTOR_HPP

#include "tool_functor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	A functor that is constantly applying.
 *	It counts as applying from the time it starts being used as the current
 *	tool until it stops being used.
 */
class AlwaysApplyingFunctor : public ToolFunctor
{
public:
	AlwaysApplyingFunctor( bool allowedToDiscardChanges,
		PyTypeObject * pType = &s_type_ );

	virtual void update( float dTime, Tool& tool );
	virtual bool handleKeyEvent( const KeyEvent & event, Tool& tool );
	virtual bool handleMouseEvent( const MouseEvent & event, Tool& tool );
	virtual void onBeginUsing( Tool & tool );
	virtual void onEndUsing( Tool & tool );
	virtual void stopApplying( Tool & tool, bool saveChanges );

protected:
	virtual bool checkStopApplying() const;
};

BW_END_NAMESPACE

#endif // ALWAYS_APPLYING_FUNCTOR_HPP
