#include "pch.hpp"
#if ENABLE_CONSOLES

#include "math/vector2.hpp"
#include "math/colour.hpp"
#include "romp/console.hpp"
#include "romp/font_metrics.hpp"
#include "pyscript/script.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/auto_config.hpp"

#include "console_gui_component.hpp"
#include "simple_gui.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CallbackConsole
// -----------------------------------------------------------------------------

/**
 * TODO: to be documented.
 */
class CallbackConsole : public EditConsole
{
public:
	CallbackConsole() : allowDarkBackground_(false) { }
	~CallbackConsole() { }

	LineEditor & le()							{ return lineEditor_; }
	const LineEditor & le() const				{ return lineEditor_; }
	const BW::wstring & editText() const		{ return lineEditor_.editString(); }
	void editText( const BW::wstring & s)		{ lineEditor_.editString( s ); }

	virtual bool allowDarkBackground() const	{ return allowDarkBackground_; }
	void setAllowDarkBackground(bool allow)		{ allowDarkBackground_ = allow; }

	SmartPointer<PyObject> pCallable_;

private:

	virtual void processLine( const BW::wstring & line )
	{
		if (!pCallable_) return;

		PyObject * pCall = pCallable_.getObject();
		Py_INCREF( pCall );
		Script::call( pCall, Py_BuildValue( "(u)", line.c_str() ),
			"CallbackConsole::processLine: " );
	}

	bool allowDarkBackground_;
};



// -----------------------------------------------------------------------------
// Section: ConsoleGUIComponent
// -----------------------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ConsoleGUIComponent::

PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::wstring, pConsole_->prompt, editPrompt )
PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, pConsole_->editLine, editRow )
PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, pConsole_->editCol, editCol )
PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, pConsole_->le().lineLength, editLineLength )
PY_RW_ATTRIBUTE_DECLARE( pConsole_->pCallable_, editCallback )

PY_TYPEOBJECT( ConsoleGUIComponent )

