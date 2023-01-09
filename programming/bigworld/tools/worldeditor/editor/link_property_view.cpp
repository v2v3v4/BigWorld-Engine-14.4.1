#include "pch.hpp"
#include "link_property_view.hpp"
#include "link_gizmo.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

LinkPropertyView::ViewEnroller LinkPropertyView::s_viewEnroller;
/*static*/ LinkGizmoPtr LinkPropertyView::s_pGizmo_;
/*static*/ int LinkPropertyView::s_count_ = 0;


LinkPropertyView::LinkPropertyView( LinkProperty & prop ) : prop_( &prop )
{		
}


LinkPropertyView::~LinkPropertyView()
{
	BW_GUARD;

    prop_ = NULL;
}


void LinkPropertyView::elect()
{
	BW_GUARD;

	if (prop_->alwaysShow())
	{
		if (s_count_++ == 0)
		{
			s_pGizmo_ = new LinkGizmo(prop_->link(), prop_->matrix());
			GizmoManager::instance().addGizmo( s_pGizmo_ );
		}
	}
}


void LinkPropertyView::expel()
{
	BW_GUARD;

	if (prop_->alwaysShow())
	{
		if (--s_count_ == 0)
		{
			if (s_pGizmo_)
			{
				GizmoManager::instance().removeGizmo( s_pGizmo_ );
				s_pGizmo_ = NULL;
			}
		}
	}
}


void LinkPropertyView::select()
{
	BW_GUARD;

	if (s_pGizmo_)
	{
		GizmoSetPtr set = new GizmoSet();
		// recreate the gizmo for this particular property
		s_pGizmo_->update( prop_->link(), prop_->matrix() );
		set->add( s_pGizmo_ );
		GizmoManager::instance().forceGizmoSet( set );
	}
}


/*static*/ GeneralProperty::View * LinkPropertyView::create(
	LinkProperty & prop )
{
	BW_GUARD;

	return new LinkPropertyView( prop );
}

BW_END_NAMESPACE
// link_property_view.cpp
