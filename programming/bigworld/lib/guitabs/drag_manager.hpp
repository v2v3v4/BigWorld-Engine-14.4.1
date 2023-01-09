
#ifndef GUITABS_DRAG_MANAGER_HPP
#define GUITABS_DRAG_MANAGER_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	This class represents a dummy tab that is temporarily inserted on a panel
 *	when dragging another panel into it, giving feedback that the dragged panel
 *	will be inserted as a tab.
 */
class TempTab : public Tab
{
public:
	TempTab() : Tab( 0, L"" ) {}
	void setLabel( const BW::wstring& label ) { label_ = label; }
	BW::wstring getDisplayString() { return label_; }
	BW::wstring getTabDisplayString() { return label_; }
private:
	BW::wstring label_;
};
typedef SmartPointer<TempTab> TempTabPtr;


/**
 *  This singleton class is responsible for managing user events from the
 *  momentwhen a panel begins to be dragged until the panel is released, be it
 *  on top of another panel, or in a non-dockable area. After determining
 *  where a panel or tab has been dragged, this class will call the appropriate
 *  methods in Dock to actually accomplish the final results (dock a panel into
 *  a panel, float a tab dragging it outside of a panel, etc).
 */
class DragManager : public ReferenceCount
{
public:
	DragManager();

	void startDrag( int x, int y, PanelPtr panel, TabPtr tab );

private:
	PanelPtr panel_;
	bool tabCtrlAtTop_;
	TabPtr tab_;
	PanelPtr destPanel_; // used to show tab position feedback
	TempTabPtr tempTab_; // used to show tab position feedback
	int tabInsertIndex_; // used to properly set a tabs position on end drag
	int startX_;
	int startY_;
	int downX_;
	int downY_;
	bool isDragging_;
	bool isFloating_;
	bool hilightSrc_;

	InsertAt lastIns_;
	CRect lastRect_;
	CSize lastSize_;
	CWnd* wnd_;
	CDC* dc_;

	void prepareDrag();
	void updateDrag( int x, int y );
	void endDrag( int x, int y );
	void cancelDrag( bool leaveTempTab = false );
	void drawDragRect( InsertAt ins, CRect rect, SIZE size );

	void dragLoop();

	InsertAt getInsertLocation( DockNodePtr node, int x, int y, CRect& locationRect );
};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_DRAG_MANAGER_HPP
