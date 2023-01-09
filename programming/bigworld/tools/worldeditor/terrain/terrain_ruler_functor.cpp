#include "pch.hpp"

#include "terrain_ruler_functor.hpp"
#include "terrain_ruler_tool_view.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT(TerrainRulerFunctor)

PY_BEGIN_METHODS(TerrainRulerFunctor)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(TerrainRulerFunctor)
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED(TerrainRulerFunctor, "TerrainRulerFunctor", Functor)

FUNCTOR_FACTORY(TerrainRulerFunctor)


TerrainRulerFunctor::TerrainRulerFunctor(PyTypeObject * pType):
	ToolFunctor(false, pType),
	isMovingRuler_(false),
	isMovingStart_(false),
	isWaitingNextClick_(false),
	time_(0),
	timeOfMouseDown_(0)
{
}

bool TerrainRulerFunctor::handleKeyEvent(const KeyEvent &event, Tool &tool)
{
	BW_GUARD;

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView == NULL)
		return false;

	if (event.key() == KeyCode::KEY_LEFTMOUSE)
	{
		if (event.isKeyDown())
		{
			if (isWaitingNextClick_)
			{
				// 2nd click
				isWaitingNextClick_ = false;
				isMovingRuler_ = false;
				isMovingStart_ = false;

			}
			else if (!isMovingRuler_)
			{
				// 1st click
				isMovingRuler_ = true;
				if (event.isShiftDown())
					isMovingStart_ = true;
				timeOfMouseDown_ = time_;
				Vector3 pos = tool.locator()->transform().applyToOrigin();
				toolView->enableRender(true);
				if (event.isCtrlDown())
				{
					isMovingStart_ = false;
					toolView->endPos(pos);
				}
				else
					toolView->startPos(pos);
			}
		}
		else
		{
			if (time_ - timeOfMouseDown_ <= 0.3)
			{
				// quick release after 1st click
				isWaitingNextClick_ = true;
				isMovingRuler_ = true;
			}
			else
			{
				// released after long pause, stop moving ruler end
				isMovingRuler_ = false;
				isMovingStart_ = false;
			}
		}
	}

	return false;
}

void TerrainRulerFunctor::update(float dTime, Tool &tool)
{
	BW_GUARD;

	time_ += dTime;

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView == NULL)
		return;

	if (isMovingRuler_)
	{
		Vector3 pos = tool.locator()->transform().applyToOrigin();
		if (isMovingStart_)
			toolView->startPos(pos);
		else
			toolView->endPos(pos);
	}
}


void TerrainRulerFunctor::onBeginUsing(Tool &tool)
{
	this->reset();
}

PyObject *TerrainRulerFunctor::pyNew(PyObject * pyArgs)
{
	BW_GUARD;

	return new TerrainRulerFunctor();
}

void TerrainRulerFunctor::reset()
{
	isMovingRuler_ = false;
	isWaitingNextClick_ = false;
	timeOfMouseDown_ = 0;
}

void TerrainRulerFunctor::onEndUsing( Tool & tool )
{
	this->reset();
}

BW_END_NAMESPACE

