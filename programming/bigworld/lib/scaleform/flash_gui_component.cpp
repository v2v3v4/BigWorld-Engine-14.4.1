#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "flash_gui_component.hpp"
#include "manager.hpp"
#include "util.hpp"
#include "ashes/simple_gui.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "moo/viewport_setter.hpp"
#include "moo/scissors_setter.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

PY_TYPEOBJECT( FlashGUIComponent )

PY_BEGIN_METHODS( FlashGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FlashGUIComponent )
	/*~ attribute FlashGUIComponent.movie
	 *
	 *	The movie associated with this gui component.
	 *
	 *	@type	PyMovieView
	 */
	PY_ATTRIBUTE( movie )
	
	/*~ attribute FlashGUIComponent.inputKeyMode
	*
	*	Defines key handling mode for component. Available values are:
	*		0 - use BW key handling model without modifications (default)
	*		1 - ignore key handling result from this component (i.e. propagate key event further)
	*		2 - don`t hanlde key event by this component (used to off key event handling for component)
	*
	*	@type	unsigned int
	*/
	PY_ATTRIBUTE(inputKeyMode)
PY_END_ATTRIBUTES()

/*~ function GUI.Flash
 *
 *	This function creates a new FlashGUIComponent which allows a flash movie to be
 *	inserted into the GUI hierarchy. If this component has an attached script, then
 *	the script must take responsibility for passing mouse and key events into the
 *	PyMovieView object.
 *
 *	@param	movie	A PyMovieView object containing the movie to associate with
 *					the component.
 *
 *	@return			the newly created component.
 *
 */
PY_FACTORY_NAMED( FlashGUIComponent, "Flash", GUI )


FlashGUIComponent::FlashGUIComponent( PyMovieView* pMovie, PyTypePlus * pType )
:SimpleGUIComponent( "", pType ),
 pMovieView_( pMovie ),
 inputKeyMode_(0)
{
	BW_GUARD;
	pMovieView_->showAsGlobalMovie( false );

	//Now the gui component controls the drawing/ticking of the movie.
	pMovieView_->visible( true );
}


FlashGUIComponent::~FlashGUIComponent()
{
	BW_GUARD;	
}


/**
 *	Static python factory method
 */
PyObject * FlashGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject * obj = NULL;
	if (!PyArg_ParseTuple( args, "O", &obj ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Flash: "
			"Argument parsing error: Expected a PyMovieView object" );
		return NULL;
	}

	if (!PyMovieView::Check(obj))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Flash: "
			"Argument parsing error: Expected a PyMovieView object" );
		return NULL;
	}
	
	FlashGUIComponent* pFlash = new FlashGUIComponent(static_cast<PyMovieView*>(obj));
	return pFlash;
}


void
FlashGUIComponent::update( float dTime, float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;

	SimpleGUIComponent::update( dTime, relativeParentWidth, relativeParentHeight );
	pMovieView_->pMovieView()->Advance( dTime, 0, false );
}


void FlashGUIComponent::drawSelf( bool reallyDraw, bool overlay )
{
	BW_GUARD;
	if (!reallyDraw)
		return;

	Ptr<Render::Renderer2D> &pRenderer = Manager::instance().pRenderer();
	if (pRenderer == NULL)
		return;

	//This scoped viewport/scissor setter will remember the original settings,
	//and reapply them when we leave the function.
	Moo::ViewportSetter viewportState;
	Moo::ScissorsSetter scissorState;

	Vector2 corners[4];
	SimpleGUIComponent::clipBounds(corners[0], corners[1], corners[2], corners[3] );

	GFx::Viewport gv;
	GRectF rect;
	runtimeGUIViewport( corners, gv, rect );

	pRenderer->BeginFrame();

	pMovieView_->pMovieView()->Capture();
	pMovieView_->pMovieView()->SetViewport( gv );
	GFx::MovieDisplayHandle &movieDisplayHandle = pMovieView_->movieDisplayHandle();
	if (movieDisplayHandle.NextCapture(pRenderer->GetContextNotify()))
		pRenderer->Display(movieDisplayHandle);

	pRenderer->EndFrame();

	//TODO - take this out when we have re-enabled the D3D wrapper,
	//when we use the wrapper, scaleform will be calling via our
	//own setRenderState methods, instead of going straight to the
	//device and causing issues with later draw calls.
	Moo::rc().initRenderStates();
}

bool FlashGUIComponent::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;
	bool handled = SimpleGUIComponent::handleMouseEvent( event );
	
	// If we don't have a script, automatically call it
	if (!handled && !script() && pMovieView_)
	{
		handled = pMovieView_->handleMouseEvent( event );
	}

	return handled;
}

bool FlashGUIComponent::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = SimpleGUIComponent::handleKeyEvent( event );
	
	// If we don't have a script, automatically call it
	if (!handled && !script() && pMovieView_ && 2 != inputKeyMode_)
	{
		handled = pMovieView_->handleKeyEvent( event );
		if (1 == inputKeyMode_ && handled) {
			handled = false;
		}
	}

	return handled;
}

bool FlashGUIComponent::handleMouseButtonEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = SimpleGUIComponent::handleMouseButtonEvent( event );
	
	// If we don't have a script, automatically call it
	if (!handled && !script() && pMovieView_)
	{
		handled = pMovieView_->handleMouseButtonEvent( event );
	}

	return handled;
}

} //namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
// flash_gui_component.cpp