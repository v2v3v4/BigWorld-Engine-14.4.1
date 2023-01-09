#ifndef TERRAIN_RULER_FUNCTOR_HPP
#define TERRAIN_RULER_FUNCTOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/terrain/terrain_functor.hpp"

BW_BEGIN_NAMESPACE

class TerrainRulerFunctor : public ToolFunctor
{
	Py_Header(TerrainRulerFunctor, ToolFunctor)

public:
	TerrainRulerFunctor(PyTypeObject * pType = &s_type_);

	virtual void update(float dTime, Tool &tool);
	virtual bool handleKeyEvent(const KeyEvent &event, Tool &tool);
	virtual bool handleMouseEvent(const MouseEvent &keyEvent, Tool &tool) { return false; }
	virtual bool applying() const { return isMovingRuler_; }
	virtual void onBeginUsing(Tool &tool);

	virtual void stopApplying( Tool& tool, bool saveChanges ) {}
	virtual void onEndUsing( Tool & tool );

	PY_FACTORY_DECLARE()

private:

	void reset();

	bool isMovingRuler_;
	bool isMovingStart_;
	bool isWaitingNextClick_; // true if mouse button was released quickly after 1st click
	
	double timeOfMouseDown_;
	double time_;

	FUNCTOR_FACTORY_DECLARE(TerrainRulerFunctor())
};

BW_END_NAMESPACE

#endif // TERRAIN_RULER_FUNCTOR_HPP
