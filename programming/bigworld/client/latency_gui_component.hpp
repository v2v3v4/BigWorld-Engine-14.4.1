#ifndef LATENCY_GUI_COMPONENT_HPP
#define LATENCY_GUI_COMPONENT_HPP

#include "ashes/simple_gui_component.hpp"


BW_BEGIN_NAMESPACE

class TextGUIComponent;


/*~ class BigWorld.LatencyGUIComponent
 *
 *	This class is used to display the word "Offline" on the screen if the
 *  client is not connected to the server.  If the client is connected to the
 *	server, then the visible attribute of the LatencyGUIObject is automatically
 *	set to false.
 *
 *	This class inherits functionality from SimpleGUIComponent.
 */
/**
 *	This class is a GUI component to draw the latency meter
 */
class LatencyGUIComponent : public SimpleGUIComponent
{
	Py_Header( LatencyGUIComponent, SimpleGUIComponent )

public:
	LatencyGUIComponent( PyTypeObject* pType = &s_type_ );
	~LatencyGUIComponent();

	PY_FACTORY_DECLARE()

	void		draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay );

private:
	TextGUIComponent* txt_;

	LatencyGUIComponent(const LatencyGUIComponent&);
	LatencyGUIComponent& operator=(const LatencyGUIComponent&);
};



#ifdef CODE_INLINE
#include "latency_gui_component.ipp"
#endif

BW_END_NAMESPACE

#endif
/*latency_gui_component.hpp*/
