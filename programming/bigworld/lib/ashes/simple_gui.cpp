#include "pch.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )

#include "simple_gui.hpp"
#include "simple_gui_component.hpp"

#include "math/colour.hpp"

#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"

#include "input/py_input.hpp"
#include <limits>

#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "simple_gui.ipp"
#endif


/// SimpleGUI Singleton
BW_SINGLETON_STORAGE( SimpleGUI )


namespace { // anonymous

/*
typedef BW::map< int, HCURSOR > CursorMap;
CursorMap s_cursorMap;
*/

// Named constants
const float c_defaultDrgaDistance = 0.002f;

// helper classes

/**
 *	Implements the EventForwarder concept. Used by the processMouseMove
 *	method to forward enter, leave and move events to the default enter, 
 *	leave and move event handlers of the target component.
 */
struct MouseMoveFuncs
{
	bool enterEvent( SimpleGUIComponent * comp, 
		const MouseEvent& event )
	{
		BW_GUARD;
		return comp->handleMouseEnterEvent( event );
	}

	bool moveEvent( SimpleGUIComponent * comp, 
		const MouseEvent & event )
	{
		BW_GUARD;
		return comp->handleMouseEvent( event );
	}

	bool leaveEvent( SimpleGUIComponent * comp, 
		const MouseEvent & event )
	{
		BW_GUARD;
		return comp->handleMouseLeaveEvent( event );
	}
};

/**
 *	Implements the EventForwarder concept. Used by the processDragMove
 *	method to forward enter, leave and move events to the drag'n drop 
 *	enter, leave and move event handlers of the target component.
 */
struct DragMoveFuncs
{
	DragMoveFuncs( SimpleGUIComponent * draggedComp, bool & accepts ) :
		dragged( draggedComp ),
		acceptsDrop( accepts ) 
	{}

	bool enterEvent( SimpleGUIComponent * comp, 
		const MouseEvent & event )
	{
		BW_GUARD;
		this->acceptsDrop = comp->handleDragEnterEvent( this->dragged, event );
		return true;
	}

	bool moveEvent( SimpleGUIComponent *, const MouseEvent & )
	{
		BW_GUARD;
		return true;
	}

	bool leaveEvent( SimpleGUIComponent * comp,
		const MouseEvent & event )
	{
		BW_GUARD;
		return comp->handleDragLeaveEvent( this->dragged, event );
	}

	SimpleGUIComponent * dragged;
	bool & acceptsDrop;
};




// helper functions
int depth_compare_gui_components( const void *arg1, const void *arg2 )
{
	SimpleGUIComponent** e1 = (SimpleGUIComponent**)arg1;
	SimpleGUIComponent** e2 = (SimpleGUIComponent**)arg2;

	//we don't need to return equals to ( ret = 0 ), because we don't care
	if ( (*e1)->position().z > (*e2)->position().z )
		return -1;
	return 1;
}

/**
 *	This method add a simple gui component onto the given 
 *	focus list, meaning the component now receives input events.
 *
 *	@param c			the component that will now receive the input focus.
 *	@param focusList	the focus list rom which to remove the component
 */
void addToFocusList( SimpleGUI::FocusList &focusList, SimpleGUIComponent* c )
{
	BW_GUARD;
	focusList.push_back(c);
}


/**
 *	This method removes a simple gui component from the given focus list.
 *
 *	@param c			the component that no longer wants focus.
 *	@param focusList	the focus list rom which to remove the component
 */
void delFromFocusList( SimpleGUI::FocusList &focusList, SimpleGUIComponent* c )
{
	BW_GUARD_PROFILER( SimpleGUI_delFromFocusList );
	SimpleGUI::FocusList::iterator it = std::find( focusList.begin(), focusList.end(), c );
	if ( it != focusList.end() )
	{
		focusList.erase( it );
	}
}


/**
 *	This static function used in std::sort to sort components according to
 *	their draw order.
 *
 *	@param a	First component
 *	@param b	Second component
 *	@return		true if the first component is greater than the second.
 */
static bool SimpleGUICloserThan ( SimpleGUIComponent* a, SimpleGUIComponent* b )
{
	return a->drawOrder() > b->drawOrder();
}

struct SimpleGuiStatics 
{
	SimpleGuiStatics()
		: inited_( false )
	{
	}

	Moo::EffectConstantValuePtr * s_pTFactorSetter;
	Moo::EffectConstantValuePtr *s_pProjSetter;
	Moo::EffectConstantValuePtr *s_pResolutionSetter;
	Moo::EffectConstantValuePtr *s_pPixelSnapSetter;
	bool inited_;
};

SimpleGuiStatics s_Statics;

SimpleGuiStatics & getStatics()
{
	SINGLETON_MANAGER_WRAPPER_FUNC( SimpleGuiStatics, &getStatics );
	return s_Statics;
}

} // anonymous namespace


/**
 *	Holds drag'n drop information.
 */
struct DragInfo
{
	SimpleGUIComponent *	component;
	SimpleGUIComponent *	target;
	KeyEvent				keyEvent;
	bool					dragging;
	bool                    targetAccepts;
};


/**
 * TODO: to be documented.
 */
class AshesProjSetter : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Matrix wvp( Moo::rc().world() );
		wvp.postMultiply( Moo::rc().viewProjection() );
		pEffect->SetMatrix( constantHandle, &wvp );
		return true;
	}
};

/**
 * TODO: to be documented.
 */
class AshesResolutionSetter : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 res;
		res.x = SimpleGUI::instance().screenWidth();
		res.y = SimpleGUI::instance().screenHeight();
		res.z = SimpleGUI::instance().halfScreenWidth();
		res.w = SimpleGUI::instance().halfScreenHeight();
		pEffect->SetVector( constantHandle, &res );
		return true;
	}
};

/**
 * TODO: to be documented.
 */
class AshesPixelSnapSetter : public Moo::EffectConstantValue
{
public:
	AshesPixelSnapSetter() : value_(false)
	{
	}

	bool value_;

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetBool( constantHandle, this->value_ );
		return true;
	}
};


/**
 * TODO: to be documented.
 */
class TFactorSetter : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetVector( constantHandle, &v4Colour_ );
		return true;
	}

	void colour( DWORD colour )
	{
		v4Colour_ = Colour::getVector4Normalised(colour);
	}
private:
	Vector4	v4Colour_;
};

/**
 *	Constructor.
 */
