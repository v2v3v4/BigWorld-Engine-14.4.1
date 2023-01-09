#ifndef FRAME_GUI_COMPONENT2_HPP
#define FRAME_GUI_COMPONENT2_HPP

#include "simple_gui_component.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.FrameGUIComponent2
 *	@components{ client, tools }
 *
 *	This class inherits from SimpleGUIComponent.  It draws a resizable
 *	frame in 1 draw call, using a very specific texture layout.  Please
 *	see the CCM for diagrams and information on how to author the texture
 *	map.
 *	A new FrameGUIComponent2 is created using GUI.Frame2 function.
 */
/**
 *	This class inherits from SimpleGUIComponent.  It draws a resizable
 *	frame in 1 draw call, using a very specific texture layout.  Please
 *	see the CCM for diagrams and information on how to author the texture
 *	map.
 *	A new FrameGUIComponent2 is created using GUI.Frame2 function.
 */
class FrameGUIComponent2 : public SimpleGUIComponent
{
	Py_Header( FrameGUIComponent2, SimpleGUIComponent )

public:
	FrameGUIComponent2( const BW::string& textureName, PyTypeObject * pType = &s_type_ );

	virtual void update( float dTime, float relParentWidth, float relParentHeight );

	PY_FACTORY_DECLARE()

protected:
	virtual void buildMesh();

private:
	FrameGUIComponent2(const FrameGUIComponent2&);
	FrameGUIComponent2& operator=(const FrameGUIComponent2&);

	void updateVertices( GUIVertex* v, float relativeParentWidth, float relativeParentHeight );
	GUIVertex* setQuad(
		GUIVertex* pFirstVert,
		float x1, float y1,
		float x2, float y2,
		float x3, float y3,
		float x4, float y4,
		float u1, float v1,
		float u2, float v2,
		float u3, float v3,
		float u4, float v4 );

	COMPONENT_FACTORY_DECLARE( FrameGUIComponent2("") )
};
BW_END_NAMESPACE


#endif // FRAME_GUI_COMPONENT2_HPP
