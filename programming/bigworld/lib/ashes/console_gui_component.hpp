#ifndef CONSOLE_GUI_COMPONENT_HPP
#define CONSOLE_GUI_COMPONENT_HPP

#if ENABLE_CONSOLES
#include "simple_gui_component.hpp"


BW_BEGIN_NAMESPACE

class CallbackConsole;


/*~ class GUI.ConsoleGUIComponent
 *	@components{ client, tools }
 *
 *	This class has been deprecated in favour of script based solutions
 *	that take advantage of multiline TextGUIComponents.
 *
 *	This class is used to supply an editable console, which renders characters
 *	and a flashing cursor to the screen.  It can be switched between
 *	edit and non-edit mode using the editEnable attribute, which defaults to 
 *	non-edit mode.  The position of the
 *	cursor can be moved around using the editRow and editCol attributes.
 *	Text can be written straight to the console programatically using the prints
 *	function.  There is no support for wrapping, so only the portion of the 
 *	string that fits on the screen is drawn.
 *
 *	When in edit mode, if the user presses enter then the callback function
 *	specified by editCallback is called, with the entered text as an argument.
 *	By default, the console does nothing with the entered text.  The callback
 *	function is required to do anything with it, including adding it to a previous
 *	line of the screen.  Once the callback function has completed, the console
 *	clears the edit line.
 *
 *	BigWorld consoles use font size matching to select which font will be
 *	used to render text. With font size matching, whenever a new screen 
 *	resolution is selected, the font that yields the number of columns that 
 *	best matches the ideal console width will be chosen from the list of 
 *	available fonts. The list of fonts is defined in resource.xml's 
 *	system/consoleFonts entry. The number of visible columns and rows, thus, 
 *	will change whenever the resolution changes. The visibleWidth and 
 *	visibleHeight methods can be used to retrieve the current number of columns 
 *	and rows. rowTop and columnLeft can be used to get the x and y screen 
 *	position, respectively, of any character in the console.
 *
 *	A new ConsoleGUIComponent is created using the GUI.Console function.
 */
/**
 *	This GUI component wraps a multi-line EditConsole
 */
class ConsoleGUIComponent : public SimpleGUIComponent
{
	Py_Header( ConsoleGUIComponent, SimpleGUIComponent )

public:
	ConsoleGUIComponent( PyTypeObject* pType = &s_type_ );
	~ConsoleGUIComponent();

	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_prints )
	PY_METHOD_DECLARE( py_clear )

	PyObject * pyGet_characterSize();
	PY_RO_ATTRIBUTE_SET( characterSize )

	PyObject * pyGet_cursor();
	int pySet_cursor( PyObject * value );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, scale, scale )

	PY_WO_ATTRIBUTE_GET( lineColour )
	int pySet_lineColour( PyObject * value );

	PY_WO_ATTRIBUTE_GET( editColour )
	int pySet_editColour( PyObject * value );

	int visibleWidth() const;
	PY_AUTO_METHOD_DECLARE( RETDATA, visibleWidth, END )

	int visibleHeight() const;
	PY_AUTO_METHOD_DECLARE( RETDATA, visibleHeight, END )

	float rowTop(int rowIndex) const;
	PY_AUTO_METHOD_DECLARE( RETDATA, rowTop, ARG(int, END) )

	float columnLeft(int columnIndex) const;
	PY_AUTO_METHOD_DECLARE( RETDATA, columnLeft, ARG(int, END) )

	void toggleFontScale();
	PY_AUTO_METHOD_DECLARE( RETVOID, toggleFontScale, END )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, editEnable, editEnable )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::wstring, editString, editText )

	PY_DEFERRED_ATTRIBUTE_DECLARE( editPrompt )
	PY_DEFERRED_ATTRIBUTE_DECLARE( editRow )
	PY_DEFERRED_ATTRIBUTE_DECLARE( editCol )
	PY_DEFERRED_ATTRIBUTE_DECLARE( editLineLength )
	PY_DEFERRED_ATTRIBUTE_DECLARE( editCallback )

	virtual void		update( float dTime, float relativeParentWidth, float relativeParentHeight );
	virtual void		draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay = true );

	virtual void		colour( uint32 col );

	float				scale() const;
	void				scale( float s );

	bool				editEnable() const;
	void				editEnable( bool on );

	void				editString( const BW::wstring& s );
	const BW::wstring& editString() const;

	virtual void		focus( bool state )	{ this->editEnable(); }
	virtual bool		handleKeyEvent( const KeyEvent & event );

private:

	CallbackConsole *	pConsole_;

	float				dTime_;

	virtual bool		load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	virtual void		save( DataSectionPtr pSect, SaveBindings & bindings );

	COMPONENT_FACTORY_DECLARE( ConsoleGUIComponent() )
};
BW_END_NAMESPACE



#endif // ENABLE_CONSOLES
#endif // CONSOLE_GUI_COMPONENT_HPP
