#include "pch.hpp"
#if SCALEFORM_SUPPORT

#include "util.hpp"
#include "moo/render_context.hpp"
#include "ashes/simple_gui.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	/**
	 *	This fuction converts from a python object to a GFx::Value.
	 *
	 *	@param	PyObject*	incoming python object.
	 *	@param	GFx::Value&	translated GFx::Value representation.
	 */
	void objectToValue(PyObject* obj, GFx::Value& value)
	{
		BW_GUARD;
		if ( !obj )
		{
			value.SetNull();
		}
		else if (PyUnicode_CheckExact(obj))
		{
			PyObject* utf8 = PyUnicode_AsUTF8String(obj);
			MF_ASSERT( utf8 != NULL );

			value.SetString(PyString_AsString(utf8));
		}
		else if (PyString_CheckExact(obj))
		{
			value.SetString(PyString_AsString(obj));
		}
		else if (PyInt_CheckExact(obj))
		{
			value.SetNumber(PyInt_AsLong(obj));
		}
		else if (PyLong_CheckExact(obj))
		{
			value.SetNumber(PyLong_AsDouble(obj));
		}
		else if (PyFloat_CheckExact(obj))
		{
			value.SetNumber(PyFloat_AsDouble(obj));
		}
		else if (PyBool_Check(obj))
		{
			value.SetBoolean(obj == Py_True);
		}
		else
		{
			value.SetNull();
		}
	}


	/**
	 *	This fuction converts a GFx::Value to a python object.
	 *
	 *	@param	GFx::Value&	incoming GFx::Value representation.
	 *	@param	PyObject*	translated as a python object.
	 */
	void valueToObject(const GFx::Value& value, PyObject** obj)
	{
		BW_GUARD;
		switch (value.GetType())
		{
		case GFx::Value::VT_Undefined:
		case GFx::Value::VT_Null:
			*obj = Py_None;
			Py_INCREF(*obj);
			break;

		case GFx::Value::VT_Boolean:
			*obj = PyBool_FromLong(value.GetBool());
			break;

		case GFx::Value::VT_Number:
			*obj = PyFloat_FromDouble(value.GetNumber());
			break;

		case GFx::Value::VT_String:
			*obj = PyString_FromString(value.GetString());
			break;

		case GFx::Value::VT_StringW:
			*obj = PyUnicode_FromWideChar(value.GetStringW(), wcslen(value.GetStringW()) );
			break;
		}
	}


	/** 
	 *	This function initialises a GViewport structure with values
	 *	required to display a movie over the full screen.
	 *
	 *	@param	GViewport&	[out] filled GViewport structure representing the
	 *						entire back buffer.
	 */
	void fullScreenViewport( GFx::Viewport &outGv )
	{
		BW_GUARD;
		SInt w = (SInt)Moo::rc().backBufferDesc().Width;
		SInt h = (SInt)Moo::rc().backBufferDesc().Height;

		outGv = GFx::Viewport( w, h, 0, 0, w, h );
	}


	/** 
	 *	This function initialises a GViewport structure with values
	 *	required to display a movie in the area given by four corners
	 *	of a GUI component.  The corners should be created via the
	 *	SimpleGUIComponent::clibBounds() method.
	 *
	 *	@param	Vector2*	clip-space coordinates of the bounding rectangle.
	 *	@param	GViewport&	[out] filled GViewport structure representing the
	 *						desired view area.
	 */
	void viewportFromClipBounds( const Vector2* corners, GFx::Viewport& outGv )
	{
		BW_GUARD;
		//topleft..topRight..bottomLeft..bottomRight
		SInt x = (SInt)((corners[0].x + 1.f) * Moo::rc().halfScreenWidth());
		SInt y = (SInt)((1.f - corners[0].y) * Moo::rc().halfScreenHeight());
		SInt w = (SInt)((corners[3].x + 1.f) * Moo::rc().halfScreenWidth()) - x;
		SInt h = (SInt)((1.f - corners[3].y) * Moo::rc().halfScreenHeight()) - y;

		//work out runtime size of movie and apply
		D3DVIEWPORT9 pViewport;
		Moo::rc().device()->GetViewport( &pViewport );
		outGv = GFx::Viewport(
			(SInt)Moo::rc().backBufferDesc().Width,
			(SInt)Moo::rc().backBufferDesc().Height
			,x, y, w, h );
	}


	/** 
	 *	This function initialises a GRectF structure with values
	 *	required to display a movie in the area given by four corners
	 *	of a GUI component.  The corners should be created via the
	 *	SimpleGUIComponent::clibBounds() method.
	 *
	 *	@param	Vector2*	clip-space coordinates of the bounding rectangle.
	 *	@param	GRectF&		[out] filled GRectF structure representing the
	 *						desired view area.
	 */
	void rectFromClipBounds( const Vector2* corners, GRectF& outRect, int halfScreenWidth, int halfScreenHeight )
	{
		BW_GUARD;
		//topleft..topRight..bottomLeft..bottomRight
		outRect.SetRect(
			(corners[0].x + 1.f) * halfScreenWidth,
			(1.f - corners[0].y) * halfScreenHeight,
			(corners[3].x + 1.f) * halfScreenWidth,
			(1.f - corners[3].y) * halfScreenHeight
		);
	}


	/**
	 *	This method takes 4 corner positions in GUI local space, and outputs
	 *	a clipped viewport/rect with which to draw a scaleform GUI component.
	 *
	 *	It additionally takes into account any clipping by current GUI windows.
	 *
	 *	@param	Vector2*	clip-space coordinates of the bounding rectangle.
	 *	@param	GViewport&	[out] filled GViewport structure representing the
	 *						desired view area.
	 *	@param	GRectF&		[out] filled GRectF structure representing the
	 *						desired view area.
	 */
	void runtimeGUIViewport( const Vector2* lcorners, GFx::Viewport& gv, GRectF& rect )
	{
		BW_GUARD;
		Vector2 corners[4];
		for ( size_t i=0; i<4; i++ )
		{
			Vector3 worldCorner =
				Moo::rc().world().applyPoint( Vector3(lcorners[i].x,lcorners[i].y,0.f) );
			corners[i].set( worldCorner.x, worldCorner.y );
		}

		fullScreenViewport( gv );

		//scissors rect gets disabled for some reason, so manually clip the text
		//viewport rectangle against the current gui clip region.
		Vector4 clippedViewport( SimpleGUI::instance().clipRegion() );		//clip region goes from -1,-1 .. 1,1

		int halfScreenWidth = gv.Width / 2;
		int halfScreenHeight = gv.Height / 2;
		clippedViewport.x = gv.Left + clippedViewport.x * halfScreenWidth + halfScreenWidth;
		clippedViewport.y = gv.Top + (-clippedViewport.y) * halfScreenHeight + halfScreenHeight;
		clippedViewport.z = gv.Left + clippedViewport.z * halfScreenWidth + halfScreenWidth;
		clippedViewport.w = gv.Top + (-clippedViewport.w) * halfScreenHeight + halfScreenHeight;

		gv.Left = (SInt)clippedViewport.x;
		gv.Width = (SInt)(clippedViewport.z - clippedViewport.x);
		gv.Top = (SInt)clippedViewport.y;
		gv.Height = (SInt)(clippedViewport.w - clippedViewport.y);

		rectFromClipBounds( corners, rect, halfScreenWidth, halfScreenHeight );

		//scaleform wants movie/text rect to be relative to the viewport rect.
		rect.SetRect(
			rect.X1() - (float)gv.Left,
			rect.Y1() - (float)gv.Top,
			rect.X2() - (float)gv.Left,
			rect.Y2() - (float)gv.Top
		);
	}

	// if true, translateKeyCode will allow all keys except of Escape
	static bool g_redefineKeysMode = false;

	/**
	 *	This function translates a BigWorld key event code to the
	 *	corresponding SF::Key code.
	 *
	 *	If there is no appropriate translation, the returned SF::Key::Code
	 *	will be set to SF::Key::None
	 *
	 *	@param	uint32			BigWorld key event code.
	 *	@return	SF::Key::Code	Translated key event code.
	 */
	SF::Key::Code translateKeyCode(uint32 eventChar)
	{
		BW_GUARD;
		UINT key = (UINT)eventChar;
		SF::Key::Code    c(SF::Key::None);
		
		struct TableEntry
		{
			int vk;
			SF::Key::Code gs;
		};

		// many keys don't correlate, so just use a look-up table.
		static TableEntry defaultKeyTable[] =
		{
			{ KeyCode::KEY_TAB,			SF::Key::Tab },
			{ KeyCode::KEY_RETURN,			SF::Key::Return },
			{ KeyCode::KEY_ESCAPE,			SF::Key::Escape },
			{ KeyCode::KEY_LEFTARROW,		SF::Key::Left },
			{ KeyCode::KEY_UPARROW,		SF::Key::Up },
			{ KeyCode::KEY_RIGHTARROW,		SF::Key::Right },
			{ KeyCode::KEY_DOWNARROW,		SF::Key::Down },
			{ KeyCode::KEY_HOME,			SF::Key::Home },
			{ KeyCode::KEY_END,			SF::Key::End },
			{ KeyCode::KEY_BACKSPACE,		SF::Key::Backspace },
			{ KeyCode::KEY_PGUP,			SF::Key::PageUp },
			{ KeyCode::KEY_PGDN,			SF::Key::PageDown },
			{ KeyCode::KEY_INSERT,			SF::Key::Insert },
			{ KeyCode::KEY_DELETE,			SF::Key::Delete },
			{ KeyCode::KEY_LSHIFT,			SF::Key::Shift },
			{ KeyCode::KEY_RSHIFT,			SF::Key::Shift },
			{ KeyCode::KEY_LCONTROL,		SF::Key::Control },
			{ KeyCode::KEY_RCONTROL,		SF::Key::Control },
			{ KeyCode::KEY_X,				SF::Key::X },
			{ KeyCode::KEY_C,				SF::Key::C },
			{ KeyCode::KEY_V,				SF::Key::V },
			{ KeyCode::KEY_A,				SF::Key::A },
			{ KeyCode::KEY_H,				SF::Key::H },
			{ KeyCode::KEY_F1,				SF::Key::F1 },
			{ KeyCode::KEY_SPACE,			SF::Key::Space },
			{ KeyCode::KEY_NUMPADENTER,	SF::Key::Return },
			{ KeyCode::KEY_0,				SF::Key::Num0 },
			{ KeyCode::KEY_1,				SF::Key::Num1 },
			{ KeyCode::KEY_2,				SF::Key::Num2 },
			{ KeyCode::KEY_3,				SF::Key::Num3 },
			{ KeyCode::KEY_4,				SF::Key::Num4 },
			{ KeyCode::KEY_5,				SF::Key::Num5 },
			{ KeyCode::KEY_6,				SF::Key::Num6 },
			{ KeyCode::KEY_7,				SF::Key::Num7 },
			{ KeyCode::KEY_8,				SF::Key::Num8 },
			{ KeyCode::KEY_9,				SF::Key::Num9 },
			{ KeyCode::KEY_CAPSLOCK,		SF::Key::CapsLock },
				// @@ TODO fill this out some more
			{ 0, SF::Key::None }
		};

		// note: missing keys in Scaleform are apostrophe, grave and some more
		static TableEntry redefineKeyModeTable[] = 
		{
			{ KeyCode::KEY_A, SF::Key::A },
			{ KeyCode::KEY_B, SF::Key::B },
			{ KeyCode::KEY_C, SF::Key::C },
			{ KeyCode::KEY_D, SF::Key::D },
			{ KeyCode::KEY_E, SF::Key::E },
			{ KeyCode::KEY_F, SF::Key::F },
			{ KeyCode::KEY_G, SF::Key::G },
			{ KeyCode::KEY_H, SF::Key::H },
			{ KeyCode::KEY_I, SF::Key::I },
			{ KeyCode::KEY_J, SF::Key::J },
			{ KeyCode::KEY_K, SF::Key::K },
			{ KeyCode::KEY_L, SF::Key::L },
			{ KeyCode::KEY_M, SF::Key::M },
			{ KeyCode::KEY_N, SF::Key::N },
			{ KeyCode::KEY_O, SF::Key::O },
			{ KeyCode::KEY_P, SF::Key::P },
			{ KeyCode::KEY_Q, SF::Key::Q },
			{ KeyCode::KEY_R, SF::Key::R },
			{ KeyCode::KEY_S, SF::Key::S },
			{ KeyCode::KEY_T, SF::Key::T },
			{ KeyCode::KEY_U, SF::Key::U },
			{ KeyCode::KEY_V, SF::Key::V },
			{ KeyCode::KEY_W, SF::Key::W },
			{ KeyCode::KEY_X, SF::Key::X },
			{ KeyCode::KEY_Y, SF::Key::Y },
			{ KeyCode::KEY_Z, SF::Key::Z },
		
			{ KeyCode::KEY_F1,  SF::Key::F1 },
			{ KeyCode::KEY_F2,  SF::Key::F2 },
			{ KeyCode::KEY_F3,  SF::Key::F3 },
			{ KeyCode::KEY_F4,  SF::Key::F4 },
			{ KeyCode::KEY_F5,  SF::Key::F5 },
			{ KeyCode::KEY_F6,  SF::Key::F6 },
			{ KeyCode::KEY_F7,  SF::Key::F7 },
			{ KeyCode::KEY_F8,  SF::Key::F8 },
			{ KeyCode::KEY_F9,  SF::Key::F9 },
			{ KeyCode::KEY_F10, SF::Key::F10 },
			{ KeyCode::KEY_F11, SF::Key::F11 },
			{ KeyCode::KEY_F12, SF::Key::F12 },
			{ KeyCode::KEY_F13, SF::Key::F13 },
			{ KeyCode::KEY_F14, SF::Key::F14 },
			{ KeyCode::KEY_F15, SF::Key::F15 },
			
			{ KeyCode::KEY_0, SF::Key::Num0 },
			{ KeyCode::KEY_1, SF::Key::Num1 },
			{ KeyCode::KEY_2, SF::Key::Num2 },
			{ KeyCode::KEY_3, SF::Key::Num3 },
			{ KeyCode::KEY_4, SF::Key::Num4 },
			{ KeyCode::KEY_5, SF::Key::Num5 },
			{ KeyCode::KEY_6, SF::Key::Num6 },
			{ KeyCode::KEY_7, SF::Key::Num7 },
			{ KeyCode::KEY_8, SF::Key::Num8 },
			{ KeyCode::KEY_9, SF::Key::Num9 },

			{ KeyCode::KEY_NUMPAD0,			SF::Key::KP_0 },
			{ KeyCode::KEY_NUMPAD1,			SF::Key::KP_1 },
			{ KeyCode::KEY_NUMPAD2,			SF::Key::KP_2 },
			{ KeyCode::KEY_NUMPAD3,			SF::Key::KP_3 },
			{ KeyCode::KEY_NUMPAD4,			SF::Key::KP_4 },
			{ KeyCode::KEY_NUMPAD5,			SF::Key::KP_5 },
			{ KeyCode::KEY_NUMPAD6,			SF::Key::KP_6 },
			{ KeyCode::KEY_NUMPAD7,			SF::Key::KP_7 },
			{ KeyCode::KEY_NUMPAD8,			SF::Key::KP_8 },
			{ KeyCode::KEY_NUMPAD9,			SF::Key::KP_9 },
			{ KeyCode::KEY_NUMPADSTAR,		SF::Key::KP_Multiply },
			{ KeyCode::KEY_NUMPADMINUS,		SF::Key::KP_Subtract },
			{ KeyCode::KEY_ADD,				SF::Key::KP_Add },
			{ KeyCode::KEY_NUMPADPERIOD,	SF::Key::KP_Decimal },
			{ KeyCode::KEY_NUMPADSLASH,		SF::Key::KP_Divide },

			{ KeyCode::KEY_ESCAPE,			SF::Key::Escape },
			{ KeyCode::KEY_RETURN,			SF::Key::Return },
			{ KeyCode::KEY_NUMPADENTER,		SF::Key::Return },
			{ KeyCode::KEY_BACKSPACE,		SF::Key::Backspace },
			{ KeyCode::KEY_TAB,				SF::Key::Tab },
			{ KeyCode::KEY_SPACE,			SF::Key::Space },
			{ KeyCode::KEY_LSHIFT,			SF::Key::Shift },
			{ KeyCode::KEY_RSHIFT,			SF::Key::Shift },
			{ KeyCode::KEY_RCONTROL,		SF::Key::Control },
			{ KeyCode::KEY_LCONTROL,		SF::Key::Control },
			{ KeyCode::KEY_LALT,			SF::Key::Alt },
			{ KeyCode::KEY_RALT,			SF::Key::Alt },
			{ KeyCode::KEY_LBRACKET,		SF::Key::BracketLeft },
			{ KeyCode::KEY_RBRACKET,		SF::Key::BracketRight },
			{ KeyCode::KEY_LEFTARROW,		SF::Key::Left },
			{ KeyCode::KEY_RIGHTARROW,		SF::Key::Right },
			{ KeyCode::KEY_UPARROW,			SF::Key::Up },
			{ KeyCode::KEY_DOWNARROW,		SF::Key::Down },
			{ KeyCode::KEY_INSERT,			SF::Key::Insert },
			{ KeyCode::KEY_DELETE,			SF::Key::Delete },
			{ KeyCode::KEY_HOME,			SF::Key::Home },
			{ KeyCode::KEY_END,				SF::Key::End },
			{ KeyCode::KEY_PGUP,			SF::Key::PageUp },
			{ KeyCode::KEY_PGDN,			SF::Key::PageDown },
			{ KeyCode::KEY_SLASH,			SF::Key::Slash },
			{ KeyCode::KEY_BACKSLASH,		SF::Key::Backslash },
			{ KeyCode::KEY_MINUS,			SF::Key::Minus },
			{ KeyCode::KEY_EQUALS,			SF::Key::Equal },
			{ KeyCode::KEY_CAPSLOCK,		SF::Key::CapsLock },

			{ KeyCode::KEY_COMMA,			SF::Key::Comma },
			{ KeyCode::KEY_PERIOD,			SF::Key::Period },
			{ KeyCode::KEY_SEMICOLON,		SF::Key::Semicolon },
			{ KeyCode::KEY_OEM_102,			SF::Key::OEM_102 },
			{ KeyCode::KEY_AX,				SF::Key::OEM_AX },

			{ 0, SF::Key::None }
		};

		TableEntry *table = g_redefineKeysMode ? redefineKeyModeTable : defaultKeyTable;
		for (int i = 0; table[i].vk != 0; i++)
		{
			if (key == (UInt)table[i].vk)
			{
				c = table[i].gs;
				break;
			}
		}

		return c;
	}


	/**
	 *	This function translates a BigWorld KeyEvent to a Scaleform Key Event.
	 *
	 *	@param	KeyEvent&		BigWorld KeyEvent.
	 *	@return	GFx::KeyEvent		Scaleform KeyEvent.
	 */
	GFx::KeyEvent translateKeyEvent(const KeyEvent & bwevent)
	{
		BW_GUARD;
		bool down = bwevent.isKeyDown();
		SF::Key::Code c = translateKeyCode(bwevent.key());
		GFx::KeyEvent gfxevent(down ? GFx::Event::KeyDown : GFx::KeyEvent::KeyUp, c, 0, 0);
		gfxevent.Modifiers.SetShiftPressed(bwevent.isShiftDown() && (c != SF::Key::Shift));
		gfxevent.Modifiers.SetCtrlPressed(bwevent.isCtrlDown() && (c != SF::Key::Control));
		gfxevent.Modifiers.SetAltPressed(bwevent.isAltDown());
		gfxevent.Modifiers.SetNumToggled((::GetKeyState(VK_NUMLOCK) & 0x1) ? 1: 0);
		gfxevent.Modifiers.SetCapsToggled((::GetKeyState(VK_CAPITAL) & 0x1) ? 1: 0);
		gfxevent.Modifiers.SetScrollToggled((::GetKeyState(VK_SCROLL) & 0x1) ? 1: 0);

		return gfxevent;
	}


	/**
	 *	This function fills a bit field desribing the current state of the mouse buttons.
	 *	The bitfield is used by various Scaleform mouse handling methods.
	 *
	 *	@param	KeyEvent&		BigWorld KeyEvent containing mouse button information.
	 *	@return int				Packed bitfield in Scaleform format.
	 */
	int mouseButtonsFromEvent(const KeyEvent &event )
	{
		BW_GUARD;
		KeyCode::Key keyEvent( KeyCode::KEY_NONE );

		if( event.isKeyDown() ) 
			keyEvent = event.key();
		else
			keyEvent = KeyCode::KEY_NONE;

		const static KeyCode::Key mouseCodes[] = {KeyCode::KEY_LEFTMOUSE,
			KeyCode::KEY_RIGHTMOUSE,
			KeyCode::KEY_MIDDLEMOUSE,
			KeyCode::KEY_MOUSE3,
			KeyCode::KEY_MOUSE4,
			KeyCode::KEY_MOUSE5,
			KeyCode::KEY_MOUSE6,
			KeyCode::KEY_MOUSE7};
		
		for (int i = 0; i < 8; i++)
		{
			if (keyEvent == mouseCodes[i])
			{
				return 1 << i;
			}
		}

		return 0;
	}

	/*
	* Turns redefine keys input mode on/off. In that mode all keyboard events are translated from
	* BigWorld to Scaleform, not just limited subset of keys (like in regular input mode).
	*/
	void setRedefineKeysMode(bool b)
	{
		g_redefineKeysMode = b;
	}
	PY_AUTO_MODULE_FUNCTION(RETVOID, setRedefineKeysMode, ARG(bool, END), BigWorld)

	bool isCapsLockOn()
	{
		return (::GetKeyState(VK_CAPITAL) & 0x1) != 0;
	}
	PY_AUTO_MODULE_FUNCTION(RETDATA, isCapsLockOn, END, BigWorld)

	bool isNumLockOn()
	{
		return (::GetKeyState(VK_NUMLOCK) & 0x1) != 0;
	}
	PY_AUTO_MODULE_FUNCTION(RETDATA, isNumLockOn, END, BigWorld)

	bool isScrollLockOn()
	{
		return (::GetKeyState(VK_SCROLL) & 0x1) != 0;
	}
	PY_AUTO_MODULE_FUNCTION(RETDATA, isScrollLockOn, END, BigWorld)

}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT