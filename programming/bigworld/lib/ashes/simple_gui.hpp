#ifndef SIMPLE_GUI_HPP
#define SIMPLE_GUI_HPP

#include "cstdmf/bw_vector.hpp"
#include <stack>

#include "mouse_cursor.hpp"
#include "moo/moo_math.hpp"
#include "moo/moo_dx.hpp"
#include "input/input.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "cstdmf/singleton.hpp"

#include "cstdmf/stdmf.hpp"
#include "moo/render_context.hpp"


BW_BEGIN_NAMESPACE

class SimpleGUIComponent;
typedef SmartPointer<SimpleGUIComponent> SimpleGUIComponentPtr;
typedef ConstSmartPointer<SimpleGUIComponent> ConstSimpleGUIComponentPtr;


extern int GUI_token;

namespace Moo
{
	class DrawContext;
}

/*~ module GUI
 *	@components{ client, tools }
 *
 *	The GUI module contains all the functions which are used to create GUI
 *  components and shaders,
 *	to display them on the screen, and to handle input events.
 */

/**
 * This is a singleton class that manages SimpleGUIComponents.
 * It maintains the roots of the GUI component tree.
 *
 * Any components added to / created by the SimpleGUI will
 * be automatically freed upon destruction.
 */
class SimpleGUI : public InputHandler, public Singleton< SimpleGUI >
{
public:
	typedef BW::vector< SimpleGUIComponent* > FocusList;
	typedef BW::vector< SimpleGUIComponent* > Components;

	~SimpleGUI();

	void				hInstance( void *h );
	void				hwnd( void *h );
	void *				hwnd() const;

	static void			init( DataSectionPtr pConfig );
	static void			fini();

	const MouseCursor &	mouseCursor() const;
	MouseCursor &		mouseCursor();

	/// This method returns whether or not the resolution has changed.
	bool				hasResolutionChanged( void )
											{ return resolutionHasChanged_; }

	//SimpleGUIComponent&	createSimpleComponent( const BW::string& textureName );
	void				addSimpleComponent( SimpleGUIComponent& c );
	void				removeSimpleComponent( SimpleGUIComponent& c );
	void				clearSimpleComponents();
	void				update( float dTime );
	void				draw();
	void				appFocus( bool state );
	bool				appFocus() const;
	void				setUpdateEnabled( bool enable = true );

	void				reSort();

	void				pixelRangesToClip( float w, float h, float* retW, float* retH );
	void				clipRangesToPixel( float w, float h, float* retW, float* retH );

	void				resolutionOverride( const Vector2& res );
	const Vector2&		resolutionOverride() const;
	bool				usingResolutionOverride() const;

	float				screenWidth() const;
	float				screenHeight() const;
	float				halfScreenWidth() const;
	float				halfScreenHeight() const;
	Vector2				screenResolution() const;
	uint32				realScreenResolutionCounter() const;
	Vector2				pixelToClip() const;


	void				countDrawCall();

	/// Input method
	void				addInputFocus( SimpleGUIComponent* c );
	void				delInputFocus( SimpleGUIComponent* c );

	void				addMouseButtonFocus( SimpleGUIComponent* c );
	void				delMouseButtonFocus( SimpleGUIComponent* c );

	void				addMouseCrossFocus( SimpleGUIComponent* c );
	void				delMouseCrossFocus( SimpleGUIComponent* c );

	void				addMouseMoveFocus( SimpleGUIComponent* c );
	void				delMouseMoveFocus( SimpleGUIComponent* c );

	void				addMouseDragFocus( SimpleGUIComponent* c );
	void				delMouseDragFocus( SimpleGUIComponent* c );
	void				addMouseDropFocus( SimpleGUIComponent* c );
	void				delMouseDropFocus( SimpleGUIComponent* c );

	bool				handleKeyEvent( const KeyEvent & /*event*/ );
	bool				handleMouseEvent( const MouseEvent & /*event*/ );
	bool				handleAxisEvent( const AxisEvent & /*event*/ );

	bool				processClickKey( const KeyEvent & event );
	bool				processDragKey( const KeyEvent & event );
	bool				processMouseMove( const MouseEvent & event );
	bool				processDragMove( const MouseEvent & event );

	/// Hierarchy / clipping methods
	bool				pushClipRegion( SimpleGUIComponent& c );	
	bool				pushClipRegion( const Vector4& );
	void				popClipRegion();	
	const Vector4&		clipRegion();
	bool				isPointInClipRegion( const Vector2& pt );
	void				setConstants(DWORD colour, bool pixelSnap);
	bool				existsInTree( const SimpleGUIComponent * pComponent ) const;

	void				recalcDrawOrders();
	void				print();

	void				dragDistance( float distance );

	PY_MODULE_STATIC_METHOD_DECLARE( py_addRoot )
	PY_MODULE_STATIC_METHOD_DECLARE( py_delRoot )
	PY_MODULE_STATIC_METHOD_DECLARE( py_reSort )
	PY_MODULE_STATIC_METHOD_DECLARE( py_roots )

private:
	SimpleGUI();
	SimpleGUI(const SimpleGUI&);
	SimpleGUI& operator=(const SimpleGUI&);

	MouseCursor &		internalMouseCursor() const;
	bool				commitClipRegion();
	SimpleGUIComponent* closestHitTest( FocusList& list, const Vector2 & pos );
	void				filterList( const FocusList& input, FocusList& result );
	void				tickCrossFocusEvents();
	void				tickDragFocusEvents();
	template< class EventForwarderT >
		WeakPyPtr< SimpleGUIComponent > detectEnterLeaveEvents(	MouseCursor& mcursor,
											const SimpleGUI::FocusList & crossFocusList,
											WeakPyPtr< SimpleGUIComponent > currentOverComponent,
											EventForwarderT eventForwarder);

	uint32				crossFocusFrame_;
	uint32				dragFocusFrame_;

	mutable MouseCursor * pMouseCursor_;

	//SmartPointer< SimpleGUIComponent >	pRoot_;
	typedef std::auto_ptr< struct DragInfo >	DragInfoPtr;
	Components			components_;

	Vector2				resolutionOverride_;

	FocusList			mouseButtonFocusList_;
	FocusList			crossFocusList_;
	FocusList			dragFocusList_;
	FocusList			dropFocusList_;
	SimpleGUIComponent* clickComponent_;
	DragInfoPtr         dragInfo_;

	WeakPyPtr< SimpleGUIComponent > currentCrossFocusComponent_;
	WeakPyPtr< SimpleGUIComponent > currentDropFocusComponent_;

	float				dragDistanceSqr_;

	bool				resolutionHasChanged_;
	Vector2				lastResolution_;

	Vector2				lastRealResolution_;
	uint32				realResolutionCounter_;
	bool				appFocus_;
	bool				updateGUI_;

	void*				hwnd_;
	void*				hInstance_;
	float				dTime_;

	typedef std::stack< Vector4, BW::vector< Vector4 > > ClipStack;
	ClipStack			clipStack_;
	//PC client needs this because the clip stack is implemented using viewports
	//and view matrices
	DX::Viewport		originalView_;
	Matrix				clipFixer_;
	uint32				guiTreeCookie_;

	size_t				drawCallCount_;
	bool				legacyKeyboard_;
	Moo::DrawContext*	guiDrawContext_;
	typedef SimpleGUI This;
};


#ifdef CODE_INLINE
#include "simple_gui.ipp"
#endif

BW_END_NAMESPACE

#endif // SIMPLE_GUI_HPP