SimpleGUI::SimpleGUI() :
	crossFocusFrame_( -1 ),
	dragFocusFrame_( -1 ),
	pMouseCursor_( NULL ),
	resolutionHasChanged_( true ),
	lastResolution_( 0.f, 0.f ),
	lastRealResolution_( 0.f, 0.f ),
	realResolutionCounter_( 0 ),
	mouseButtonFocusList_(),
	crossFocusList_(),
	dragFocusList_(),
	dropFocusList_(),
	clickComponent_( NULL ),
	dragInfo_( NULL ),
	dragDistanceSqr_( c_defaultDrgaDistance * c_defaultDrgaDistance ),
	resolutionOverride_( 0.f, 0.f ),
	hwnd_( NULL ),
	hInstance_( NULL ),
	appFocus_( true ),
	updateGUI_( true ),
	drawCallCount_(0),
	legacyKeyboard_(false),
	guiTreeCookie_(1),
	guiDrawContext_( NULL )
{	
	BW_GUARD;
	clipFixer_.setIdentity();
	clipStack_.push( Vector4( -1.f, 1.f, 1.f, -1.f ) );

	MF_WATCH( "GUI/numDrawCalls", drawCallCount_, Watcher::WT_READ_ONLY,
			"Number of draw calls in the last frame." );
}


/**
 *	Destructor.
 */
SimpleGUI::~SimpleGUI()
{
	BW_GUARD;
	if (Moo::rc().device() != NULL)
	{
		bw_safe_delete( guiDrawContext_ );
		Py_XDECREF( pMouseCursor_ );
		pMouseCursor_ = NULL;
	}

}


/**
 *	This static method creates the instance, and also initialises related
 *	static objects.
 *
 *	@param pConfig	Configuration data section for SimpleGUIComponent.
 */
/*static*/ void SimpleGUI::init( DataSectionPtr pConfig )
{
	BW_GUARD;
	// This local variable is not used because the base Singleton class will
	// store a pointer internally, so we don't need to store it.
	SimpleGUI* pNewInstance = new SimpleGUI();

	SimpleGUIComponent::init( pConfig );

	REGISTER_SINGLETON_FUNC( SimpleGuiStatics, &getStatics );
	SimpleGuiStatics & statics = getStatics();
	statics.s_pProjSetter =	new Moo::EffectConstantValuePtr( new AshesProjSetter() );
	statics.s_pResolutionSetter = new Moo::EffectConstantValuePtr( new AshesResolutionSetter() );
	statics.s_pTFactorSetter = new Moo::EffectConstantValuePtr( new TFactorSetter() );
	statics.s_pPixelSnapSetter = new Moo::EffectConstantValuePtr( new AshesPixelSnapSetter() );

	statics.inited_ = true;
}


/**
 *	This method sets the HWND for the main application window.
 *	SimpleGUI needs this only if the mouse cursor is to be used.
 *
 *	@param h	The HWND for the application
 */
void SimpleGUI::hwnd( void * h )
{
	hwnd_ = h;
}


/**
 *	Returns the HWND for the main application window.
 */
void * SimpleGUI::hwnd() const
{
	return hwnd_;
}


/**
 * Sets the application instance handle.
 *
 *	@param h	The application instance for the application
 */ 
void SimpleGUI::hInstance( void * h )
{
	hInstance_ = h;
}


/**
 *	This method adds a simple component to the GUI.
 *
 *	@param c	The component to add.
 */
void
SimpleGUI::addSimpleComponent( SimpleGUIComponent& c )
{
	BW_GUARD_PROFILER( SimpleGUI_addSimpleComponent );
	Components::iterator it = std::find( components_.begin(), components_.end(), &c );

	if ( it == components_.end() )
	{
		Py_INCREF( &c );
		components_.push_back( &c );
		reSort();
	}
	else
	{
		WARNING_MSG( "SimpleGUI::addSimpleComponent - \
			attempted to add a component to the root twice\n" );
	}
}


/**
 *	This method returns a simple component from the GUI.
 *
 *	@param c	The component to remove.
 */
void
SimpleGUI::removeSimpleComponent( SimpleGUIComponent& c )
{
	BW_GUARD_PROFILER( SimpleGUI_removeSimpleComponent );
	Components::iterator it = std::find( components_.begin(), components_.end(), &c );

	if ( it != components_.end() )
	{
		if ( (*it)->allowDelete() )
		{
			Py_DECREF( *it );
			components_.erase( it );
			reSort();
		}
	}
}

/**
 *	This method removes all components from the GUI root.
 */
void
SimpleGUI::clearSimpleComponents( void )
{
	Components::iterator it = components_.begin();
	Components::iterator end = components_.end();

	while (it != end)
	{
		Py_DECREF( *it );
		it++;
	}
	PyGC_Collect();

	components_.clear();
}


/**
 *	This method resorts the components in the GUI.
 */
void
SimpleGUI::reSort( void )
{
	BW_GUARD;
	if ( !components_.empty() )
	{
		qsort( &components_.front(), components_.size(), sizeof( SimpleGUIComponent* ), depth_compare_gui_components );
	}

	recalcDrawOrders();
}


/**
 *	Ticks the SimpleGUI system.
 *
 *	@param	dTime	Time elapsed since last update.
 */
void SimpleGUI::update( float dTime )
{
	BW_GUARD;
	dTime_ = dTime;
	mouseCursor().tick( dTime );
}


/**
 *	This static method cleans up the SimpleGUI singleton instance and related
 *	static objects. It should be called before Script::fini.
 */
/*static*/ void SimpleGUI::fini()
{
	BW_GUARD;
	SimpleGuiStatics & statics = getStatics();
	if (!statics.inited_)
	{
		return;
	}

	statics.inited_ = false;

	delete pInstance();

	SimpleGUIComponent::fini();

	bw_safe_delete(statics.s_pTFactorSetter);
	bw_safe_delete(statics.s_pProjSetter);
	bw_safe_delete(statics.s_pResolutionSetter);
	bw_safe_delete(statics.s_pPixelSnapSetter);
}


/**
 *	This method returns the mouse cursor object, or creates it if it's NULL.
 *
 *  @return		The mouse cursor object.
 */
MouseCursor & SimpleGUI::internalMouseCursor() const
{
	BW_GUARD;
	MF_ASSERT_DEV( getStatics().inited_ );

	if (!pMouseCursor_)
	{
		pMouseCursor_ = new MouseCursor();
	}
	return *pMouseCursor_;
} 


/**
 *	This method returns the const mouse cursor object.
 *
 *  @return		The mouse cursor object.
 */
const MouseCursor & SimpleGUI::mouseCursor() const
{
	BW_GUARD;
	return internalMouseCursor();
}


/**
 *	This method returns the non-const mouse cursor object.
 *
 *  @return		The mouse cursor object.
 */
MouseCursor & SimpleGUI::mouseCursor()
{
	BW_GUARD;
	return internalMouseCursor();
}


