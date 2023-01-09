#include "pch.hpp"
#include "link_functor.hpp"
#include "gizmo/tool_manager.hpp"
#include "gizmo/current_general_properties.hpp"
#include "worldeditor/editor/link_gizmo.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

/**
 *  LinkFunctor constructor.
 *
 *  @param linkProxy    A proxy to consult regarding linking.
 */
LinkFunctor::LinkFunctor( LinkProxyPtr linkProxy,
	bool allowedToDiscardChanges ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges ),
    linkProxy_( linkProxy )
{
}


void LinkFunctor::stopApplyCommitChanges( Tool & tool, bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		LinkProxy::TargetState canLink =
			linkProxy_->canLinkAtPos( tool.locator() );
		if (canLink == LinkProxy::TS_CAN_LINK)
		{
			linkProxy_->createLinkAtPos( tool.locator() );
		}

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}

void LinkFunctor::stopApplyDiscardChanges( Tool & tool )
{
	BW_GUARD;

	if (applying_)
	{
		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}
BW_END_NAMESPACE

