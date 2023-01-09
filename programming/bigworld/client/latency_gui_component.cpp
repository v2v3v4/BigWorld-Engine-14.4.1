#include "pch.hpp"
#include "latency_gui_component.hpp"
#include "connection_control.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "romp/font.hpp"
#include "romp/font_manager.hpp"
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "latency_gui_component.ipp"
#endif



PY_TYPEOBJECT( LatencyGUIComponent )

PY_BEGIN_METHODS( LatencyGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( LatencyGUIComponent )
PY_END_ATTRIBUTES()

/*~ function GUI.Latency
 *
 *	This function creates a new LatencyGUIComponent.  This will display the
 *  word "Offline" on the screen if the client is not connected to the server,
 *  and nothing otherwise.
 *
 *	@return		a new LatencyGUIComponent.
 */
PY_FACTORY_NAMED( LatencyGUIComponent, "Latency", GUI )



LatencyGUIComponent::LatencyGUIComponent( PyTypeObject* pType )
 	: SimpleGUIComponent( "" )
{
	BW_GUARD;
	this->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	this->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );

	this->position(Vector3(-0.85f, -0.85f, 0.0f));
	this->width(1.7f);
	this->height(0.01f);
	this->anchor( SimpleGUIComponent::ANCHOR_H_LEFT, SimpleGUIComponent::ANCHOR_V_BOTTOM );

	CachedFontPtr tf = FontManager::instance().getCachedFont( "default_medium.font" );
	//tf->scale( 0.75f, 0.75f );

	txt_ = new TextGUIComponent(tf);
	txt_->label(L"Offline");
	txt_->colour(0x808080ff);
	txt_->anchor(SimpleGUIComponent::ANCHOR_H_LEFT, SimpleGUIComponent::ANCHOR_V_BOTTOM);
	txt_->position(Vector3(-0.85f, -0.85f, 0));
	this->addChild( "label", txt_ );
}


LatencyGUIComponent::~LatencyGUIComponent()
{
	BW_GUARD;
	Py_DECREF(txt_);
}


/**
 *	Static python factory method
 */
PyObject* LatencyGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	return new LatencyGUIComponent();
}

/**
 *	This method draws the meter
 */
void LatencyGUIComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	if (!SimpleGUIComponent::visible())
		return;

	BW::vector<int>::iterator it = childOrder_.begin();
	BW::vector<int>::iterator end = childOrder_.end();
	while (it != end)
	{
		(children_.begin() + *it)->second->draw( drawContext, reallyDraw, overlay );
		it++;
	}

	if (reallyDraw)
	{
		BWServerConnection * pServerConn =
			ConnectionControl::instance().pServerConnection();

		if (pServerConn != NULL && pServerConn->isOnline())
		{

			txt_->visible(false);
		}
		else
		{
			txt_->visible(true);
		}

		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( GUIVertex::fvf() );
	}
}





std::ostream& operator<<(std::ostream& o, const LatencyGUIComponent& t)
{
	BW_GUARD;
	o << "LatencyGUIComponent\n";
	return o;
}

BW_END_NAMESPACE

/*latency_gui_component.cpp*/