void SimpleGUI::setConstants( DWORD colour, bool pixelSnap )
{
	BW_GUARD;
	static Moo::EffectConstantValuePtr* pProjConstantValue_ = NULL;
	static Moo::EffectConstantValuePtr* pTFactorConstantValue_ = NULL;
	static Moo::EffectConstantValuePtr* pResolutionConstantValue_ = NULL;
	static Moo::EffectConstantValuePtr* pPixelSnapSetter_ = NULL;	

	if ( !pProjConstantValue_ )
	{
		pProjConstantValue_ = Moo::rc().effectVisualContext().getMapping( "WorldViewProjection" );
		pTFactorConstantValue_ = Moo::rc().effectVisualContext().getMapping( "GUIColour" );
		pResolutionConstantValue_ = Moo::rc().effectVisualContext().getMapping( "GUIResolution" );
		pPixelSnapSetter_ = Moo::rc().effectVisualContext().getMapping( "GUIPixelSnap" );		
	}

	SimpleGuiStatics & statics = getStatics();
	((TFactorSetter*)(*statics.s_pTFactorSetter).getObject())->colour( colour );
	((AshesPixelSnapSetter *)(*statics.s_pPixelSnapSetter).getObject())->value_ = pixelSnap;
	*pProjConstantValue_ = *statics.s_pProjSetter;
	*pTFactorConstantValue_ = *statics.s_pTFactorSetter;
	*pResolutionConstantValue_ = *statics.s_pResolutionSetter;
	*pPixelSnapSetter_ = *statics.s_pPixelSnapSetter;
	Moo::rc().setFVF( GUIVertex::fvf() );
}


/**
 *	Determines if a given SimpleGUIComponent is in the current SimpleGUI tree
 */
bool SimpleGUI::existsInTree( const SimpleGUIComponent * pComponent ) const
{
	BW_GUARD;
	Components::const_iterator it;
	Components::const_iterator end = components_.end();

	for ( it = components_.begin(); it != end; ++it )
	{
		if( (*it) == pComponent || (*it)->hasChild( pComponent, true ) )
		{
			return true;
		}
	}
	return false;
}


/**
 *	Draws all GUI components.
 */
void SimpleGUI::draw()
{
	BW_GUARD;
	if (!updateGUI_)
	{
		return;
	}

	static DogWatch	dwUpdateGUI("Update");
	dwUpdateGUI.start();

	//Update
	Moo::rc().getViewport( &originalView_ );

	Vector2 newResolution;
	if ( usingResolutionOverride() )
	{
		newResolution = resolutionOverride_;
	}
	else
	{
		newResolution.set( Moo::rc().screenWidth(), Moo::rc().screenHeight() );
	}

	if ( newResolution.x != lastResolution_.x ||
		newResolution.y != lastResolution_.y )
	{
		lastResolution_ = newResolution;
		resolutionHasChanged_ = true;
	}
	else
	{
		resolutionHasChanged_ = false;
	}

	Vector2 realResolution( Moo::rc().screenWidth(), Moo::rc().screenHeight() );
	if ( realResolution.x != lastRealResolution_.x ||
		realResolution.y != lastRealResolution_.y )
	{
		realResolutionCounter_++;
		lastRealResolution_ = realResolution;
	}

	Components::iterator it = components_.begin();
	Components::iterator end = components_.end();

	while (it != end)
	{
		SimpleGUIComponent* c = (*it);

		c->update( dTime_, newResolution.x, newResolution.y );
		c->applyShaders( dTime_ );

		it++;
	}

	dwUpdateGUI.stop();

	static DogWatch	dwDrawGUI("Draw");
	dwDrawGUI.start();

	drawCallCount_ = 0;

	//Draw
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	Matrix origView = Moo::rc().view();
	Matrix origProj = Moo::rc().projection();
	Moo::rc().view( Matrix::identity );
	Moo::rc().projection( Matrix::identity );
	Moo::rc().updateViewTransforms();

	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Matrix::identity );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Matrix::identity );

	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( GUIVertex::fvf() );

	if (!guiDrawContext_)
	{
		guiDrawContext_ = new Moo::DrawContext( Moo::RENDERING_PASS_COLOR );
	}

	guiDrawContext_->begin( Moo::DrawContext::ALL_CHANNELS_MASK );
	Moo::rc().push();
	Moo::rc().world(Matrix::identity);

	it = components_.begin();
	end = components_.end();

	while ( it != end )
	{
		SimpleGUIComponent* c = (*it);

		c->draw( *guiDrawContext_, true );

		it++;
	}

	Moo::rc().pop();

	guiDrawContext_->end( Moo::DrawContext::ALL_CHANNELS_MASK );
	guiDrawContext_->flush( Moo::DrawContext::ALL_CHANNELS_MASK );

	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setViewport( &originalView_ );
	MF_ASSERT_DEV( clipStack_.size() == 1 );

	Moo::rc().view( origView );
	Moo::rc().projection( origProj );
	Moo::rc().updateViewTransforms();

	dwDrawGUI.stop();

	tickCrossFocusEvents();
	tickDragFocusEvents();
}

void SimpleGUI::appFocus( bool state )
{
	BW_GUARD;
	appFocus_ = state;
}

bool SimpleGUI::appFocus() const
{
	BW_GUARD;
	return appFocus_;
}

void SimpleGUI::setUpdateEnabled( bool enable )
{
	BW_GUARD;
	updateGUI_ = enable;
}

/**
 *	Converts coordinates from screen space (pixels) to clip scape.
 *
 *  @param w		horizontal position in screen space (input)
 *  @param h		vertical position in screen space (input)
 *  @param retW	horizontal position in clip space (output, optional)
 *  @param retH	vertical position in clip space (output, optional)
 */
void
SimpleGUI::pixelRangesToClip( float w, float h, float* retW, float* retH )
{
	BW_GUARD;
	Vector2 pixelToClip( this->pixelToClip() );

	if ( retW )
		*retW = w * pixelToClip.x;

	if ( retH)
		*retH = h * pixelToClip.y;
}


/**
 *	Converts coordinates from clip space to screen scape (pixels).
 *
 *  @param w		horizontal position in clip space (input)
 *  @param h		vertical position in clip space (input)
 *  @param retW	horizontal position (output, optional)
 *  @param retH	vertical position (output, optional)
 */
void
SimpleGUI::clipRangesToPixel( float w, float h, float* retW, float* retH )
{
	BW_GUARD;
	Vector2 pixelToClip( this->pixelToClip() );

	if ( retW )
		*retW = w / pixelToClip.x;
	if ( retH )
		*retH = h / pixelToClip.y;
}


/**
 *  This method returns a scaling factor giving pixel to clip coordinates.
 */
Vector2 SimpleGUI::pixelToClip() const
{	
	BW_GUARD;
	Vector2 res( this->screenResolution() );

	MF_ASSERT( !almostZero(res.x) )
	MF_ASSERT( !almostZero(res.y) )

	res.x = 2.f / res.x;
	res.y = 2.f / res.y;
	return res;
};

