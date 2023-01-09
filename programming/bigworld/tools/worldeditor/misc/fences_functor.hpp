#ifndef FENCES_FUNCTOR_HPP
#define FENCES_FUNCTOR_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class FencesFunctor : public ToolFunctor
{
	Py_Header( FencesFunctor, ToolFunctor )

public:
	FencesFunctor( PyTypePlus * pType = &s_type_ );

	virtual void update( float dTime, Tool &tool );
	virtual bool handleKeyEvent( const KeyEvent &event, Tool &tool ) { return false; }
	virtual bool handleMouseEvent( const MouseEvent &keyEvent, Tool &tool ) { return false; }
	virtual bool applying() const { return isActive_; }
	virtual void onEndUsing( Tool &tool ) { isActive_ = false; }
	virtual void stopApplying( Tool& tool, bool saveChanges ){}
	virtual void onBeginUsing( Tool & tool ){}
	

	void addFenceSection();

	PY_FACTORY_DECLARE()

private:
	bool isActive_;

	FUNCTOR_FACTORY_DECLARE( FencesFunctor() )
};

BW_END_NAMESPACE

#endif // FENCES_FUNCTOR_HPP
