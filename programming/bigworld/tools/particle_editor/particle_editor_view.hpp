#ifndef PARTICLE_EDITOR_VIEW_HPP
#define PARTICLE_EDITOR_VIEW_HPP

#include "fwd.hpp"

BW_BEGIN_NAMESPACE

class ParticleEditorView : public CView
{
protected:
	ParticleEditorView();

	BOOL PreCreateWindow(CREATESTRUCT& cs);

	DECLARE_DYNCREATE(ParticleEditorView)

	CRect lastRect_;

public:
    /*virtual*/ ~ParticleEditorView();

    static ParticleEditorView *instance();

	ParticleEditorDoc* GetDocument() const;

    /*virtual*/ void OnDraw(CDC *dc);

	/*virtual*/ void 
    OnActivateView
    (
        BOOL            bActivate, 
        CView           *pActivateView, 
        CView           *
    );

protected:
    afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnKillFocus( CWnd *pNewWnd );
	DECLARE_MESSAGE_MAP()

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    static ParticleEditorView   *s_instance_;
};

BW_END_NAMESPACE

#endif // PARTICLE_EDITOR_VIEW_HPP