/**
 *	This method handles key events for the gui system.
 *
 *	Key events are passed on to the current component that
 *	has the focus, if any.
 *
 *	@param event	The key event to handle.
 *
 *	@return True if the key event was handled.
 */
bool SimpleGUI::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = false;
	Vector2 mousePos = event.cursorPosition();

	if (mouseCursor().isActive() && event.isMouseButton() )
	{
		SimpleGUIComponent * pComponent =
									closestHitTest( mouseButtonFocusList_, mousePos );
		if (pComponent != NULL)
		{
			handled = pComponent->handleMouseButtonEvent( event );
		}

		handled |= this->processClickKey( event );
		handled |= this->processDragKey( event );
	}

	if (!handled)
	{
		for ( size_t n = components_.size(); n > 0; --n )
		{
			SimpleGUIComponent * pComponent = components_[ n - 1 ];
			handled = pComponent->handleKeyEvent( event );
			if (handled)
			{
				break;
			}
		}
	}

    return handled;
}

/**
 *	This method handles mouse events for the gui system.
 *
 *	Mouse events are passed on to the whatever component
 *	is at the mouse location.
 *
 *	@param event	The mouse event to handle.
 *
 *	@return True if the mouse event was handled.
 */
bool SimpleGUI::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (mouseCursor().isActive())
	{
		Vector2 mousePos = mouseCursor().position();
		handled = this->processDragMove( event );

		if (!handled) 
		{
			handled = this->processMouseMove( event ); 
		}
	}

	return handled;
}


/**
 *	This method handles axis events for the gui system.
 *
 *	Axis events are passed on to the current component that
 *	has the focus, if any.
 *
 *	@param event	The axis event to handle.
 *
 *	@return True if the axis event was handled.
 */
bool SimpleGUI::handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	bool handled = false;

	for ( size_t n = components_.size(); n > 0; --n )
	{
		SimpleGUIComponent* c = components_[n - 1];
        handled = c->handleAxisEvent( event );
        if (handled)
            break;
    }

    return handled;
}


/**
 *	This function fills "result" with components from "input" that are in the
 *	GUI hierarchy, as roots or as children.
 *
 *	@param input	list to filter.
 *	@param result	filtered components are returned in this param
 */
void SimpleGUI::filterList( const FocusList& input, FocusList& result )
{
	BW_GUARD;

	guiTreeCookie_++;

	for ( SimpleGUI::Components::iterator c = components_.begin();
		c != components_.end(); ++c )
	{
		SimpleGUIComponent* sgc = *c;
		sgc->guiTreeCookieRecursive( guiTreeCookie_ );
	}

	// Now filter the input list into the result list, adding only elements
	// from the input list that actually belong to the GUI hierarchy.
	result.reserve( input.size() );
	for ( SimpleGUI::FocusList::const_iterator i = input.begin();
		i != input.end(); ++i )
	{
		SimpleGUIComponent* sgc = *i;
		if (sgc->guiTreeCookie() == guiTreeCookie_)
		{
			result.push_back( sgc );
		}
	}
}


/**
 *	Template function that processes mouse events over two lists of components
 *	to detect enter and leave (cross) events. It is used to generate events during 
 *	normal and dragged mouse hovering. It relies on a EventForwarder concept to 
 *	deliver the detected events.
 *
 *	@param	mcursor					the mouse cursor object
 *	@param	roots					list of root components. The crossFocusList
 *									param gets filtered to a list that includes
 *									only components that are root or children of
 *									a root.
 *	@param	crossFocusList			list of components to check for cross events.
 *	@param	currentOverComponent	the component that should be considered as the 
 *									component the mouse is currently over. This will
 *									be decref'ed if the mouse is no longer over it.
 *	@param	eventForwarder			the event forwarder object.
 *
 *	@return	New reference to the SimpleGUIComponent* that the mouse is now over.
 */
template< class EventForwarderT >
WeakPyPtr< SimpleGUIComponent > SimpleGUI::detectEnterLeaveEvents(	MouseCursor& mcursor,														
														const SimpleGUI::FocusList & crossFocusList,
														WeakPyPtr< SimpleGUIComponent > currentOverComponent,
														EventForwarderT eventForwarder)
{
	BW_GUARD;
	static const Vector2 kOutOfBoundsPos( std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );

	Vector2 mousePos( kOutOfBoundsPos );
	SimpleGUIComponent* mouseOverComponent = NULL;

	// Only detect current mouse over (and therefore raise an enter event) if the cursor 
	// is visible. If it is not visible, then mouseOverComponent will always be NULL and
	// any previous mouse over component will receive a leave event.
	if ( mcursor.isActive() )
	{
		mousePos = mcursor.position();

		// first filter out cross focus components that are not in any root
		// hierarchy, and sort it from top-most to bottom-most.
		static SimpleGUI::FocusList filteredList;
		this->filterList( crossFocusList, filteredList );
		std::sort<SimpleGUI::FocusList::iterator>(
			filteredList.begin(), filteredList.end(), SimpleGUICloserThan );

		// find the item currently below the cursor			
		for ( SimpleGUI::FocusList::iterator i = filteredList.begin();
			i != filteredList.end() && mouseOverComponent == NULL; ++i )
		{
			if ( (*i)->hitTest( mousePos ) )
			{
				mouseOverComponent = (*i);
			}
		}

		filteredList.clear();
	}

	WeakPyPtr< SimpleGUIComponent > newOverComponent (mouseOverComponent);

	if ( newOverComponent.getObject() != currentOverComponent.getObject() )
	{
		MouseEvent event;
		event.fill( 0, 0, 0, mousePos );

		// generate leave event
		if ( currentOverComponent.good() )
		{
			eventForwarder.leaveEvent( currentOverComponent.getObject(), event );
		}

		// generate enter event
		if ( newOverComponent.good() )
		{
			eventForwarder.enterEvent( newOverComponent.getObject(), event );
		}

		currentOverComponent = newOverComponent;		
	}

	return WeakPyPtr< SimpleGUIComponent >( currentOverComponent );
}


/**
 *	Helper method to get the closest element that hitTests true for a position
 *	in a focus list.
 *
 *	@param list	focus list with the components to hit-test
 *	@param pos	position for the hit test, usually the mouse cursor position
 *
 *  @return the closest component that hit-tests to true for the position, or
 *          NULL if no component hit-tests
 */
SimpleGUIComponent* SimpleGUI::closestHitTest(
	FocusList& list, const Vector2 & pos )
{
	BW_GUARD;
	// first filter out cross focus components that are not in any root
	// hierarchy
	FocusList filteredList;
	this->filterList( list, filteredList );

	SimpleGUIComponent* c = NULL;
	for ( FocusList::reverse_iterator i = filteredList.rbegin();
		i != filteredList.rend(); ++i )
	{
		if ( (*i)->hitTest( pos ) )
		{
			if ( c == NULL || c->drawOrder() < (*i)->drawOrder() )
			{
				c = (*i);
			}
		}
	}
	return c;
}


