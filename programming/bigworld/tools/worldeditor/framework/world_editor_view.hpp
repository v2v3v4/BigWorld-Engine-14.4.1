#ifndef WORLD_EDITOR_VIEW_HPP
#define WORLD_EDITOR_VIEW_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class WorldEditorView : public CView
{
protected: 
	DECLARE_DYNCREATE(WorldEditorView)

	WorldEditorView();

public:
	virtual ~WorldEditorView();

	WorldEditorDoc* GetDocument() const;

	virtual void OnDraw(CDC* pDC); 
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	afx_msg void OnPaint();	
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	afx_msg BOOL OnSetCursor( CWnd*, UINT, UINT );
	afx_msg void OnKillFocus( CWnd *pNewWnd );
	DECLARE_MESSAGE_MAP()

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	CRect		lastRect_;
};

BW_END_NAMESPACE

#endif // WORLD_EDITOR_VIEW_HPP
