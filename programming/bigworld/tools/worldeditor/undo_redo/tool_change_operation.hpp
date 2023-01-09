#ifndef TOOL_CHANGE_OPERATION_HPP
#define TOOL_CHANGE_OPERATION_HPP

#include "cstdmf/smartpointer.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

class PanelManager;
class Tool;
typedef SmartPointer<Tool> ToolPtr;

/**
 *	This class is used for undoing tool changes.
 *	Undoing tool changes prevents the editor getting in strange states.
 *	Eg. move item, change to terrain paint, undo - end up terrain painting and
 *	moving item at the same time.
 */
class ToolChangeOperation : public UndoRedo::Operation
{
public:
	ToolChangeOperation( PanelManager& panelManager,
		const BW::wstring& oldToolId,
		const BW::wstring& newToolId );

	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;
private:
	PanelManager& panelManager_;
	BW::wstring oldToolId_;
	BW::wstring newToolId_;
};

BW_END_NAMESPACE

#endif // TOOL_CHANGE_OPERATION_HPP