/**
 * Process a key event, looking for possible mouse click events.
 *
 *	@param event	the input key event.
 *
 *	@
 */
bool SimpleGUI::processClickKey( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (event.key() == KeyCode::KEY_LEFTMOUSE)
	{
		if (event.isKeyDown()) 
		{
			this->clickComponent_ = closestHitTest( mouseButtonFocusList_, event.cursorPosition() );
		}
		// not a click event if drag is happening
		else if (this->clickComponent_ != NULL && 
				(this->dragInfo_.get() == NULL || !this->dragInfo_->dragging) &&
					this->clickComponent_->hitTest( event.cursorPosition() ))
		{
			// This is a click event
			handled = this->clickComponent_->handleMouseClickEvent( event );
			this->clickComponent_ = NULL;
		}
	}

	return handled;
}


/**
 *	Process a key event, looking for possible drag'n drop events.
 *
 *	@param event	the input key event.
 */
bool SimpleGUI::processDragKey( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = true;

	if (event.key() == KeyCode::KEY_LEFTMOUSE)
	{
		if (event.isKeyDown()) 
		{
			SimpleGUIComponent* c = closestHitTest( dragFocusList_, event.cursorPosition() );
			if ( c != NULL )
			{
				// store button down position 
				// for possible drag operation
				this->dragInfo_.reset( new DragInfo );
				this->dragInfo_->keyEvent      = event;
				this->dragInfo_->component     = c;
				this->dragInfo_->target        = NULL;
				this->dragInfo_->dragging      = false;
				this->dragInfo_->targetAccepts = false;
				this->currentDropFocusComponent_ = NULL;
			}
		}
		else if (this->dragInfo_.get() != NULL)
		{
			SimpleGUIComponent * component = this->dragInfo_->component;

			// Dragging is under way. 
			// This is a drop event
			if (this->dragInfo_->dragging)
			{
				SimpleGUIComponent * target  = this->dragInfo_->target;
				if (target != NULL && this->dragInfo_->targetAccepts)
				{
					target->handleDropEvent( component, event );
				}
				// component may have been deleted 
				// when processing drop event
				if (this->dragInfo_.get() != NULL)
				{
					component->handleDragStopEvent( event );
				}
			}
			this->dragInfo_.reset( NULL );
			this->currentDropFocusComponent_ = NULL;
		}
	}

	return this->dragInfo_.get() != NULL;
}


/**
 *	Process a mouse event, passing the move event to the first component that processes it.
 *
 *	@param event	the input mouse event.
 */
bool SimpleGUI::processMouseMove( const MouseEvent & event )
{
	BW_GUARD;
	// When the mouse moves we want to make sure that enter/leave events happen before 
	// the handleMouseEvent on this frame.
	tickCrossFocusEvents();
	tickDragFocusEvents();

	// Send mouse move to the first component that handles it.
	// We just iterate through roots - handleMouseEvent on the component
	// will recurse into its children.
	for ( SimpleGUI::Components::const_reverse_iterator i = components_.rbegin();
		i != components_.rend(); ++i )
	{
		SimpleGUIComponent* c = (*i);

		if ( c->hitTest( event.cursorPosition() ) &&
			 c->handleMouseEvent( event ) )
		{
			return true;
		}
	}

	return false;
}


/**
 *	Process a mouse event, looking for possible drag'n drop events.
 *
 *	@param event	the input mouse event.
 */
bool SimpleGUI::processDragMove( const MouseEvent & event )
{
	BW_GUARD;
	if (this->dragInfo_.get() != NULL)
	{
		// Mouse button down has been detected but 
		// the drag operation itself not yet started
		if (!this->dragInfo_->dragging )
		{
			const Vector2 &dragStartPos = this->dragInfo_->keyEvent.cursorPosition();
			if ((dragStartPos - event.cursorPosition()).lengthSquared() > this->dragDistanceSqr_)
			{
				if (this->dragInfo_->component->handleDragStartEvent( 
						this->dragInfo_->keyEvent ))
				{
					this->dragInfo_->dragging = true;
				}
				else 
				{
					this->dragInfo_.reset( NULL );
				}				
			}
		}
	}

	return this->dragInfo_.get() != NULL;
}


/**
 *	This method add a simple gui component onto the
 *	focus list, meaning the component now receives button events.
 *
 *	@param c	the component that will now receive the input focus.
 */
void SimpleGUI::addInputFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
}


/**
 *	This method removes a simple gui component from the focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delInputFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
}

/**
 *	This method add a simple gui component onto the
 *	focus list, meaning the component now receives button events.
 *
 *	@param c	the component that will now receive the input focus.
 */
void SimpleGUI::addMouseButtonFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	addToFocusList( this->mouseButtonFocusList_, c );
}


/**
 *	This method removes a simple gui component from the focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delMouseButtonFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	delFromFocusList( this->mouseButtonFocusList_, c );
	if (c == this->clickComponent_)
	{
		this->clickComponent_ = NULL;
	}
}


/**
 *	This method recalculates the draw order of the components in the GUI.
 */
void SimpleGUI::recalcDrawOrders()
{
	BW_GUARD;
	// recalculate the draw orders
	uint32 currDrawOrder = 0;

	Components::iterator it = components_.begin();
	Components::iterator end = components_.end();

	while (it != end)
	{
		currDrawOrder = (*it++)->calcDrawOrderRecursively( currDrawOrder, 0 );
	}
}

/**
 *	This method goes through each component that has dragFocus enabled and calls enter and
 *	leave events as neccessary.
 */
void SimpleGUI::tickCrossFocusEvents()
{	
	BW_GUARD;
	// If we've already ticked this frame, then don't do anything
	if ( crossFocusFrame_ == Moo::rc().frameTimestamp() )
	{
		return;
	}

	crossFocusFrame_ = Moo::rc().frameTimestamp();

	currentCrossFocusComponent_ = 
		detectEnterLeaveEvents(	mouseCursor(), crossFocusList_, 
								currentCrossFocusComponent_, MouseMoveFuncs() );
}

/**
 *	This method goes through each component that has dragFocus enabled and calls dragEnter and
 *	dragLeave events as neccessary.
 */
