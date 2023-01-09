#ifndef GENPROP_GIZMOVIEWS_HPP
#define GENPROP_GIZMOVIEWS_HPP

#include "general_editor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class maintains the kind id used by gizmo property views.
 */
class GizmoViewKind
{
public:
	static int kindID()
	{
		static int kid = GeneralProperty::nextViewKindID();
		return kid;
	}
};



BW_END_NAMESPACE
#endif // GENPROP_GIZMOVIEWS_HPP
