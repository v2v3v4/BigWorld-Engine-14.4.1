#ifndef FLASH_GUI_COMPONENT_HPP
#define FLASH_GUI_COMPONENT_HPP
#if SCALEFORM_SUPPORT

#include "resmgr/datasection.hpp"
#include "ashes/simple_gui_component.hpp"
#include "cstdmf/stdmf.hpp"
#include "config.hpp"
#include "py_movie_view.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

/*~ class GUI.FlashGUIComponent
 *
 *	The FlashGUIComponent is a GUI component used to display a flash
 *	movie on the screen.  It inherits from SimpleGUIComponent.
 *
 *	It delegates all drawing, input handling to flash.  It can have
 *	an associated python script, like any gui components.
 *
 *	The flash movie will draw within the bounds of this gui component.
 *
 *	A new FlashGUIComponent is created using the GUI.Flash function.
 *
 *	For example:
 *	@{
 *	mvDef = Scaleform.createMovie( "scaleform/flash_ui.swf" )
 *	g = GUI.Flash( mvDef.createInstance() )
 *	GUI.addRoot( g )
 *	setattr( g.movie, "_root.actionScriptVariable", "hello world" )
 *	g.movie.invoke( "_root.OpenMenuLevel2" )
 *	@}
 *
 *	This example creates and displays a FlashGUIComponent, sets
 *	and action script text variable to "hello world" then calls
 *	an action script function via the invoke() call.
 */
/**
 *	This class is a scriptable GUI component that displays a flash
 *	movie using scaleform.
 */
class FlashGUIComponent : public SimpleGUIComponent
{
	Py_Header( FlashGUIComponent, SimpleGUIComponent )

public:
	FlashGUIComponent( PyMovieView* pMovieView, PyTypePlus * pType = &s_type_ );
	~FlashGUIComponent();

	//SimpleGUIComponent methods
	void update( float dTime, float relParentWidth, float relParentHeight );
	void drawSelf( bool reallyDraw, bool overlay );
	bool handleKeyEvent( const KeyEvent & /*event*/ );
	bool handleMouseButtonEvent (  const KeyEvent & /*event*/ );
	bool handleMouseEvent( const MouseEvent & /*event*/ );

	PY_FACTORY_DECLARE()
	PY_RO_ATTRIBUTE_DECLARE( pMovieView_.getObject(), movie )
	PY_RW_ATTRIBUTE_DECLARE( inputKeyMode_, inputKeyMode )

protected:
	FlashGUIComponent(const FlashGUIComponent&);
	FlashGUIComponent& operator=(const FlashGUIComponent&);

	PyMovieViewPtr		pMovieView_;

private:
	uint inputKeyMode_;
};

} //namespace ScaleformBW

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT
#endif // FLASH_GUI_COMPONENT_HPP