void SimpleGUI::tickDragFocusEvents()
{
	BW_GUARD;
	bool dragging = (dragInfo_.get() != NULL && dragInfo_->dragging);

	// If we've already ticked this frame, then don't do anything
	if (dragFocusFrame_ == Moo::rc().frameTimestamp() || !dragging)
	{
		return;
	}

	dragFocusFrame_ = Moo::rc().frameTimestamp();

	currentDropFocusComponent_ = 
		detectEnterLeaveEvents(	mouseCursor(), dropFocusList_, 
								currentDropFocusComponent_,
								DragMoveFuncs( 
									this->dragInfo_->component,
									this->dragInfo_->targetAccepts ) );

	if (dragInfo_.get() != NULL)
	{
		dragInfo_->target = currentDropFocusComponent_.getObject();
	}
}


/**
 *	This method add a simple gui component onto the mouse cross focus list, 
 *	meaning the component now receives mouse enter and mouse leave events.
 *
 *	@param c	the component that will now receive the input focus.
 */
void SimpleGUI::addMouseCrossFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	addToFocusList( this->crossFocusList_, c );
}


/**
 *	This method removes a simple gui component from the 
 *	mouse cross focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delMouseCrossFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	delFromFocusList( this->crossFocusList_, c );
}


/**
 *	This method add a simple gui component onto the mouse move focus 
 *	list, meaning the component now receives button move events.
 *
 *	@param c	the component that will now receive the input focus.
 */
void SimpleGUI::addMouseMoveFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	// now handled through the hierarchy
}


/**
 *	This method removes a simple gui component from the 
 *	mouse move focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delMouseMoveFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	// now handled through the hierarchy
}

/**
 *	This method add a simple gui component onto the drag focus list, 
 *	meaning the component now receives drag start events.
 *
 *	@param c	the component that will now receive the drag focus.
 */
void SimpleGUI::addMouseDragFocus( SimpleGUIComponent* c )
{
	addToFocusList( this->dragFocusList_, c );
}


/**
 *	This method removes a simple gui component from the drag focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delMouseDragFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	delFromFocusList( this->dragFocusList_, c );
	if (this->dragInfo_.get() != NULL && this->dragInfo_->component == c)
	{
		this->dragInfo_.reset( NULL );
	}
}


/**
 *	This method add a simple gui component onto the drop focus list, 
 *	meaning the component now receives drop events.
 *
 *	@param c	the component that will now receive the drop focus.
 */
void SimpleGUI::addMouseDropFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	addToFocusList( this->dropFocusList_, c );
}


/**
 *	This method removes a simple gui component from the drop focus list.
 *
 *	@param c	the component that no longer wants focus.
 */
void SimpleGUI::delMouseDropFocus( SimpleGUIComponent* c )
{
	BW_GUARD;
	delFromFocusList( this->dropFocusList_, c );
	if (this->dragInfo_.get() != NULL && this->dragInfo_->target == c)
	{
		this->dragInfo_->target = NULL;		
		this->dragInfo_->dragging = false;
		this->dragInfo_->targetAccepts = false;
	}
}

// -----------------------------------------------------------------------------
// Section: Clip region methods
// -----------------------------------------------------------------------------

/**
 *	This method pushes a gui component's area as the current clipping region.
 *
 *	@param c		The simple gui component whose area defines the new clip region.
 *	@return false	if the region is NULL and therefore drawing shouldn't continue.
 */
bool SimpleGUI::pushClipRegion( SimpleGUIComponent& c )
{
	BW_GUARD;
	int nVerts;
	GUIVertex* verts = c.vertices( &nVerts );

	//Current implementation only allows setting a clip region from
	//a very simple gui component.
	MF_ASSERT_DEV( nVerts == 4);

	//note we need to transform the clipping region by the current transform.
	//this allows for nested ( and scrolling ) clip regions.
	const Matrix& w = Moo::rc().world();

	Vector3 v1 = w.applyPoint(verts[1].pos_);
	Vector3 v3 = w.applyPoint(verts[3].pos_);

	// The bottom-right of the rect is shifted down one pixel, since D3D RECT's are right hand exclusive.
	Vector2 pixelToClip( this->pixelToClip() );
	Vector4 a( v1.x, v3.y, v3.x+pixelToClip.x, v1.y-pixelToClip.y );

	return this->pushClipRegion(a);
}


/**
 *	This method pushes a clipping region.
 *
 *	@param cr		The clip region, (x,y,x,y)
 *	@return false	if the region is NULL and therefore drawing shouldn't continue.
 */
bool SimpleGUI::pushClipRegion( const Vector4& cr )
{
	BW_GUARD;
	Vector4 a(cr);

	//if there is an existing clip region, then clip the current one to it.
	if ( !clipStack_.empty() )
	{
		Vector4 b = clipStack_.top();
		a.x = max( a.x, b.x );
		a.y = min( a.y, b.y );
		a.z = min( a.z, b.z );
		a.w = max( a.w, b.w );
	}

	clipStack_.push( a );

	bool success = commitClipRegion();

	if ( !success )
		clipStack_.pop();

	return success;
}


/**
 *	This private method commits the current clip region, either as a
 *	scissors rectangle ( xbox ) or by using an elaborate fudge with
 *	viewports.
 */
bool SimpleGUI::commitClipRegion()
{
	BW_GUARD;
	if ( clipStack_.empty() )
	{
		Moo::rc().setRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
		return true;
	}

	Vector4 top = clipStack_.top();
	D3DRECT region;
	region.x1 = (LONG)(top.x * Moo::rc().halfScreenWidth() + Moo::rc().halfScreenWidth());
	region.y1 = (LONG)(top.y * -Moo::rc().halfScreenHeight() + Moo::rc().halfScreenHeight());
	region.x2 = (LONG)(top.z * Moo::rc().halfScreenWidth() + Moo::rc().halfScreenWidth());
	region.y2 = (LONG)(top.w * -Moo::rc().halfScreenHeight() + Moo::rc().halfScreenHeight());

	//right now, windows are always anchored in the middle.
	Vector2 center( (top.x + top.z)/2.f, (top.y + top.w)/2.f );

	if ( ( region.x2 - region.x1 > 0.f ) && ( region.y2 - region.y1 > 0.f ) )
	{
		RECT rect;
		rect.left = region.x1;
		rect.right = region.x2;
		rect.top = region.y1;
		rect.bottom = region.y2;

		Moo::rc().device()->SetScissorRect( &rect );
		return ( Moo::rc().setRenderState( D3DRS_SCISSORTESTENABLE, TRUE ) == S_OK );
	}
	else
		return false;
}


/**
 *	This method pops the current clipping region.
 */
void SimpleGUI::popClipRegion()
{
	BW_GUARD;
	MF_ASSERT_DEV( !clipStack_.empty() );

	if( !clipStack_.empty() )
		clipStack_.pop();

	commitClipRegion();
}


/**
 *	This method returns the current clip region, as a vector4
 *	of ( left, top, right, bottom ) in clip coords
 */
const Vector4& SimpleGUI::clipRegion()
{
	BW_GUARD;
	static Vector4 fullscreen( -1.f, 1.f, 1.f, -1.f );

	if ( clipStack_.empty() )
		return fullscreen;
	else
		return clipStack_.top();
}


