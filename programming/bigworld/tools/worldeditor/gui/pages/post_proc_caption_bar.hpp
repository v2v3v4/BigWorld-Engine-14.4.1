
#pragma once
#include "resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/themed_image_button.hpp"
#include "afxwin.h"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a caption bar for the post processing panel.
 */
class PostProcCaptionBar : public CDialog
{
	DECLARE_DYNAMIC(PostProcCaptionBar)

public:
	static const char * OPTION_PREVIEW3D;
	static const char * OPTION_PROFILE;

	PostProcCaptionBar( CWnd * pParent = NULL );
	virtual ~PostProcCaptionBar();

	void setEventHandler( CWnd * handler ) { handler_ = handler; }

	enum { IDD = IDD_POST_PROC_CAPTIONBAR };

	void captionText( const BW::wstring & text );

	void setPreview3D( bool activate );

	void setProfile( bool activate );

protected:
	DECLARE_AUTO_TOOLTIP( PostProcCaptionBar, CDialog );
	virtual void DoDataExchange( CDataExchange * pDX );    // DDX/DDV support

	virtual BOOL OnInitDialog();

	void OnOK() {}
	void OnCancel() {}

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnNoZoom();
	afx_msg void On3dPreview();
	afx_msg void OnProfile();
	afx_msg void OnLayout();
	afx_msg void OnDeleteAll();
	DECLARE_MESSAGE_MAP()

private:
	CWnd * handler_;
	CStatic graphCaption_;
	controls::ThemedImageButton zoomIn_;
	controls::ThemedImageButton zoomOut_;
	controls::ThemedImageButton noZoom_;
	controls::ThemedImageButton preview3D_;
	controls::ThemedImageButton profile_;
	controls::ThemedImageButton layout_;
	controls::ThemedImageButton deleteAll_;

	HICON zoomInImg_;
	HICON zoomOutImg_;
	HICON noZoomImg_;
	HICON previewImg_;
	HICON previewOnImg_;
	HICON profileImg_;
	HICON profileOnImg_;
	HICON layoutImg_;
	HICON deleteImg_;

	bool inited_;
};
BW_END_NAMESPACE

