#ifndef WINDOW_GUI_COMPONENT_HPP
#define WINDOW_GUI_COMPONENT_HPP

#include "simple_gui_component.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.WindowGUIComponent
 *	@components{ client, tools }
 *
 *	This class forms the basis of windowing functionality for the GUI.
 *	Children added to a window are clipped and scrolled inside it.
 *	Children of Window components inherit the translation of the window.
 *
 *  If a child has either of its position modes set to "CLIP" then 
 *  then this window is used as the basis for that child's clip space.
 *  In other words (-1,-1) is the bottom left of this window, and (1,1)
 *  is the top right of this window, and (0,0) is the centre of this
 *  window.
 *
 *  If a child has either of its position modes set to "PIXEL" then 
 *  the position coordinate will be relative to the top left of this
 *  window.
 */
/**
 *	This class implements a window that supports hierarchical
 *	transforms, a clipping area and has scrollable contents.
 */
class WindowGUIComponent : public SimpleGUIComponent
{
	Py_Header( WindowGUIComponent, SimpleGUIComponent )

public:
	WindowGUIComponent( const BW::string& textureName = "",
		PyTypeObject * pType = &s_type_ );
	~WindowGUIComponent();

	void update( float dTime, float relParentWidth, float relParentHeight );
	void draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay = true );

	PY_RW_ATTRIBUTE_REF_DECLARE( scroll_, scroll );
	PY_RW_ATTRIBUTE_REF_DECLARE( scrollMin_, minScroll );
	PY_RW_ATTRIBUTE_REF_DECLARE( scrollMax_, maxScroll );

	PY_FACTORY_DECLARE()
protected:
	WindowGUIComponent(const WindowGUIComponent&);
	WindowGUIComponent& operator=(const WindowGUIComponent&);

	virtual void updateChildren( float dTime, float relParentWidth, float relParentHeight );
	virtual ConstSimpleGUIComponentPtr nearestRelativeParent(int depth=0) const;

	virtual Vector2 localToScreenInternalOffset( bool isThis ) const;

	bool load( DataSectionPtr pSect, const BW::string& ownerName,  LoadBindings & bindings );
	void save( DataSectionPtr pSect, SaveBindings & bindings );

	Matrix scrollTransform_;
	Matrix anchorTransform_;
	Vector2 scroll_;
	Vector2 scrollMin_;
	Vector2 scrollMax_;

	COMPONENT_FACTORY_DECLARE( WindowGUIComponent() )
};

#ifdef CODE_INLINE
#include "window_gui_component.ipp"
#endif

BW_END_NAMESPACE

#endif // WINDOW_GUI_COMPONENT_HPP