/**
 *	This method returns whether or not the given point is
 *	within the current clip region or not.
 */
bool SimpleGUI::isPointInClipRegion( const Vector2& pt )
{
	BW_GUARD;
	if ( clipStack_.empty() )
		return true;
	else
	{
		const Vector4& clipRgn = clipStack_.top();
		//clipRgn is (x,y,x,y)
		return ( pt.x >= clipRgn.x && pt.y <= clipRgn.y && pt.x <= clipRgn.z && pt.y >= clipRgn.w );
	}
}


/**
 * Sets the current resolution override. This overrides what the GUI system thinks
 * the current resolution is (good for scaling UI's that use PIXEL layout modes).
 * Setting this to a zero vector disables the override
 */
void SimpleGUI::resolutionOverride( const Vector2& res)
{
	BW_GUARD;
	resolutionOverride_ = res;
}

// -----------------------------------------------------------------------------
// Section: Script methods
// -----------------------------------------------------------------------------

PY_MODULE_STATIC_METHOD( SimpleGUI, addRoot, GUI )
PY_MODULE_STATIC_METHOD( SimpleGUI, delRoot, GUI )
PY_MODULE_STATIC_METHOD( SimpleGUI, reSort, GUI )
PY_MODULE_STATIC_METHOD( SimpleGUI, roots, GUI )


/*~ function GUI.addRoot
 *	@components{ client, tools }
 *
 *	This function adds a GUI component to the gui root.  GUI elements exist in
 *	a treelike structure, with every element having a parent.  The top level
 *	parent is the root of the whole tree.  This root exists internal to the GUI
 *	module.	In order for a gui component to be processed, for both rendering
 *	and event handling, it, or one of its parents, must be added to the root.
 *
 *	@param component	the gui component to add to the root
 */
/**
 *	This method is a script wrapper to the C++ method.
 */
PyObject * SimpleGUI::py_addRoot( PyObject * args )
{
	BW_GUARD;
	PyObject	* pComponent;
	if (!PyArg_ParseTuple( args, "O", &pComponent ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.addRoot: "
			"Argument parsing error: Expected a GUI component" );
		return NULL;
	}

	if (PyWeakref_CheckProxy( pComponent))
	{
		pComponent = (PyObject*) PyWeakref_GET_OBJECT( pComponent );
	}
	if (!SimpleGUIComponent::Check( pComponent ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.addRoot: "
			"Argument parsing error: Expected a GUI component(weak ref may be gone)" );
		return NULL;
	}

	SimpleGUI::instance().addSimpleComponent(
		*(SimpleGUIComponent*)pComponent );

	Py_RETURN_NONE;
}


/*~ function GUI.delRoot
 *	@components{ client, tools }
 *
 *	This function removes a Gui component from the root gui's list of children.
 *
 *	@param	component	the component to remove
 */
/**
 *	This method is a script wrapper to the C++ method.
 */
PyObject * SimpleGUI::py_delRoot( PyObject * args )
{
	BW_GUARD;
	PyObject	* pComponent;
	if (!PyArg_ParseTuple( args, "O", &pComponent ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.delRoot: "
			"Argument parsing error: Expected a GUI component" );
		return NULL;
	}

	if (PyWeakref_CheckProxy( pComponent))
	{
		pComponent = (PyObject*) PyWeakref_GET_OBJECT( pComponent );
	}
	if (!SimpleGUIComponent::Check( pComponent ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.delRoot: "
			"Argument parsing error: Expected a GUI component(weak ref may be gone)" );
		return NULL;
	}

	SimpleGUI::instance().removeSimpleComponent(
		*(SimpleGUIComponent*)pComponent );

	Py_RETURN_NONE;
}


/*~ function GUI.reSort
 *	@components{ client, tools }
 *
 *	This function resorts the gui components at the root of the GUI tree.
 *	This function sorts according to the third (depth) component of the
 *	children's position attributes.  Changing this depth value in a child
 *	doesn't automatically reorder the children, so an explicit call to
 *	this function is required.
 *
 *	@return Read-Only list of GUI components.
 *
 */
/**
 *	This method is a script wrapper to the C++ method.
 */
PyObject * SimpleGUI::py_reSort( PyObject * args )
{
	BW_GUARD;
	SimpleGUI::instance().reSort();

	Py_RETURN_NONE;
}


/*~ function GUI.roots
 *	@components{ client, tools }
 *
 *	This function returns a list of the current root components.  It is not
 *	editable.  Instead, use the GUI.addRoot and GUI.delRoot to add and remove
 *	components from the list.
 *
 *	@return Read-Only list of GUI components.
 *
 */
/**
 *	This method returns a list of the current roots. It is
 *	currently not editable.
 *	TODO: Use stl_to_py and make it editable. Then turn it
 *	into a module property instead of this function.
 */
PyObject * SimpleGUI::py_roots( PyObject * args )
{
	BW_GUARD;
	Components & cs = SimpleGUI::instance().components_;

	PyObject * pList = PyList_New( cs.size() );
	for (uint i = 0; i < cs.size(); i++)
	{
		PyList_SetItem( pList, i, cs[i] );
		Py_INCREF( cs[i] );
	}
	return pList;
}


/*~ function GUI.update
 *	@components{ client, tools }
 *
 *	This function causes the GUI to be updated as if the specified amount of
 *	time had passed.  This updates all of the components and shaders which
 *	have been added to the GUI root.
 *
 *	The system automatically updates the gui each tick with dTime on each
 *	update being the actual time since the last update.  This call would not
 *	normally need to be used.
 *
 *	@param	dTime	The amount of time assumed to have passed since the last
 *					update.
 */
/**
 *	This method gets the GUI to update
 */
static void update( float dtime )
{
	BW_GUARD;
	SimpleGUI::instance().update( dtime );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, update, ARG( float, END ), GUI )


/*~ function GUI.draw
 *	@components{ client, tools }
 *
 *	This method causes the GUI and all components which have been added as
 *	roots to be redrawn.
 *
 *	It is called automatically by the system each tick.  It would not normally
 *	be necessary to call this function.
 */
/**
 *	This method gets the GUI to draw
 */
static void draw()
{
	BW_GUARD;
	SimpleGUI::instance().draw();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, draw, END, GUI )


/*~ function GUI.handleKeyEvent
 *	@components{ client, tools }
 *
 *	This functions gets the GUI to handle the given key event.  It hands the 
 *	event to each of its child components which have the focus attribute set, 
 *	until one of them returns non zero, at which point it returns non-zero. 
 *	Otherwise, it returns zero.
 *
 *	This needs to be called from the personality script, otherwise the GUI will
 *	not process key events.
 *
 *	@param	event	A PyKeyEvent containing information about this key event.
 *
 *	@return			non-zero means the event was handled, zero means it wasn't.
 */