PY_BEGIN_METHODS( ConsoleGUIComponent )
	/*~ function ConsoleGUIComponent.prints
	 *	@components{ client, tools }
	 *
	 *	This function renders a string to the console at the position
	 *	specified by the editRow and editCol attributes.
	 *
	 *	Note that if, immediately after calling the prints function, the cursor
	 *	position is not updated then the next tick will overwrite the string
	 *	with the editText.
	 *
	 *	@param	str	the string to print to the console.
	 *
	 */
	PY_METHOD( prints )
	/*~ function ConsoleGUIComponent.clear
	 *	@components{ client, tools }
	 *
	 *	This function clears all text from the Console.
	 */
	PY_METHOD( clear )
	/*~	function ConsoleGUIComponent.visibleWidth
	 *	@components{ client, tools }
	 *
	 *	This will return the number of columns visible for
	 *	this console in the current screen resolution.
	 */
	PY_METHOD( visibleWidth )
	/*~	function ConsoleGUIComponent.visibleHeight
	 *	@components{ client, tools }
	 *
	 *	This will return the number of rows visible for
	 *	this console in the current screen resolution.
	 */
	PY_METHOD( visibleHeight )
	/*~	function ConsoleGUIComponent.rowTop
	 *	@components{ client, tools }
	 *
	 *	This will return the upper y position of the console
	 *	row at the given index, in clip space.
	 *
	 *	@param	rowIndex	index to a row of text in the console.
	 *	@return				upper y position to the row, in clip space.
	 */
	PY_METHOD( rowTop )
	/*~	function ConsoleGUIComponent.columnLeft
	 *	@components{ client, tools }
	 *
	 *	This will return the left side y position of the console
	 *	columns at the given index, in clip space.
	 *
	 *	@param	columnIndex	index to a column of text in the console.
	 *	@return				left side x position to the column, in clip space.
	 */
	PY_METHOD( columnLeft )
	/*~	function ConsoleGUIComponent.toggleFontScale (deprecated)
	 *	@components{ client, tools }
	 *
	 *	This will toggle between using a fixed size font and scaling the font
	 *	to fit the current window size.  Scaling is off by default.
	 *
	 *	Note: this function has been deprecated in favor of font size matching.
	 *	Please, refer to the class overview for more information.
	 *
	 */
	PY_METHOD( toggleFontScale )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ConsoleGUIComponent )
	/*~ attribute ConsoleGUIComponent.characterSize
	 *	@components{ client, tools }
	 *
	 *	This attribute represents the size of a character on the screen.
	 *
	 *	@type	Read-Only Vector2
	 */
	PY_ATTRIBUTE( characterSize )
	/*~ attribute ConsoleGUIComponent.cursor
	 *	@components{ client, tools }
	 *
	 *	This attribute is the read only position of the cursor, in character
	 *	coordinates.  Character (0,0) is the top left, with the horizontal
	 *	component first, and the vertical second.  Assigning to the attribute
	 *	does not cause an error, but has no effect.
	 *
	 *	@type	Vector2
	 */
	PY_ATTRIBUTE( cursor )
	/*~ attribute ConsoleGUIComponent.lineColour
	 *	@components{ client, tools }
	 *
	 *	This write-only attribute is used to set the colour
	 *	of the line of text specified by the editRow attribute. It
	 *	takes a Vector4, interpreted as (red, green, blue alpha),
	 *	where each number should be between 0. and 255.0.
	 *
	 *	@type	Write-Only Vector4
	 */
	PY_ATTRIBUTE( lineColour )
	/*~ attribute ConsoleGUIComponent.editLineLength
	 *	@components{ client, tools }
	 *
	 *	This represents the maximum length of the edit string.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( editLineLength )
	/*~ attribute ConsoleGUIComponent.editEnable
	 *	@components{ client, tools }
	 *
	 *	This attribute enables editing on the console.  If it is non-zero
	 *  (true) then the cursor is displayed, and user input is accepted.  If it
	 *	is set to zero (false), then the cursor is hidden and user input is not
	 *	accepted.
	 *
	 *	@type	Integer (treated as boolean)
	 */
	PY_ATTRIBUTE( editEnable )
	/*~ attribute ConsoleGUIComponent.editPrompt
	 *	@components{ client, tools }
	 *
	 *	This attribute is the prompt that will be displayed before the edit
	 *	line of the console.  The user is not able to move the cursor into the
	 *	prompt.
	 *
	 *	Edits to this attribute are effected immediately, changing the prompt
	 *	on the current edit line.
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( editPrompt )
	/*~ attribute ConsoleGUIComponent.editRow
	 *	@components{ client, tools }
	 *
	 *	This attribute is the vertical position of the cursor, in character
	 *	coordinates.  Zero means the top row of the screen, increasing editRow
	 *	values refer to positions further down the screen.
	 *
	 *	Assigning to this attribute moves the cursor to the specified row.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( editRow )
	/*~ attribute ConsoleGUIComponent.editCol
	 *	@components{ client, tools }
	 *
	 *	This attribute is the vertical position of the cursor, in character
	 *	coordinates.  Zero means the leftmost column of the screen, increasing
	 *	editRow values refer to positions further to the right.
	 *
	 *	Assigning to this attribute moves the cursor to the specified column.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( editCol )
	/*~ attribute ConsoleGUIComponent.editText
	 *	@components{ client, tools }
	 *
	 *	This is the text that is currently being edited.  It is displayed in
	 *	the console immediately to the right of the editPrompt.  On assignment
	 *	the text is immediately changed on the screen, moving the editCursor
	 *	to the end of the string.
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( editText )
	/*~ attribute ConsoleGUIComponent.editCallback
	 *	@components{ client, tools }
	 *
	 *	This attribute is a callback function that gets called when the user
	 *	presses enter when editing.  The function should take one argument
	 *	which is set to the editString as it was when the enter key was
	 *	pressed.
	 *
	 *	@type	Callable Python object, which takes one argument
	 */
	PY_ATTRIBUTE( editCallback )
	/*~ attribute ConsoleGUIComponent.editColour
	 *	@components{ client, tools }
	 *
	 *	This attribute sets the edit line colour.
	 *
	 *	@type	Write-Only Vector2
	 */
	PY_ATTRIBUTE( editColour )
PY_END_ATTRIBUTES()

/*~	function GUI.Console
 *	@components{ client, tools }
 *
 *	Creates and returns a new ConsoleGUIComponent, which displays an editable console on the screen.
 *
 *	@return	The new ConsoleGUIComponent.
 */
PY_FACTORY_NAMED( ConsoleGUIComponent, "Console", GUI )

COMPONENT_FACTORY( ConsoleGUIComponent )


// The default texture for consoles, so that you can get them to draw without
// having to set textureName in script.
AutoConfigString s_defaultTexture( "system/nullBmp" );


/**
 *	Constructor.
 */
