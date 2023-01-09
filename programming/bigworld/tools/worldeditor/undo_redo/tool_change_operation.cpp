#include "pch.hpp"
#include "tool_change_operation.hpp"
#include "gizmo/tool.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"

BW_BEGIN_NAMESPACE

ToolChangeOperation::ToolChangeOperation( PanelManager& panelManager,
	const BW::wstring& oldToolId,
	const BW::wstring& newToolId ) :

	UndoRedo::Operation( size_t( typeid( ToolChangeOperation ).name() ) ),
	panelManager_( panelManager ),
	oldToolId_( oldToolId ),
	newToolId_( newToolId )
{
}

/**
 *	Restore the tool.
 */
void ToolChangeOperation::undo()
{
	// First add the redo operation (opposite)
	UndoRedo::instance().add( new ToolChangeOperation(
		panelManager_, newToolId_, oldToolId_ ) );

	// Now apply our stored change
	panelManager_.setToolMode( oldToolId_, false );
}

bool ToolChangeOperation::iseq( const UndoRedo::Operation & oth ) const
{
	const ToolChangeOperation& other =
		static_cast<const ToolChangeOperation&>( oth );

	return (oldToolId_ == other.oldToolId_) && (newToolId_ == other.newToolId_);
}

BW_END_NAMESPACE
// tool_change_operation.cpp