/**
 *	This method gets the GUI to handle the given key event
 */
static bool handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	return SimpleGUI::instance().handleKeyEvent( event );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, handleKeyEvent, ARG( KeyEvent, END ), GUI )


/*~ function GUI.handleMouseEvent
 *	@components{ client, tools }
 *
 *	This function gets the GUI to handle the given mouse event.  It hands the 
 *	event to each of its child components which have the focus attribute set, 
 *	until one of them returns non zero, at which point it returns non-zero. 
 *	Otherwise, it returns zero.
 *
 *	This needs to be called from the personality script, otherwise the GUI will
 *	not process mouse events.
 *
 *	@param	event	A PyMouseEvent object.
 *
 *	@return			non-zero means the event was handled, zero means it wasn't.
 */
/**
 *	This method gets the GUI to handle the given mouse event
 */
static bool handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;
	return SimpleGUI::instance().handleMouseEvent( event );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, handleMouseEvent, ARG( MouseEvent, END ), GUI )


/*~ function GUI.handleAxisEvent
 *	@components{ client, tools }
 *
 *	This function gets the GUI to handle the given axis event.  It hands the 
 *	event to each of its child components which have the focus attribute set, 
 *	until one of them returns non zero, at which point it returns non-zero.
 *	Otherwise, it returns zero.
 *
 *	This needs to be called from the personality script, otherwise the GUI will
 *	not process axis events.
 *
 *	@param	event	A PyAxisEvent object.
 *
 *	@return			non-zero means the event was handled, zero means it wasn't.
 */
/**
 *	This method gets the GUI to handle the given axis event
 */
static bool handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	return SimpleGUI::instance().handleAxisEvent( event );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, handleAxisEvent, ARG( AxisEvent, END ), GUI )


/*~	function GUI.screenResolution
 *	@components{ client, tools }
 *
 *	This function simply returns the current (width, height) in pixels of the
 *	screen. If a resolution override has been set, then this returns the
 *	override. Use BigWorld.screenSize if you need the physical resolution
 *	regardless of whether the GUI resolution has been overridden.
 *
 *	@return	A 2-tuple representing the width and height of the screen, in
 *	pixels.
 */
/**
 *	This method gets the current screen resolution, in pixels.
 */
static PyObject * py_screenResolution( PyObject * args )
{
	BW_GUARD;
	PyObject * pTuple = PyTuple_New( 2 );

	if ( SimpleGUI::instance().usingResolutionOverride() )
	{
		Vector2 resolutionOverride = SimpleGUI::instance().resolutionOverride();
		PyTuple_SET_ITEM( pTuple, 0, Script::getData( resolutionOverride.x ) );
		PyTuple_SET_ITEM( pTuple, 1, Script::getData( resolutionOverride.y ) );
	}
	else
	{
		PyTuple_SET_ITEM( pTuple, 0, Script::getData( Moo::rc().screenWidth() ) );
		PyTuple_SET_ITEM( pTuple, 1, Script::getData( Moo::rc().screenHeight() ) );
	}

	return pTuple;
}
PY_MODULE_FUNCTION( screenResolution, GUI )

/*~	function GUI.setResolutionOverride
 *	@components{ client, tools }
 *
 *	This method overrides the resolution that the GUI system uses for
 *	calculations. This allows components setup in pixel space to be 
 *	automatically scaled. For example, if the resolution override is set
 *	to 1024x768, then the GUI will take up the same area of the screen 
 *	no matter what the actual screen resolution is.
 *
 *	@param	resolution	A Vector2 of the new override. If it is a zero vector
 *						then the override is disabled.
 */
static void setResolutionOverride( const Vector2& res )
{
	BW_GUARD;
	SimpleGUI::instance().resolutionOverride( res );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, setResolutionOverride, ARG( Vector2, END ), GUI )


/*~	function GUI.setDragDistance
 *	@components{ client, tools }
 *
 *	Sets the minimum distance the mouse pointer has to travel after the left
 *	mouse button has been pressed before the movement is considered a drag.
 *
 *	Once the movement is classified as dragging, a click event will no longer 
 *	be generated if the mouse is released over the dragged component.
 *
 *	@param	distance	the minimum drag distance (float, in clip space).
 *						The default value is 0.002.
 */
/**
 *	Sets the Sets the minimum drag distance.
 */
static void setDragDistance( float dragDistance )
{
	BW_GUARD;
	SimpleGUI::instance().dragDistance( dragDistance );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, setDragDistance, ARG( float, END ), GUI )


/**
 *	print the GUI structure (as info msgs) this is useful for debugging of GUI structures
 */
void SimpleGUI::print()
{
	BW_GUARD;
	for ( size_t n = components_.size(); n > 0; --n )
	{
		SimpleGUIComponent * pComponent = components_[ n - 1 ];
		pComponent->print("");
	}
}


/*~	function GUI.printRoots
 *	@components{ client, tools }
 *
 *	Prints the GUI structure (as info msgs) this is useful for debugging GUI structures.
 *
 */
/**
 *	print the GUI structure (as info msgs) this is useful for debugging of GUI structures
 */
void printRoots()
{
	BW_GUARD;
	SimpleGUI::instance().print();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, printRoots, END, GUI )



// Class linking definitions
extern int SimpleGUIComponent_token;
extern int TextGUIComponent_token;
extern int FrameGUIComponent_token;
extern int FrameGUIComponent2_token;
#if ENABLE_CONSOLES
extern int ConsoleGUIComponent_token;
#endif // ENABLE_CONSOLES
extern int BoundingBoxGUIComponent_token;
extern int AlphaGUIShader_token;
extern int ClipGUIShader_token;
extern int ColourGUIShader_token;
extern int MatrixGUIShader_token;
extern int WindowGUIComponent_token;
extern int GuiAttachment_token;
extern int GraphGUIComponent_token;
extern int MeshGUIAdaptor_token;
extern int GoboComponent_token;
int GUI_token =
	SimpleGUIComponent_token &&
	TextGUIComponent_token &&
	FrameGUIComponent_token &&
	FrameGUIComponent2_token &&
#if ENABLE_CONSOLES
	ConsoleGUIComponent_token &&
#endif // ENABLE_CONSOLES
	BoundingBoxGUIComponent_token &&
	AlphaGUIShader_token &&
	ClipGUIShader_token &&
	ColourGUIShader_token &&
	MatrixGUIShader_token &&
	WindowGUIComponent_token &&
	GuiAttachment_token &&
	GraphGUIComponent_token &&
	MeshGUIAdaptor_token &&
	GoboComponent_token;



BW_END_NAMESPACE

// simple_gui.cpp
// simple_gui.cpp