ConsoleGUIComponent::ConsoleGUIComponent( PyTypeObject * pType ) :
	SimpleGUIComponent( s_defaultTexture.value(), pType ),
	dTime_( 0 )
{
	BW_GUARD;
	vertices_[0].colour_ = this->SimpleGUIComponent::colour();

	pConsole_ = new CallbackConsole();
	pConsole_->init();
	pConsole_->setConsoleColour( vertices_[0].colour_ );
	pConsole_->setScrolling( true );

	pConsole_->hideCursor();

	WARNING_MSG( "ConsoleGUIComponent has been deprecated.\n" );
}


/**
 *	Destructor.
 */
ConsoleGUIComponent::~ConsoleGUIComponent()
{
	BW_GUARD;
	bw_safe_delete(pConsole_);
}


/**
 *	Static python factory method
 */
PyObject * ConsoleGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Console: "
			"Argument parsing error: No arguments expected." );
		return NULL;
	}

	return new ConsoleGUIComponent();
}


/**
 *	Write a string to the console
 */
PyObject * ConsoleGUIComponent::py_prints( PyObject * args )
{
	BW_GUARD;
	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Console.prints: Expected a string" );
		return NULL;
	}

	pConsole_->print( str );

	Py_RETURN_NONE;
}


/**
 *	Clear the console
 */
PyObject * ConsoleGUIComponent::py_clear( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) > 0)
	{
		PyErr_SetString( PyExc_TypeError,
			"Console.clear: Expected no arguments" );
		return NULL;
	}

	pConsole_->clear();

	Py_RETURN_NONE;
}


/**
 * This methods get the characters size for Python
 */
PyObject * ConsoleGUIComponent::pyGet_characterSize()
{
	BW_GUARD;
	Vector2 size = pConsole_->font()->screenCharacterSize();
	return Script::getData( size );
}


/**
 *	This method gets the 'cursor' attribute for Python
 */
PyObject * ConsoleGUIComponent::pyGet_cursor()
{
	BW_GUARD;
	Vector2	pos( 
		static_cast<float>( pConsole_->cursorX() ), 
		static_cast<float>( pConsole_->cursorY() ));
	return Script::getData( pos );
}


/**
 *	This method sets the 'cursor' attribute for Python
 */
int ConsoleGUIComponent::pySet_cursor( PyObject * value )
{
	BW_GUARD;
	Vector2		pos;
	int ret = Script::setData( value, pos, "cursor" );
	if (ret == 0)
	{
		pConsole_->setCursor( uint8(pos[0]), uint8(pos[1]) );
	}

	return ret;
}


int ConsoleGUIComponent::visibleWidth() const
{
	BW_GUARD;
	return pConsole_->visibleWidth();
}


int ConsoleGUIComponent::visibleHeight() const
{
	BW_GUARD;
	return pConsole_->visibleHeight();
}


float ConsoleGUIComponent::rowTop(int rowIndex) const
{
	BW_GUARD;
	float result = 0;
	if (pConsole_->font().exists())
	{
		float fontHeight = pConsole_->font()->metrics().clipHeight();
		result = 1.0f - fontHeight * rowIndex;
	}
	return result;
}


float ConsoleGUIComponent::columnLeft(int columnIndex) const
{
	BW_GUARD;
	float result = 0;
	if (pConsole_->font().exists())
	{
		// assumes fixed font
		float fontWidth = pConsole_->font()->metrics().clipWidth(L'@');
		result = 1.0f - fontWidth * columnIndex;
	}
	return result;
}


void ConsoleGUIComponent::toggleFontScale()
{
	BW_GUARD;
	pConsole_->toggleFontScale();
}


/**
 *	This method sets the 'lineColour' attribute for Python
 */
int ConsoleGUIComponent::pySet_lineColour( PyObject * value )
{
	BW_GUARD;
	if (value == Py_None)
	{
		pConsole_->lineColourOverride( pConsole_->cursorY() );
		return 0;
	}

	Vector4		vColour;
	if (Script::setData( value, vColour, "lineColour" ) == 0)
	{
		pConsole_->lineColourOverride( pConsole_->cursorY(),
			Colour::getUint32( vColour ) );
		return 0;
	}

	PyErr_SetString( PyExc_TypeError,
		"Console.lineColour must be set to a Vector4 or None" );
	return -1;
}


/**
 *	This method sets the 'editColour' attribute for Python
 */
int ConsoleGUIComponent::pySet_editColour( PyObject * value )
{
	BW_GUARD;
	if (value == Py_None)
	{
		return 0;
	}

	Vector4		vColour;
	if (Script::setData( value, vColour, "editColour" ) == 0)
	{
		pConsole_->editColour( Colour::getUint32( vColour ) );
		return 0;
	}

	PyErr_SetString( PyExc_TypeError,
		"Console.editColour must be set to a Vector4 or None" );
	return -1;
}


