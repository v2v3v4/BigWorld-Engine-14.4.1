#ifndef LINK_PROPERTY_VIEW_HPP
#define LINK_PROPERTY_VIEW_HPP

#include "gizmo/genprop_gizmoviews.hpp"
#include "worldeditor/editor/link_property.hpp"

BW_BEGIN_NAMESPACE

class LinkGizmo;
typedef SmartPointer< LinkGizmo > LinkGizmoPtr;


/**
 *	This creates LinkGizmos.
 */
class LinkPropertyView : public GeneralProperty::View
{
public:
	LinkPropertyView( LinkProperty & prop );
	~LinkPropertyView();

	virtual void elect();
	virtual void expel();
	virtual void select();
	static GeneralProperty::View * create( LinkProperty & prop );

private:
	LinkProperty	    *prop_;
	static LinkGizmoPtr s_pGizmo_;
	static int			s_count_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			LinkProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

BW_END_NAMESPACE

#endif // LINK_PROPERTY_VIEW_HPP
