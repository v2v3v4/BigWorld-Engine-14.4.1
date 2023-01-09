#ifndef SCALEFORM_UTIL_HPP
#define SCALEFORM_UTIL_HPP
#if SCALEFORM_SUPPORT

#include "config.hpp"
#include "input/input.hpp"
#include "math/vector2.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	//-------------------------------------------------------------------------
	// data translation utilities
	//-------------------------------------------------------------------------
	void objectToValue(PyObject* obj, GFx::Value& value);
	void valueToObject(const GFx::Value& value, PyObject** obj);

	//-------------------------------------------------------------------------
	// input handling utilities
	//-------------------------------------------------------------------------
	struct NotifyMouseStateFn
	{
		float x;
		float y;
		int buttons;
		Float scrollDelta;
		bool handled_;

		NotifyMouseStateFn(float x_, float y_, int buttons_, Float scrollDelta_);
		void operator () ( class PyMovieView* elem );
		bool handled() const { return handled_; }
	};

	struct HandleEventFn
	{
		HandleEventFn(const GFx::Event& gfxevent);
		void operator () ( class PyMovieView* elem );
		bool handled() const { return handled_; }
		const GFx::Event& gfxevent_;
		bool handled_;
	};

	int mouseButtonsFromEvent(const KeyEvent& event );
	SF::Key::Code translateKeyCode(uint32 eventChar);
	GFx::KeyEvent translateKeyEvent(const KeyEvent & bwevent);

	//-------------------------------------------------------------------------
	// viewport utilities
	//-------------------------------------------------------------------------
	void fullScreenViewport( GFx::Viewport &outGv  );
	void viewportFromClipBounds( const Vector2* corners, GFx::Viewport& outGv );
	void rectFromClipBounds( const Vector2* corners, GRectF& outRect, int halfScreenWidth, int halfScreenHeight  );
	void runtimeGUIViewport( const Vector2* corners, GFx::Viewport& gv, GRectF& rect );

}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
#endif //#ifndef SCALEFORM_UTIL_HPP