/**
 *	This method gets the edit enabled state
 */
bool ConsoleGUIComponent::editEnable() const
{
	BW_GUARD;
	return pConsole_->isCursorShowing();
}

/**
 *	This method sets the edit enabled state
 */
void ConsoleGUIComponent::editEnable( bool on )
{
	BW_GUARD;
	if (on)	pConsole_->showCursor();
	else	pConsole_->hideCursor();

	this->SimpleGUIComponent::focus( on );
}





/**
 *	This method overrides SimpleGUIComponent's update method
 */
void ConsoleGUIComponent::update( float dTime, float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;
	dTime_ = dTime;

	float x, y;
	positionInClip( relativeParentWidth, relativeParentHeight, x, y );

	float width = widthInClip(relativeParentWidth);
	float height = heightInClip(relativeParentHeight);

	float anchorOffsetX, anchorOffsetY;
	this->anchorOffset( width, height, anchorOffsetX, anchorOffsetY );

	x += anchorOffsetX;
	y += anchorOffsetY;

	// update console offset in pixels from clip space
	pConsole_->consoleOffset( Vector2((x + 1) * Moo::rc().screenWidth() / 2,
									  (1 - y) * Moo::rc().screenHeight() / 2) );
}


/**
 *	This method overrides SimpleGUIComponent's draw method
 */
void ConsoleGUIComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	if (this->visible() && !this->momentarilyInvisible() && reallyDraw)
	{
		pConsole_->setConsoleColour( vertices_[0].colour_ );
		pConsole_->draw( dTime_ );
	}

	this->momentarilyInvisible( false );
}

/**
 *	This method overrides SimpleGUIComponent's colour mutator
 */
void ConsoleGUIComponent::colour( uint32 col )
{
	BW_GUARD;
	this->SimpleGUIComponent::colour( col );
	vertices_[0].colour_ = col;	// so shaders get applied
}


/**
 *	Load
 */
bool ConsoleGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	DataSectionPtr pLines = pSect->openSection( "lines" );
	if (pLines)
	{
		int i = 0;
		DataSectionIterator it;
		for (it = pLines->begin(); it != pLines->end(); ++it)
		{
			const BW::string & sname = (*it)->sectionName();
			if (sname == "colour")
			{
				pConsole_->lineColourOverride( i,
					Colour::getUint32( (*it)->asVector4() ) );
			}
			else if (sname == "line")
			{
				BW::wstring tline = (*it)->asWideString();
				if (tline.length() > 2)
				{
					pConsole_->line( i, tline.data()+1, tline.length()-1 );

					i++;
					if (i >= MAX_CONSOLE_HEIGHT) break;

					pConsole_->lineColourOverride( i );
				}
			}
		}
	}

	return true;
}

/**
 *	Save
 */
void ConsoleGUIComponent::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	BW_GUARD;
	this->SimpleGUIComponent::save( pSect, bindings );

	DataSectionPtr pLines = pSect->newSection( "lines" );
	for (int i = 0; i < MAX_CONSOLE_HEIGHT; i++)
	{
		uint32 lc;
		if (pConsole_->lineColourRetrieve( i, lc ))
			pSect->newSection( "colour" )->setVector4(
				Colour::getVector4( lc ) );

		const wchar_t * bline = pConsole_->line( i );

		BW::wstring tline = L"\"";
		for (int x = 0; x < MAX_CONSOLE_WIDTH; x++)
		{
			tline += bline[x] ? bline[x] : L' ';
		}
		tline += L'\"';

		pLines->newSection( "line" )->setWideString( tline );
	}
}




/**
 *	Handle this key event if editing is enabled,
 *	otherwise do the default behaviour.
 */
bool ConsoleGUIComponent::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	if (!this->editEnable())
		return this->SimpleGUIComponent::handleKeyEvent( event );

	return pConsole_->handleKeyEvent( event );
}


/**
 *	This method sets the edit string on the line editor, and advances
 *	the cursor to the end of the string.
 *	@param	s		The string to set.
 */
void ConsoleGUIComponent::editString( const BW::wstring& s )
{
	BW_GUARD;
	MF_ASSERT( s.size() <= INT_MAX );
	pConsole_->le().editString( s );
	pConsole_->le().cursorPosition( ( int )s.size() );
}


/**
 *	This method returns the current edit string. 
 */
const BW::wstring& ConsoleGUIComponent::editString() const
{
	BW_GUARD;
	return pConsole_->le().editString();
}

BW_END_NAMESPACE

/*console_gui_component.cpp*/
#endif // ENABLE_CONSOLES
